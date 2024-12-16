
#include "common.h"

#include "resource.h"




PROCESS_INFORMATION g_pi = { 0 };

HWND g_EmulatorWindowHandle = 0;          ///< Handle of the Oricutron emulator which was launched
bool g_EmulatorWasLaunchedAndHasStopped = false;


bool IsProcessRunning(PROCESS_INFORMATION pi)
{
	DWORD exitCode;
	if (GetExitCodeProcess(pi.hProcess, &exitCode))
	{
		return exitCode == STILL_ACTIVE;
	}
	return false;
}

bool IsEmulatorRunning()
{
	return IsProcessRunning(g_pi);
}


BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM /*lParam*/)
{
	DWORD windowProcessId;
	GetWindowThreadProcessId(hwnd, &windowProcessId);

	if (windowProcessId == g_pi.dwProcessId)
	{
		g_EmulatorWindowHandle = hwnd;
		return FALSE; // Stop enumeration once we find the window
	}
	return TRUE; // Continue enumeration
}



unsigned __stdcall MonitorWindowPosition(void* /*pArguments*/)
{
	OutputDebugStringA("MonitorWindowPosition thread started\n");
	g_EmulatorWindowHandle = 0;

	// Find the window
	int counter = 10;
	do 
	{
		EnumWindows(EnumWindowsProc, NULL);
		if (!g_EmulatorWindowHandle)
		{
			Sleep(100);
		}
	} 
	while (!g_EmulatorWindowHandle && (counter--));

	// Change the emulator title from "Oricutron 1.2" to "Encounter"
	SetWindowText(g_EmulatorWindowHandle, _T("Encounter - Oricutron Emulator"));

	// Replace the default "Oric" icon by the Encounter one
	HICON iconHandle = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_GAMELAUNCHER));
	SendMessage(g_EmulatorWindowHandle, WM_SETICON, ICON_BIG, (LPARAM)iconHandle);
	SendMessage(g_EmulatorWindowHandle, WM_SETICON, ICON_SMALL, (LPARAM)iconHandle);

	RECT rect;
	if (LoadEmulatorPosition(rect) && (IsDlgButtonChecked(g_DialogHandle, IDC_CHECK_REMEMBER_POSITIONS) == BST_CHECKED))
	{
		// There was a previously saved window location, let's restore it.
		SetWindowPos(g_EmulatorWindowHandle, NULL, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER | SWP_NOACTIVATE);
	}

	while (IsEmulatorRunning())
	{
		if (GetWindowRect(g_EmulatorWindowHandle, &rect))
		{
			// Save the window position
			SaveEmulatorPosition(rect);
		}
		Sleep(1000); // Check every second
	}

	// Destroy the icon handle
	DestroyIcon(iconHandle);

	OutputDebugStringA("MonitorWindowPosition thread stopped\n");
	return 0;
}



unsigned __stdcall MonitorEmulator(void* /*pArguments*/)
{
	OutputDebugStringA("MonitorEmulator thread started\n");

	// First we start the monitoring thread
	_beginthreadex(NULL, 0, MonitorWindowPosition, NULL, 0, NULL);

	WaitForSingleObject(g_pi.hProcess, INFINITE);

	// Useful to determine if we want to quit after the emulator has ran it's course
	g_EmulatorWasLaunchedAndHasStopped = true;

	// Close process and thread handles
	CloseHandle(g_pi.hProcess);
	CloseHandle(g_pi.hThread);

	// Restore the window if the option is checked
	if (IsDlgButtonChecked(g_DialogHandle, IDC_CHECK_AUTO_MINIMIZE) == BST_CHECKED)
	{
		ShowWindow(g_DialogHandle, SW_RESTORE);
	}

	// Update the button caption and dialog title
	UpdateDialogRunStopStatus(g_DialogHandle, false);

	OutputDebugStringA("MonitorEmulator thread stopped\n");
	return 0;
}



bool FindFileWithPattern(LPCTSTR directory, LPCTSTR pattern, LPTSTR foundFile)
{
	WIN32_FIND_DATA findFileData;
	TCHAR searchPath[MAX_PATH];
	StringCchPrintf(searchPath, MAX_PATH, _T("%s\\%s"), directory, pattern);

	HANDLE hFind = FindFirstFile(searchPath, &findFileData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		return false; // No file found
	}
	else
	{
		StringCchPrintf(foundFile, MAX_PATH, _T("%s\\%s"), directory, findFileData.cFileName);
		FindClose(hFind);
		return true;
	}
}



FILETIME GetLastModifiedTime(LPCTSTR filePath)
{
	HANDLE hFile = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) 
	{
		return { 0, 0 };
	}

	FILETIME ftCreate, ftAccess, ftWrite;
	if (!GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite)) 
	{
		CloseHandle(hFile);
		return { 0, 0 };
	}

	CloseHandle(hFile);
	return ftWrite;
}


