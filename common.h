// header.h : include file for standard system include files,
// or project specific include files
//

#pragma once
#define _CRT_SECURE_NO_WARNINGS


#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <atomic>
#include <thread>
#include <cassert>

#include <windows.h>
#include <shellapi.h>   // For ShellExecute
#include <Shlwapi.h>	// For PathRemoveFileSpec and PathAppend
#include <ShlObj.h>     // For SHGetFolderPath
#include <process.h>	// For _beginthreadex
#include <tchar.h>
#include <vector>
#include <string>
#include <cstring> // For memcpy
#include <tchar.h>
#include <strsafe.h>


//
// This copy-pasted manifest enables the version 6 of Common Controls:
// - Enable modern style (flat instead of 3D)
// - Allow 32 bit transparent bitmaps in static controls
//
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif


//
// Globals
//
extern HWND g_DialogHandle;						///< Handle of the Launcher dialog
extern HWND g_EmulatorWindowHandle;				///< Handle of the Oricutron emulator which was launched
extern bool g_EmulatorWasLaunchedAndHasStopped; ///< To detect if we want to quit or not


//
// Settings
//
const TCHAR* GetSaveFolderPath();            ///< C:\Users\<username>\AppData\Local\EncounterByDefenceForce
const TCHAR* GetIniFilePath();               ///< C:\Users\<username>\AppData\Local\EncounterByDefenceForce\GameLauncherSettings.ini
const TCHAR* GetDskSaveSlotFilePath();       ///< C:\Users\<username>\AppData\Local\EncounterByDefenceForce\DskSaveSlotFile.bin

enum Hyperlink
{
	HyperlinkGamePage,
	HyperlinkManual,
	HyperlinkSupport
};

const TCHAR* GetHyperlink(Hyperlink linkType);

void SaveEmulatorPosition(const RECT& rect);
bool LoadEmulatorPosition(RECT& rect);

void SaveLauncherPosition(const RECT& rect);
bool LoadLauncherPosition(RECT& rect);

void WriteSettings(HWND hDlg);
void LoadSettings(HWND hDlg, bool loadDefault);

void UpdateDialogRunStopStatus(HWND hDlg, bool isRunning);
void SetDialogLanguage(HWND hDlg);

const TCHAR* GetLocalizedString(int stringId);

//
// Disk Manager
//
#include "D:\Git\Encounter\code\game_enums.h"


namespace dsk_file
{

/*
;
 ; Each entry occupies 19 bytes:
 ;  2 bytes for the score (+32768)
 ;  1 byte for the game ending condition
 ;  1 byte to define if the score was pre-made or made by a player
 ; 15 bytes for the name (padded with spaces)
 ;-------------------------------------------
 ; 18 bytes per entry * 24 entries = 432 bytes total - But we save 512 because of the save system
*/
#define KEYBOARD_QWERTY 0
#define KEYBOARD_AZERTY 1
#define KEYBOARD_QWERTZ 2

#define	ACHIEVEMENT_COUNT_ 				48      //  Can't have more than 48 achievements
#define ACHIEVEMENT_BYTE_COUNT           6
#define SCORE_COUNT  24

#pragma pack(push, 1)
struct ScoreEntry
{
	int16_t         score;          // The score can actually be negative if the player is doing stupid things on purpose (plus or minus 32768 because of assembler reasons)
	uint8_t         condition;      // The reason why the game ended (victory, abandon, death, ...)
	uint8_t         player_score;   // 0 = initial score, 1 = score made by an actual player
	uint8_t         name[15];       // The name of the character  
};
#pragma pack(pop)
static_assert(sizeof(ScoreEntry) == 19, "Size of score_entry should be 19 bytes");


#pragma pack(push, 1)
struct Version
{
	Version() : array{'0', '.', '0', '.', '0' }  {}
	Version(uint8_t high, uint8_t mid, uint8_t low) : array{ uint8_t('0'+high), '.', uint8_t('0' + mid) , '.', uint8_t('0' + low) } {}

