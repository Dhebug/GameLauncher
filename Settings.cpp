
#pragma once

#include "common.h"

#include "resource.h"



const TCHAR* GetSaveFolderPath()
{
	static TCHAR s_saveFolderPath[MAX_PATH];
	if (!s_saveFolderPath[0])
	{
		SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, s_saveFolderPath);
		PathAppend(s_saveFolderPath, _T("EncounterByDefenceForce"));
	}
	// Ensure the directory exists
	if (!CreateDirectory(s_saveFolderPath, NULL))
	{
		if (GetLastError() != ERROR_ALREADY_EXISTS)
		{
			// Handle the error
			_tprintf(_T("Failed to create directory: %s\n"), s_saveFolderPath);
			return nullptr;     // Should probably hard fail here
		}
	}
	return s_saveFolderPath;
}


const TCHAR* GetIniFilePath()
{
	static TCHAR s_iniFilePath[MAX_PATH];
	if (!s_iniFilePath[0])
	{
		_tcscpy_s(s_iniFilePath, MAX_PATH, GetSaveFolderPath());
		PathAppend(s_iniFilePath, _T("GameLauncherSettings.ini"));
	}
	return s_iniFilePath;
}


const TCHAR* GetDskSaveSlotFilePath()
{
	static TCHAR s_dskSaveSlotFilePath[MAX_PATH];
	if (!s_dskSaveSlotFilePath[0])
	{
		_tcscpy_s(s_dskSaveSlotFilePath, MAX_PATH, GetSaveFolderPath());
		PathAppend(s_dskSaveSlotFilePath, _T("DskSaveSlotFile.bin"));
	}
	return s_dskSaveSlotFilePath;
}


void SaveRectPosition(const TCHAR* sectionName, const RECT& rect)
{
	const TCHAR* iniFilePath(GetIniFilePath());

	// Convert RECT values to strings
	TCHAR left[16], top[16], right[16], bottom[16];
	_stprintf_s(left, _T("%d"), rect.left);
	_stprintf_s(top, _T("%d"), rect.top);
	_stprintf_s(right, _T("%d"), rect.right);
	_stprintf_s(bottom, _T("%d"), rect.bottom);

	// Write the window position to the INI file
	WritePrivateProfileString(sectionName, _T("WindowPositionLeft"), left, iniFilePath);
	WritePrivateProfileString(sectionName, _T("WindowPositionTop"), top, iniFilePath);
	WritePrivateProfileString(sectionName, _T("WindowPositionRight"), right, iniFilePath);
	WritePrivateProfileString(sectionName, _T("WindowPositionBottom"), bottom, iniFilePath);
}

bool LoadRectPosition(const TCHAR* sectionName,RECT& rect)
{
	const TCHAR* iniFilePath(GetIniFilePath());

	// Read the window position from the INI file
	int left   = GetPrivateProfileInt(sectionName, _T("WindowPositionLeft"), -1, iniFilePath);
	int top    = GetPrivateProfileInt(sectionName, _T("WindowPositionTop"), -1, iniFilePath);
	int right  = GetPrivateProfileInt(sectionName, _T("WindowPositionRight"), -1, iniFilePath);
	int bottom = GetPrivateProfileInt(sectionName, _T("WindowPositionBottom"), -1, iniFilePath);

	// Check if the values are different from the default values
	if (left == -1 || top == -1 || right == -1 || bottom == -1)
	{
		return false; // No existing position found
	}

	// Set the RECT values
	rect.left = left;
	rect.top = top;
	rect.right = right;
	rect.bottom = bottom;

	return true; // Existing position found
}


void SaveEmulatorPosition(const RECT& rect)
{
	SaveRectPosition(_T("Oricutron"), rect);
}

bool LoadEmulatorPosition(RECT& rect)
{
	return LoadRectPosition(_T("Oricutron"), rect);
}

void SaveLauncherPosition(const RECT& rect)
{
	SaveRectPosition(_T("Launcher"), rect);
}

bool LoadLauncherPosition(RECT& rect)
{
	return LoadRectPosition(_T("Launcher"), rect);
}




