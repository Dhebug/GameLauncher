

#include "common.h"
#include "resource.h"

#pragma comment(lib, "Shlwapi.lib")



HWND g_DialogHandle = 0;                  ///< Handle of the Launcher dialog



INT_PTR CALLBACK MessageHandler(HWND dialogHandle, UINT message, WPARAM wParam, LPARAM /*lParam*/)
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
		}
		return (INT_PTR)TRUE;

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

	SteamManager steamManager;
	if (!steamManager.Initialize())
	{
		return EXIT_FAILURE;
	}

	DialogBox(hInstance, MAKEINTRESOURCE(IDD_LAUNCHER), 0, MessageHandler);

	ReleaseMutex(mutexHandle);
	CloseHandle(mutexHandle);
	return 0;
};