	int Compare(const Version& v2) const 
	{ 
		return std::memcmp(array, v2.array, sizeof(array)); 
	}
	uint8_t array[5];          // 1.2.3
};                      // sizeof(save_game_file)=5
#pragma pack(pop)

inline bool operator==(const Version& v1, const Version& v2) { return v1.Compare(v2) == 0; }
inline bool operator<(const Version& v1, const Version& v2)  { return v1.Compare(v2) < 0; }
inline bool operator>(const Version& v1, const Version& v2)  { return v1.Compare(v2) > 0; }
inline bool operator!=(const Version& v1, const Version& v2) { return !(v1==v2); }
inline bool operator<=(const Version& v1, const Version& v2) { return !(v1>v2); }
inline bool operator>=(const Version& v1, const Version& v2) { return !(v1<v2); }

static_assert(sizeof(Version) == 5, "Size of score_entry should be 5 bytes");



#pragma pack(push, 1)
struct SaveGameFile
{
	uint8_t start_marker[8];
	Version file_version;          // 1.2.3
	ScoreEntry scores[SCORE_COUNT];   // 18*24=432
	uint8_t achievements[ACHIEVEMENT_BYTE_COUNT];     // Enough for 6*8=48 achievements
	uint8_t free_data[56 - 4 - ACHIEVEMENT_BYTE_COUNT - 8 - 5 - 8];
	uint8_t keyboard_layout;
	uint8_t music_enabled;
	uint8_t sound_enabled;
	uint8_t launchCount;
	uint8_t end_marker[8];
};                      // sizeof(save_game_file)=512
#pragma pack(pop)
static_assert(sizeof(SaveGameFile) == 512, "Size of save_game_file should be 512 bytes");

} // namespace dsk_file


extern TCHAR g_DskFilePath[MAX_PATH];

void SetDiskLanguage(const TCHAR* languagePrefix);
bool ReadSaveSlotFromDsk(dsk_file::SaveGameFile& saveFile);
bool WriteSaveSlotToDsk(dsk_file::SaveGameFile& saveFile);
void PatchDskFile(HWND hDlg);

bool LoadSaveSlotFile(dsk_file::SaveGameFile& saveFile);
bool SaveSaveSlotFile(dsk_file::SaveGameFile& saveFile);


//
// Emulator Monitor
//
DWORD WINAPI MonitorFileChanges(LPVOID lpParam);
INT_PTR LaunchStopClicked(HWND hDlg);
void TerminateEmulator();
bool IsEmulatorRunning();


#ifdef STEAM_LAUNCHER

//
// Steam
// See: https://partner.steamgames.com/doc/features/achievements/ach_guide
//
#pragma warning(push)
#pragma warning(disable: 4996) // Disable warning C4996: 'strncpy': This function or variable may be unsafe. Consider using strncpy_s instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.
#include "C:\Projects\Encounter\steamworks_sdk_160\sdk\public\steam\steam_api.h"
#pragma warning(pop)
#pragma comment(lib, "C:/Projects/Encounter/steamworks_sdk_160/sdk/redistributable_bin/win64/steam_api64.lib")


enum Achievements
{
	STEAMACH_WELCOME = 0
	// These are the Oric achievements
	,_FIRST_ORIC_ACHIEVEMENT_
	// 0 to 7
	,STEAMACH_SOLVED_THE_CASE = _FIRST_ORIC_ACHIEVEMENT_
	,STEAMACH_MAIMED_BY_DOG
	,STEAMACH_SHOT_BY_THUG
	,STEAMACH_FELL_INTO_PIT
	,STEAMACH_TRIPPED_ALARM
	,STEAMACH_RAN_OUT_OF_TIME
	,STEAMACH_BLOWN_INTO_BITS
	,STEAMACH_GAVE_UP
	// 8 to 15
	,STEAMACH_WRONG_DIRECTION
	,STEAMACH_LAUNCHED_THE_GAME
	,STEAMACH_WATCHED_THE_INTRO
	,STEAMACH_READ_THE_NEWSPAPER
	,STEAMACH_READ_THE_BOOK
	,STEAMACH_READ_THE_NOTE
	,STEAMACH_READ_THE_RECIPES
	,STEAMACH_OPENED_THE_FRIDGE
	// 16 to 23
	,STEAMACH_OPENED_THE_CABINET
	,STEAMACH_DRUGGED_THE_MEAT
	,STEAMACH_KILLED_THE_DOG
	,STEAMACH_DRUGGED_THE_DOG
	,STEAMACH_CHASED_THE_DOG
	,STEAMACH_KILLED_THE_THUG
	,STEAMACH_DRUGGED_THE_THUG
	,STEAMACH_CAPTURED_THE_DOVE
	// 24 to 31
	,STEAMACH_USED_THE_ROPE
	,STEAMACH_USED_THE_LADDER
	,STEAMACH_EXAMINED_THE_MAP
	,STEAMACH_EXAMINED_THE_GAME
	,STEAMACH_OPENED_THE_SAFE
	,STEAMACH_OPENED_THE_PANEL
	,STEAMACH_BUILT_A_FUSE
	,STEAMACH_BUILT_A_BOMB
	// 32 to 39
	,STEAMACH_MADE_BLACK_POWDER
	,STEAMACH_FRISKED_THE_THUG
	,STEAMACH_USED_THE_ACID
	,STEAMACH_CAN_YOU_REPEAT
	,STEAMACH_LETS_PAUSE
	,STEAMACH_OPENED_THE_CURTAIN
	,STEAMACH_GAVE_THE_KNIFE
	,STEAMACH_GAVE_THE_ROPE
	// 40 to 47
	,STEAMACH_WATCHED_THE_OUTRO
	,STEAMACH_GOT_A_HIGHSCORE
	,STEAMACH_GOT_THE_BEST_SCORE
	,STEAMACH_DOG_ATE_THE_MEAT
	,STEAMACH_USED_HOSE
	,STEAMACH_CLOSED_THE_FRIDGE
	,STEAMACH_READ_INVOICE
	,STEAMACH_READ_TOMBSTONE
};