void WriteSettings(HWND hDlg)
{
	const TCHAR* iniFilePath(GetIniFilePath());
	WritePrivateProfileString(_T("Settings"), _T("WindowMode"), IsDlgButtonChecked(hDlg, IDC_RADIO_WindowedMode) == BST_CHECKED ? _T("Windowed") : _T("Fullscreen"), iniFilePath);
	WritePrivateProfileString(_T("Settings"), _T("Filter"), IsDlgButtonChecked(hDlg, IDC_RADIO_NoFilter) == BST_CHECKED ? _T("NoFilter") : IsDlgButtonChecked(hDlg, IDC_RADIO_CrtFilter) == BST_CHECKED ? _T("CrtFilter") : _T("FullCrtFilter"), iniFilePath);
	WritePrivateProfileString(_T("Settings"), _T("Language"), IsDlgButtonChecked(hDlg, IDC_RADIO_English) == BST_CHECKED ? _T("English") : _T("French"), iniFilePath);
	WritePrivateProfileString(_T("Settings"), _T("KeyboardLayout"), IsDlgButtonChecked(hDlg, IDC_RADIO_LayoutQwerty) == BST_CHECKED ? _T("Qwerty") : IsDlgButtonChecked(hDlg, IDC_RADIO_LayoutQwertz) == BST_CHECKED ? _T("Qwertz") : _T("Azerty"), iniFilePath);
	WritePrivateProfileString(_T("Settings"), _T("Music"), IsDlgButtonChecked(hDlg, IDC_CHECK_MUSIC) == BST_CHECKED ? _T("Enabled") : _T("Disabled"), iniFilePath);
	WritePrivateProfileString(_T("Settings"), _T("Sounds"), IsDlgButtonChecked(hDlg, IDC_CHECK_SOUNDS) == BST_CHECKED ? _T("Enabled") : _T("Disabled"), iniFilePath);
	WritePrivateProfileString(_T("Settings"), _T("AutoMinimize"), IsDlgButtonChecked(hDlg, IDC_CHECK_AUTO_MINIMIZE) == BST_CHECKED ? _T("Enabled") : _T("Disabled"), iniFilePath);
	WritePrivateProfileString(_T("Settings"), _T("AutoQuit"), IsDlgButtonChecked(hDlg, IDC_CHECK_QUIT_WHEN_DONE) == BST_CHECKED ? _T("Enabled") : _T("Disabled"), iniFilePath);
	WritePrivateProfileString(_T("Settings"), _T("RememberPositions"), IsDlgButtonChecked(hDlg, IDC_CHECK_REMEMBER_POSITIONS) == BST_CHECKED ? _T("Enabled") : _T("Disabled"), iniFilePath);
}


void LoadSettings(HWND hDlg,bool loadDefault)
{
	const TCHAR* iniFilePath(loadDefault?_T(""):GetIniFilePath());

	TCHAR buffer[256];

	GetPrivateProfileString(_T("Settings"), _T("WindowMode"), _T("Windowed"), buffer, sizeof(buffer) / sizeof(buffer[0]), iniFilePath);
	CheckRadioButton(hDlg, IDC_RADIO_WindowedMode, IDC_RADIO_FullscreenMode, _tcscmp(buffer, _T("Windowed")) == 0 ? IDC_RADIO_WindowedMode : IDC_RADIO_FullscreenMode);

	GetPrivateProfileString(_T("Settings"), _T("Filter"), _T("CrtFilter"), buffer, sizeof(buffer) / sizeof(buffer[0]), iniFilePath);
	CheckRadioButton(hDlg, IDC_RADIO_NoFilter, IDC_RADIO_FullCrtFilter, _tcscmp(buffer, _T("NoFilter")) == 0 ? IDC_RADIO_NoFilter : _tcscmp(buffer, _T("CrtFilter")) == 0 ? IDC_RADIO_CrtFilter : IDC_RADIO_FullCrtFilter);

	GetPrivateProfileString(_T("Settings"), _T("Language"), _T("English"), buffer, sizeof(buffer) / sizeof(buffer[0]), iniFilePath);
	CheckRadioButton(hDlg, IDC_RADIO_English, IDC_RADIO_French, _tcscmp(buffer, _T("English")) == 0 ? IDC_RADIO_English : IDC_RADIO_French);

	GetPrivateProfileString(_T("Settings"), _T("KeyboardLayout"), _T("Qwerty"), buffer, sizeof(buffer) / sizeof(buffer[0]), iniFilePath);
	CheckRadioButton(hDlg, IDC_RADIO_LayoutQwerty, IDC_RADIO_LayoutAzerty, _tcscmp(buffer, _T("Qwerty")) == 0 ? IDC_RADIO_LayoutQwerty : _tcscmp(buffer, _T("Qwertz")) == 0 ? IDC_RADIO_LayoutQwertz : IDC_RADIO_LayoutAzerty);

	GetPrivateProfileString(_T("Settings"), _T("Music"), _T("Enabled"), buffer, sizeof(buffer) / sizeof(buffer[0]), iniFilePath);
	CheckDlgButton(hDlg, IDC_CHECK_MUSIC, _tcscmp(buffer, _T("Enabled")) == 0 ? BST_CHECKED : BST_UNCHECKED);

	GetPrivateProfileString(_T("Settings"), _T("Sounds"), _T("Enabled"), buffer, sizeof(buffer) / sizeof(buffer[0]), iniFilePath);
	CheckDlgButton(hDlg, IDC_CHECK_SOUNDS, _tcscmp(buffer, _T("Enabled")) == 0 ? BST_CHECKED : BST_UNCHECKED);

	GetPrivateProfileString(_T("Settings"), _T("AutoMinimize"), _T("Enabled"), buffer, sizeof(buffer) / sizeof(buffer[0]), iniFilePath);
	CheckDlgButton(hDlg, IDC_CHECK_AUTO_MINIMIZE, _tcscmp(buffer, _T("Enabled")) == 0 ? BST_CHECKED : BST_UNCHECKED);

	GetPrivateProfileString(_T("Settings"), _T("AutoQuit"), _T("Enabled"), buffer, sizeof(buffer) / sizeof(buffer[0]), iniFilePath);
	CheckDlgButton(hDlg, IDC_CHECK_QUIT_WHEN_DONE, _tcscmp(buffer, _T("Enabled")) == 0 ? BST_CHECKED : BST_UNCHECKED);

	GetPrivateProfileString(_T("Settings"), _T("RememberPositions"), _T("Enabled"), buffer, sizeof(buffer) / sizeof(buffer[0]), iniFilePath);
	CheckDlgButton(hDlg, IDC_CHECK_REMEMBER_POSITIONS, _tcscmp(buffer, _T("Enabled")) == 0 ? BST_CHECKED : BST_UNCHECKED);

	SetDialogLanguage(hDlg);
}





