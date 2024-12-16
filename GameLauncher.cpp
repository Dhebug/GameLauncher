

#include "common.h"
#include "resource.h"

#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Comctl32.lib")


HWND g_DialogHandle = 0;                  ///< Handle of the Launcher dialog



LRESULT CALLBACK StaticControlWithCustomCursor(HWND handle, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
{
	switch (uMsg)
	{
	case WM_SETCURSOR:
	{
		// Because IDC_HAND is not available on all operating systems, we will load the arrow cursor if IDC_HAND is not present.
		HCURSOR cursorHandle = LoadCursor(NULL, IDC_HAND);
		if (!cursorHandle)
		{
			cursorHandle = LoadCursor(NULL, IDC_ARROW);
		}
		SetCursor(cursorHandle);
		return TRUE;
	}
	break;

	case WM_NCDESTROY:
		RemoveWindowSubclass(handle, StaticControlWithCustomCursor, 0);
		// fall through
	default:
		return DefSubclassProc(handle, uMsg, wParam, lParam);
	}
}




INT_PTR CALLBACK MessageHandler(HWND dialogHandle, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		{
			bool controlPressed = !!(GetKeyState(VK_CONTROL) & 0x8000);

			g_DialogHandle = dialogHandle;

			LoadSettings(dialogHandle, controlPressed);

			RECT dialogRectangle = { 0 };

			 
			bool hasStoredPosition = false;
			if ( (!controlPressed) && (IsDlgButtonChecked(g_DialogHandle, IDC_CHECK_REMEMBER_POSITIONS) == BST_CHECKED) )
			{
				hasStoredPosition = LoadLauncherPosition(dialogRectangle);
			}

			int xPos;
			int yPos;

			// We only use the position stored in the settings if the CTRL key is not pressed
			if (hasStoredPosition)
			{
				// Reuse the saved values
				xPos = dialogRectangle.left;
				yPos = dialogRectangle.top;
			}
			else
			{
				// Center the dialog
				// Get the dialog's rectangle
				GetWindowRect(dialogHandle, &dialogRectangle);
				int dlgWidth  = dialogRectangle.right - dialogRectangle.left;
				int dlgHeight = dialogRectangle.bottom - dialogRectangle.top;

				// Get the screen's rectangle
				RECT desktopRectangle;
				GetWindowRect(GetDesktopWindow(), &desktopRectangle);

				// Calculate the position to center the dialog on the screen
				xPos = (desktopRectangle.right - dlgWidth) / 2;
				yPos = (desktopRectangle.bottom - dlgHeight) / 2;
			}

			// Set the dialog's position
			SetWindowPos(dialogHandle, HWND_TOP, xPos, yPos, 0, 0, SWP_NOSIZE);

			// Set initial dialog caption and button caption
			UpdateDialogRunStopStatus(dialogHandle, false);

			// If we want to control the shape of the mouse cursor when it hovers on specific items, we need to subclass and have a custom message handler for these elements
			SetWindowSubclass(GetDlgItem(dialogHandle, IDC_LINK_HOMEPAGE), StaticControlWithCustomCursor, 0, 0);
			SetWindowSubclass(GetDlgItem(dialogHandle, IDC_LINK_MANUAL), StaticControlWithCustomCursor, 0, 0);
			SetWindowSubclass(GetDlgItem(dialogHandle, IDC_LINK_SUPPORT), StaticControlWithCustomCursor, 0, 0);

		}
		return (INT_PTR)TRUE;

	case WM_CTLCOLORSTATIC:
	{
		// Set the colour of the text for our URL (at the dialog creation, cannot be changed later with this method)
		// See: https://stackoverflow.com/questions/1525669/set-static-text-color-win32
		// https://docs.microsoft.com/en-us/windows/desktop/Controls/wm-ctlcolorstatic
		if ( ((HWND)lParam == GetDlgItem(dialogHandle, IDC_LINK_HOMEPAGE)) ||
			 ((HWND)lParam == GetDlgItem(dialogHandle, IDC_LINK_MANUAL)) ||
			 ((HWND)lParam == GetDlgItem(dialogHandle, IDC_LINK_SUPPORT)))
		{
			HDC dc = (HDC)wParam;
			SetBkMode(dc, TRANSPARENT);
			SetTextColor(dc, RGB(0, 0, 255));
			return (INT_PTR)GetSysColorBrush(COLOR_MENU);   // By returning a system brush, we don't have to bother with releasing or leaking a brush
		}
		break;
	}

	case WM_MOVE:
	case WM_SIZE:
	{
		RECT dialogRectangle;
		GetWindowRect(dialogHandle, &dialogRectangle);
		SaveLauncherPosition(dialogRectangle);
		return TRUE;
	}

	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case IDC_RADIO_English:
			case IDC_RADIO_French:
				SetDialogLanguage(dialogHandle);
				break;

			case ID_QUIT:
				WriteSettings(dialogHandle);
				TerminateEmulator();
				EndDialog(dialogHandle, LOWORD(wParam));
				return (INT_PTR)TRUE;

			case ID_LAUNCH_STOP:
				return LaunchStopClicked(dialogHandle);

			case IDC_LINK_HOMEPAGE:  // Open the game homepage
				ShellExecute(dialogHandle, L"open", GetHyperlink(HyperlinkGamePage), 0, 0, SW_SHOWNORMAL);
				return TRUE;

			case IDC_LINK_MANUAL:   // Open the game user manual
				ShellExecute(dialogHandle, L"open", GetHyperlink(HyperlinkManual), 0, 0, SW_SHOWNORMAL);
				return TRUE;

			case IDC_LINK_SUPPORT:  // Open a email link
				ShellExecute(dialogHandle, L"open", GetHyperlink(HyperlinkSupport), 0, 0, SW_SHOWNORMAL);
				return TRUE;
			}
		}
		break;
	}
	return (INT_PTR)FALSE;
}





int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPWSTR /*lpCmdLine*/, int /*nCmdShow*/)
{
	// Create a named mutex to check if the launcher has not been launched multiple times
	HANDLE mutexHandle = CreateMutex(NULL, TRUE, L"DefenceForceGameLauncherMutex");
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		// Mutex already exists, another instance is running
		MessageBox(NULL, L"Another Game Launcher instance is already running.", L"Encounter Launcher", MB_OK | MB_ICONEXCLAMATION);
		return 0; // Exit the application
	}

#if 0
	{
		// Poor man's unit tests
		dsk_file::version v091(0, 9, 1);
		dsk_file::version v092(0, 9, 2);
		dsk_file::version v100(1, 0, 0);
		bool isOk = true;

		isOk &= (v091 == v091);
		isOk &= (v092 == v092);
		isOk &= (v091 != v092);
		isOk &= (v092 != v091);

		isOk &= (v091 < v092);
		isOk &= (v091 < v100);
	}
#endif

#ifdef STEAM_LAUNCHER
	SteamManager steamManager;
	if (!steamManager.Initialize())
	{
		return EXIT_FAILURE;
	}
#endif // STEAM_LAUNCHER

	DialogBox(hInstance, MAKEINTRESOURCE(IDD_LAUNCHER), 0, MessageHandler);

	ReleaseMutex(mutexHandle);
	CloseHandle(mutexHandle);
	return 0;
};