#define _ACH_ID( id, name ) { 1+id, false, #id,  name }
struct Achievement
{
	void GetSteamStatus()
	{
		SteamUserStats()->GetAchievement(m_ApiId, &m_Achieved);
		_snprintf(m_Name, sizeof(m_Name), "%s", SteamUserStats()->GetAchievementDisplayAttribute(m_ApiId, "name"));
		_snprintf(m_Description, sizeof(m_Description), "%s", SteamUserStats()->GetAchievementDisplayAttribute(m_ApiId, "desc"));

	}

	int m_EnumId;
	bool m_Achieved = false;
	const char* m_ApiId;
	char m_Name[128];
	char m_Description[256] = "";
};


class SteamAchievements
{
public:
	SteamAchievements();
	~SteamAchievements();

	bool RequestStats();
	bool SetAchievement(Achievements achievement);
	bool SetOricAchievements(uint8_t achievements[ACHIEVEMENT_BYTE_COUNT]);
	bool ClearAllAchievements();

	STEAM_CALLBACK(SteamAchievements, OnUserStatsReceived, UserStatsReceived_t		, m_CallbackUserStatsReceived);
	STEAM_CALLBACK(SteamAchievements, OnUserStatsStored, UserStatsStored_t			, m_CallbackUserStatsStored);
	STEAM_CALLBACK(SteamAchievements, OnAchievementStored, UserAchievementStored_t	, m_CallbackAchievementStored);

private:
	bool	m_IsInitialized = false;			// Have we called Request stats and received the callback?
	uint64	m_AppID = 0;						// Our current AppID