const TCHAR* GetLocalizedString(int stringId)
{
	static HINSTANCE s_instance = GetModuleHandle(NULL);
	static TCHAR s_buffer[256];
	LoadString(s_instance, stringId, s_buffer, sizeof(s_buffer) / sizeof(s_buffer[0]));
	return s_buffer;
}


void UpdateDialogRunStopStatus(HWND hDlg, bool isRunning)
{
	if (isRunning)
	{
		SetWindowText(hDlg, GetLocalizedString(IDS_EMULATOR_RUNNING));
		SetDlgItemText(hDlg, ID_LAUNCH_STOP, GetLocalizedString(IDS_STOP));
	}
	else
	{
		SetWindowText(hDlg, GetLocalizedString(IDS_EMULATOR_STOPPED));
		SetDlgItemText(hDlg, ID_LAUNCH_STOP, GetLocalizedString(IDS_LAUNCH));
	}
}


void SetDialogLanguage(HWND hDlg)
{
	// Handle language change
	LANGID langID = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
	if (IsDlgButtonChecked(hDlg, IDC_RADIO_French) == BST_CHECKED)
	{
		// French selected
		langID = MAKELANGID(LANG_FRENCH, SUBLANG_FRENCH);
		SetDiskLanguage(_T("FR"));
	}
	else
	{
		SetDiskLanguage(_T("EN"));
	}
	SetThreadUILanguage(langID);

	struct {
		int controlId;
		int stringId;
	} controls[] =
	{
		{ IDC_LANGUAGE, IDS_Language },
		{ IDC_RADIO_English, IDS_English },
		{ IDC_RADIO_French, IDS_French },

		{ IDC_DISPLAY, IDS_DISPLAY },
		{ IDC_RADIO_WindowedMode, IDS_WindowedMode},
		{ IDC_RADIO_FullscreenMode, IDS_FullScreenMode},

		{ IDC_FILTERS, IDS_FILTERS },
		{ IDC_RADIO_NoFilter, IDS_NoFilter},
		{ IDC_RADIO_CrtFilter, IDS_CrtFilter},
		{ IDC_RADIO_FullCrtFilter, IDS_FullCrtFilter},

		{ IDC_KEYBOARD_LAYOUT, IDS_KEYBOARD_LAYOUT },
		{ IDC_RADIO_LayoutQwerty, IDS_RADIO_LayoutQwerty},
		{ IDC_RADIO_LayoutQwertz, IDS_RADIO_LayoutQwertz},
		{ IDC_RADIO_LayoutAzerty, IDS_RADIO_LayoutAzerty},

		{ IDC_AUDIO, IDS_AUDIO },
		{ IDC_CHECK_MUSIC, IDS_CHECK_MUSIC},
		{ IDC_CHECK_SOUNDS, IDS_CHECK_SOUNDS},

		{ IDC_LAUNCHER, IDS_LAUNCHER },
		{ IDC_CHECK_AUTO_MINIMIZE, IDS_CHECK_AUTO_MINIMIZE},
		{ IDC_CHECK_QUIT_WHEN_DONE, IDS_CHECK_QUIT_WHEN_DONE},
		{ IDC_CHECK_REMEMBER_POSITIONS, IDS_CHECK_REMEMBER_POSITIONS},

		{ ID_LAUNCH_STOP, IDS_LAUNCH},
		{ ID_QUIT, IDS_QUIT}
	};

	for (int i = 0; i < sizeof(controls) / sizeof(controls[0]); ++i)
	{
		SetDlgItemText(hDlg, controls[i].controlId, GetLocalizedString(controls[i].stringId));
	}
	UpdateDialogRunStopStatus(hDlg, IsEmulatorRunning());
}

