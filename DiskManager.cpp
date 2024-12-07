

#include "common.h"

#include "resource.h"



TCHAR g_DskFilePath[MAX_PATH];


// EncounterHD-FR-v0.9.0.dsk or EncounterHD-EN-v0.9.0.dsk
void SetDiskLanguage(const TCHAR* languagePrefix)
{
	GetModuleFileName(NULL, g_DskFilePath, MAX_PATH);
	PathRemoveFileSpec(g_DskFilePath);
	PathAppend(g_DskFilePath, TEXT("game\\"));

	// Get a pointer to the zero terminator
	auto pathLength = lstrlen(g_DskFilePath);
	_stprintf_s(g_DskFilePath + pathLength, MAX_PATH - pathLength, _T("EncounterHD-%s-v*.dsk"), languagePrefix);


	WIN32_FIND_DATA findFileData;
	HANDLE hFind = FindFirstFile(g_DskFilePath, &findFileData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		MessageBox(0, TEXT("Disk file not found"), TEXT("Error"), MB_OK | MB_ICONERROR);
	}
	else
	{
		_tcscpy_s(g_DskFilePath + pathLength, MAX_PATH - pathLength, findFileData.cFileName);
		FindClose(hFind);
	}
}


bool ExtractMemoryBlock(std::vector<char>& buffer, dsk_file::SaveGameFile& saveFile, bool reading)
{
	//
	// Note: Due to the way a disk is structure the two 256 sectors:
	// - are not consecutive in the file due to the presence of gaps and other synchronization elements.
	// - are not necessarily placed in a specific order due to the interleave factor
	//
	std::string data(buffer.begin(), buffer.end());
	size_t firstHalf  = data.find("SAVESTRT");
	size_t secondHalf = data.find("SAVE-END");

	if ( (firstHalf != std::string::npos) && (secondHalf != std::string::npos) )
	{
		// Ensure we have enough data for both halves
		secondHalf -= 248;
		if ( (firstHalf + 256 <= buffer.size()) && (secondHalf + 256 <= buffer.size()) )
		{
			if (reading)
			{				
				memcpy(&saveFile, &buffer[firstHalf], 256);										// Copy the first 256 bytes starting from "SAVESTRT"				
				memcpy(reinterpret_cast<char*>(&saveFile) + 256, &buffer[secondHalf], 256);	// Copy the last 256 bytes ending with "SAVE-END"
			}
			else
			{
				memcpy(&buffer[firstHalf], &saveFile, 256);										// Copy the first 256 bytes starting from "SAVESTRT"				
				memcpy(&buffer[secondHalf], reinterpret_cast<char*>(&saveFile) + 256,  256);	// Copy the last 256 bytes ending with "SAVE-END"
			}
			return true;
		}
		else
		{
			MessageBox(NULL, TEXT("Not enough data for both halves"), TEXT("Error"), MB_OK | MB_ICONERROR);
		}
	}
	else
	{
		MessageBox(NULL, TEXT("Markers not found or invalid"), TEXT("Error"), MB_OK | MB_ICONERROR);
	}
	return false;
}




bool LoadSaveSlotFile(dsk_file::SaveGameFile& saveFile)
{
	bool isOk = false;
	HANDLE hFile = CreateFile(GetDskSaveSlotFilePath(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD fileSize = GetFileSize(hFile, NULL);
		if (fileSize != sizeof(saveFile))
		{
			// Not the right size, something's wrong
			return false;

		}
		DWORD bytesRead;
		isOk = ReadFile(hFile, &saveFile, fileSize, &bytesRead, NULL);
		CloseHandle(hFile);
	}
	return isOk;
}


bool SaveSaveSlotFile(dsk_file::SaveGameFile& saveFile)
{
	bool isOk = false;
	HANDLE hFile = CreateFile(GetDskSaveSlotFilePath(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD bytesWritten;
		isOk  = WriteFile(hFile, &saveFile, sizeof(saveFile), &bytesWritten, NULL);
		isOk &= (bytesWritten != sizeof(saveFile));
		CloseHandle(hFile);
	}
	return isOk;
}



bool ReadSaveSlotFromDsk(dsk_file::SaveGameFile& saveFile)
{
	bool isOk = false;
	HANDLE hFile = CreateFile(g_DskFilePath,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD fileSize = GetFileSize(hFile, NULL);
		std::vector<char> buffer(fileSize);

		DWORD bytesRead;
		if (ReadFile(hFile, buffer.data(), fileSize, &bytesRead, NULL))
		{
			// Extract the block of memory between "SAVESTRT" and "SAVE-END"
			isOk = ExtractMemoryBlock(buffer, saveFile, true);
		}
		CloseHandle(hFile);
	}
	return isOk;
}


bool WriteSaveSlotToDsk(dsk_file::SaveGameFile& saveFile)
{
	bool isOk = false;
	HANDLE hFile = CreateFile(g_DskFilePath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD fileSize = GetFileSize(hFile, NULL);
		std::vector<char> buffer(fileSize);

		DWORD bytesRead;
		if (ReadFile(hFile, buffer.data(), fileSize, &bytesRead, NULL))
		{
			// Extract the block of memory between "SAVESTRT" and "SAVE-END"
			if (ExtractMemoryBlock(buffer, saveFile, false))
			{
				// Move the file pointer to the beginning of the file
				SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

				DWORD bytesWritten;
				if (WriteFile(hFile, buffer.data(), fileSize, &bytesWritten, NULL))
				{
					isOk = true;
				}

			}
		}
		CloseHandle(hFile);
	}
	return isOk;
}


void PatchDskFile(HWND hDlg)
{
	// Note: We should probably do some version checking at some point to make sure the versions are compatible.
	// But at this point the format has not changed.
	dsk_file::SaveGameFile dskSaveFile = { 0 };
	if (ReadSaveSlotFromDsk(dskSaveFile))
	{
		// If available, load the local save
		dsk_file::SaveGameFile userSaveFile = { 0 };
		if (!LoadSaveSlotFile(userSaveFile))
		{
			// Else we load the content of the DSK
			userSaveFile = dskSaveFile;
		}
		else
		{
			// We may want to reset the content in some situations.
			if ( (dskSaveFile.file_version >= dsk_file::Version(0, 9, 2)) && (userSaveFile.file_version <= dsk_file::Version(0, 9, 1)) )
			{
				// Ignore the player existing save file and overwrite it with the DSK one
				userSaveFile = dskSaveFile;
				SteamManager::ClearAllAchievements();
			}
		}

		// Then we patch the options from the UI 
		userSaveFile.keyboard_layout = IsDlgButtonChecked(hDlg, IDC_RADIO_LayoutQwerty) == BST_CHECKED ? KEYBOARD_QWERTY : IsDlgButtonChecked(hDlg, IDC_RADIO_LayoutQwertz) == BST_CHECKED ? KEYBOARD_QWERTZ : KEYBOARD_AZERTY;
		userSaveFile.music_enabled   = IsDlgButtonChecked(hDlg, IDC_CHECK_MUSIC) == BST_CHECKED ? 1 : 0;
		userSaveFile.sound_enabled   = IsDlgButtonChecked(hDlg, IDC_CHECK_SOUNDS) == BST_CHECKED ? 1 : 0;
		WriteSaveSlotToDsk(userSaveFile);
	}
}