	// Achievement array which will hold data about the achievements and their state
	std::vector<Achievement>  m_Achievements =
	{
		_ACH_ID(STEAMACH_WELCOME		, "Welcome to Oric")
		// 0 to 7
		,_ACH_ID(STEAMACH_SOLVED_THE_CASE, "Case closed")
		,_ACH_ID(STEAMACH_MAIMED_BY_DOG	, "Beware of dog")
		,_ACH_ID(STEAMACH_SHOT_BY_THUG	, "Beware of owner")
		,_ACH_ID(STEAMACH_FELL_INTO_PIT	, "Beware of holes")
		,_ACH_ID(STEAMACH_TRIPPED_ALARM	, "Avoid the bell")
		,_ACH_ID(STEAMACH_RAN_OUT_OF_TIME, "The hourglass")
		,_ACH_ID(STEAMACH_BLOWN_INTO_BITS, "Explosives")
		,_ACH_ID(STEAMACH_GAVE_UP        , "Surrendered")
		// 8 to 15
		,_ACH_ID(STEAMACH_WRONG_DIRECTION , "Where am I?")
		,_ACH_ID(STEAMACH_LAUNCHED_THE_GAME , "Launched the game!")
		,_ACH_ID(STEAMACH_WATCHED_THE_INTRO , "Watched the intro")
		,_ACH_ID(STEAMACH_READ_THE_NEWSPAPER , "Read the newspaper")
		,_ACH_ID(STEAMACH_READ_THE_BOOK , "Read the book")
		,_ACH_ID(STEAMACH_READ_THE_NOTE , "Read the note")
		,_ACH_ID(STEAMACH_READ_THE_RECIPES , "Read the recipes")
		,_ACH_ID(STEAMACH_OPENED_THE_FRIDGE , "Opened the fridge")
		// 16 to 23
		,_ACH_ID(STEAMACH_OPENED_THE_CABINET , "Opened the cabinet")
		,_ACH_ID(STEAMACH_DRUGGED_THE_MEAT , "Drugged the meat")
		,_ACH_ID(STEAMACH_KILLED_THE_DOG , "Killed the dog")
		,_ACH_ID(STEAMACH_DRUGGED_THE_DOG , "Drugged the dog")
		,_ACH_ID(STEAMACH_CHASED_THE_DOG , "Chased away the dog")
		,_ACH_ID(STEAMACH_KILLED_THE_THUG , "Killed the thug")
		,_ACH_ID(STEAMACH_DRUGGED_THE_THUG , "Drugged the thug")
		,_ACH_ID(STEAMACH_CAPTURED_THE_DOVE , "Caught the dove")
		// 24 to 31
		,_ACH_ID(STEAMACH_USED_THE_ROPE , "Used the rope")
		,_ACH_ID(STEAMACH_USED_THE_LADDER , "Used the ladder")
		,_ACH_ID(STEAMACH_EXAMINED_THE_MAP , "Examined the map")
		,_ACH_ID(STEAMACH_EXAMINED_THE_GAME , "Examined the game")
		,_ACH_ID(STEAMACH_OPENED_THE_SAFE , "Opened the safe")
		,_ACH_ID(STEAMACH_OPENED_THE_PANEL , "Opened the panel")
		,_ACH_ID(STEAMACH_BUILT_A_FUSE , "Built the fuse")
		,_ACH_ID(STEAMACH_BUILT_A_BOMB , "Built the bomb")
		// 32 to 39
		,_ACH_ID(STEAMACH_MADE_BLACK_POWDER , "Made black powder")
		,_ACH_ID(STEAMACH_FRISKED_THE_THUG , "Frisked the thug")
		,_ACH_ID(STEAMACH_USED_THE_ACID , "Used the acid")
		,_ACH_ID(STEAMACH_CAN_YOU_REPEAT , "Can you repeat?")
		,_ACH_ID(STEAMACH_LETS_PAUSE , "Let's pause")
		,_ACH_ID(STEAMACH_OPENED_THE_CURTAIN , "Opened the curtain")
		,_ACH_ID(STEAMACH_GAVE_THE_KNIFE , "Gave the knife")
		,_ACH_ID(STEAMACH_GAVE_THE_ROPE , "Gave the rope")
		// 40 to 47
		,_ACH_ID(STEAMACH_WATCHED_THE_OUTRO , "Watched the outro")
		,_ACH_ID(STEAMACH_GOT_A_HIGHSCORE , "Got a high score")
		,_ACH_ID(STEAMACH_GOT_THE_BEST_SCORE , "Got the best score")
		,_ACH_ID(STEAMACH_DOG_ATE_THE_MEAT , "Dog ate the meat")
		,_ACH_ID(STEAMACH_USED_HOSE , "Found petrol")
		,_ACH_ID(STEAMACH_CLOSED_THE_FRIDGE , "Closed the fridge")
		,_ACH_ID(STEAMACH_READ_INVOICE , "Read the invoice")
		,_ACH_ID(STEAMACH_READ_TOMBSTONE , "Inspected the tombstone")
	};
};




class SteamManager
{
public:
	SteamManager();
	~SteamManager();

	bool Initialize();
	void Terminate();

	static bool SetAchievement(Achievements achievement);
	static bool SetOricAchievements(uint8_t achievements[ACHIEVEMENT_BYTE_COUNT]);

	static bool ClearAllAchievements();

public:
	bool				m_IsInitialised = false;
	std::atomic<bool>	m_ThreadRunning = true;
	std::thread         m_UpdateThread;
};

extern SteamManager	   g_SteamManager;

#endif // STEAM_LAUNCHER