void CheckDskTime(FILETIME& referenceTime)
{
	FILETIME currentTime = GetLastModifiedTime(g_DskFilePath);
	if (CompareFileTime(&referenceTime, &currentTime) != 0)
	{
		OutputDebugStringA("DSK file modified\n");
		// Different time stamp
		referenceTime = currentTime;
		// File change detected, load the file content
		dsk_file::SaveGameFile saveFile = { 0 };
		if (ReadSaveSlotFromDsk(saveFile))
		{
			OutputDebugStringA("DSK file slot extracted\n");
			SaveSaveSlotFile(saveFile);
#ifdef STEAM_LAUNCHER
			SteamManager::SetOricAchievements(saveFile.achievements);
#endif
		}
	}
}


DWORD WINAPI MonitorFileChanges(LPVOID /*lpParam*/)
{
	OutputDebugStringA("MonitorFileChanges thread started\n");
	FILETIME originalTime = GetLastModifiedTime(g_DskFilePath);
	
	while (IsEmulatorRunning())
	{
		Sleep(500); // Wait for half a second
		CheckDskTime(originalTime);
	}
	OutputDebugStringA("MonitorFileChanges thread stopped\n");

	// Automaticaly quit the launcher when the play session is done if the option is checked
	if (IsDlgButtonChecked(g_DialogHandle, IDC_CHECK_QUIT_WHEN_DONE) == BST_CHECKED)
	{
		// Post a message to the main dialog to quit
		PostMessage(g_DialogHandle, WM_COMMAND, ID_QUIT, 0);
	}
	return 0;
}




INT_PTR LaunchStopClicked(HWND hDlg)
{
	if (!IsEmulatorRunning())
	{
		g_EmulatorWasLaunchedAndHasStopped = false;

		// Patch the DSK file with the new settings
		PatchDskFile(hDlg);

		// Define the path to the executable
		TCHAR szPath[MAX_PATH];
		GetModuleFileName(NULL, szPath, MAX_PATH);
		PathRemoveFileSpec(szPath);
		PathAppend(szPath, TEXT("emulator"));

		// Set the current directory to the executable's directory
		SetCurrentDirectory(szPath);

		PathAppend(szPath, TEXT("oricutron.exe"));

		// Build the command line parameters
		if (IsDlgButtonChecked(hDlg, IDC_RADIO_FullscreenMode) == BST_CHECKED)
		{
			_tcscat_s(szPath, TEXT(" -f"));
		}
		if (IsDlgButtonChecked(hDlg, IDC_RADIO_FullCrtFilter) == BST_CHECKED)
		{
			_tcscat_s(szPath, TEXT(" --scanlines on --rendermode opengl"));
		}
		if (IsDlgButtonChecked(hDlg, IDC_RADIO_CrtFilter) == BST_CHECKED)
		{
			_tcscat_s(szPath, TEXT(" --scanlines on"));
		}

		_tcscat_s(szPath, TEXT(" \"")); // Open quotes
		_tcscat_s(szPath, g_DskFilePath); // Use the resolved file name
		_tcscat_s(szPath, TEXT("\""));  // Close quotes

		// Set up the process information structure
		STARTUPINFO si = { sizeof(si) };

		// Create the process
		if (CreateProcess(NULL, szPath, NULL, NULL, FALSE, 0, NULL, NULL, &si, &g_pi))
		{
			// Update the button caption and dialog title
			UpdateDialogRunStopStatus(hDlg, true);

			// Create a thread to wait for the process
			_beginthreadex(NULL, 0, MonitorEmulator, NULL, 0, NULL);

			// Start the file monitoring thread
			CreateThread(NULL, 0, MonitorFileChanges, NULL, 0, NULL);
#ifdef STEAM_LAUNCHER
			SteamManager::SetAchievement(STEAMACH_WELCOME);
#endif
			// Minimize the window if the option is checked
			if (IsDlgButtonChecked(hDlg, IDC_CHECK_AUTO_MINIMIZE) == BST_CHECKED)
			{
				ShowWindow(hDlg, SW_MINIMIZE);
			}
		}
		else
		{
			MessageBox(hDlg, TEXT("Failed to launch oricutron.exe"), TEXT("Error"), MB_OK | MB_ICONERROR);
		}
	}
	else
	{
		// Terminate the process
		TerminateProcess(g_pi.hProcess, 0);
	}
	return (INT_PTR)TRUE;
}


// TODO: Should probably ask confirmation...
void TerminateEmulator()
{
	if (IsEmulatorRunning())
	{
		// Terminate the process
		TerminateProcess(g_pi.hProcess, 0);
		Sleep(500);                              // TODO: Ugly, need to make sure all threads are join, but these are Win32 threads
	}
}


