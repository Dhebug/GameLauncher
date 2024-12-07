
#include "common.h"



// Global access to Achievements object
SteamAchievements* g_SteamAchievements = NULL;




SteamAchievements::SteamAchievements() :
	m_CallbackUserStatsReceived(this, &SteamAchievements::OnUserStatsReceived),
	m_CallbackUserStatsStored(this, &SteamAchievements::OnUserStatsStored),
	m_CallbackAchievementStored(this, &SteamAchievements::OnAchievementStored)
{
	m_AppID = SteamUtils()->GetAppID();
	RequestStats();
}

SteamAchievements::~SteamAchievements()
{
}



bool SteamAchievements::RequestStats()
{
	// Is Steam loaded? If not we can't get stats.
	if (NULL == SteamUserStats() || NULL == SteamUser())
	{
		return false;
	}
	// Is the user logged on?  If not we can't get stats.
	if (!SteamUser()->BLoggedOn())
	{
		return false;
	}
	// Request user stats.
	return SteamUserStats()->RequestCurrentStats();
}



bool SteamAchievements::SetAchievement(Achievements achievement)
{
	// Have we received a call back from Steam yet?
	if (m_IsInitialized)
	{
		if (achievement < m_Achievements.size())
		{
			SteamUserStats()->SetAchievement(m_Achievements[achievement].m_pchAchievementID);
			return SteamUserStats()->StoreStats();
		}

	}
	// If not then we can't set achievements yet
	return false;
}


bool SteamAchievements::SetOricAchievements(uint8_t achievements[ACHIEVEMENT_BYTE_COUNT])
{
	// Have we received a call back from Steam yet?
	if (m_IsInitialized)
	{
		bool needStore = false;
		unsigned char achievementOffset = 0;
		unsigned char achievementMask = 1;

		Achievement* achievementPtr(&m_Achievements[_FIRST_ORIC_ACHIEVEMENT_]);
		for (int entry = 0; entry < ACHIEVEMENT_COUNT_; entry++)
		{
			if (achievementMask == 0)
			{
				achievementMask = 1;
				achievementOffset++;
			}
			// Is it unlocked?
			if (achievements[achievementOffset] & achievementMask)
			{
				if ( (entry + _FIRST_ORIC_ACHIEVEMENT_) < m_Achievements.size())
				{
					if (!achievementPtr->m_bAchieved)
					{
						SteamUserStats()->SetAchievement(achievementPtr->m_pchAchievementID);
						achievementPtr->m_bAchieved = true;
						needStore = true;
					}
				}
			}
			achievementMask <<= 1;
			++achievementPtr;
		}
		if (needStore)
		{
			return SteamUserStats()->StoreStats();
		}
		return true;
	}
	// If not then we can't set achievements yet
	return false;
}



bool SteamAchievements::ClearAllAchievements()
{
	// Have we received a call back from Steam yet?
	if (m_IsInitialized)
	{
		Achievement* achievementPtr(&m_Achievements[_FIRST_ORIC_ACHIEVEMENT_]);
		for (int entry = 0; entry < ACHIEVEMENT_COUNT_; entry++)
		{
			SteamUserStats()->ClearAchievement(achievementPtr->m_pchAchievementID);
			++achievementPtr;
		}
		bool isOk=SteamUserStats()->StoreStats();
		RequestStats();
		return isOk;
	}
	// If not then we can't set achievements yet
	return false;
}


void SteamAchievements::OnUserStatsReceived(UserStatsReceived_t* pCallback)
{
	// we may get callbacks for other games' stats arriving, ignore them
	if (m_AppID == pCallback->m_nGameID)
	{
		if (k_EResultOK == pCallback->m_eResult)
		{
			OutputDebugStringA("Received stats and achievements from Steam\n");
			m_IsInitialized = true;

			// load achievements
			for (Achievement& achievement : m_Achievements)
			{
				achievement.GetSteamStatus();
				/*
				SteamUserStats()->GetAchievement(achievement.m_pchAchievementID, &achievement.m_bAchieved);
				_snprintf(achievement.m_rgchName, sizeof(achievement.m_rgchName), "%s",SteamUserStats()->GetAchievementDisplayAttribute(achievement.m_pchAchievementID,"name"));
				_snprintf(achievement.m_rgchDescription, sizeof(achievement.m_rgchDescription), "%s",SteamUserStats()->GetAchievementDisplayAttribute(achievement.m_pchAchievementID,"desc"));				
				*/
			}
		}
		else
		{
			char buffer[128];
			_snprintf(buffer, 128, "RequestStats - failed, %d\n", pCallback->m_eResult);
			OutputDebugStringA(buffer);
		}
	}
}


void SteamAchievements::OnUserStatsStored(UserStatsStored_t* pCallback)
{
	// we may get callbacks for other games' stats arriving, ignore them
	if (m_AppID == pCallback->m_nGameID)
	{
		if (k_EResultOK == pCallback->m_eResult)
		{
			OutputDebugStringA("Stored stats for Steam\n");
		}
		else
		{
			char buffer[128];
			_snprintf(buffer, 128, "StatsStored - failed, %d\n", pCallback->m_eResult);
			OutputDebugStringA(buffer);
		}
	}
}


void SteamAchievements::OnAchievementStored(UserAchievementStored_t* pCallback)
{
	// we may get callbacks for other games' stats arriving, ignore them
	if (m_AppID == pCallback->m_nGameID)
	{
		OutputDebugStringA("Stored Achievement for Steam\n");
	}
}


extern SteamAchievements* g_SteamAchievements;



// https://partner.steamgames.com/doc/sdk/api
SteamManager::SteamManager()
{
}

SteamManager::~SteamManager()
{
	Terminate();
}

bool SteamManager::Initialize()
{
	SteamErrMsg errMsg = { 0 };
	if (SteamAPI_InitEx(&errMsg) != k_ESteamAPIInitResult_OK)
	{
		OutputDebugStringA("SteamAPI_Init() failed: ");
		OutputDebugStringA(errMsg);
		OutputDebugStringA("\n");

		// Handle initialization failure
		MessageBox(0, TEXT("Failed initializing the Steam PI"), TEXT("Error"), MB_OK | MB_ICONERROR);
		return EXIT_FAILURE;
	}

	g_SteamAchievements = new SteamAchievements();
	m_IsInitialised = true;
	m_UpdateThread = std::thread([this]()
		{
			while (m_ThreadRunning)
			{
				SteamAPI_RunCallbacks();
				Sleep(100);					// Sleep for 100 milliseconds to achieve 10Hz
			}
		});

	return m_IsInitialised;
}


void SteamManager::Terminate()
{
	if (m_IsInitialised)
	{
		delete g_SteamAchievements;
		m_ThreadRunning = false;
		if (m_UpdateThread.joinable())
		{
			m_UpdateThread.join();
		}
		SteamAPI_Shutdown();
		m_IsInitialised = false;
	}
}


// static 
bool SteamManager::SetAchievement(Achievements achievement)
{
	return g_SteamAchievements ? g_SteamAchievements->SetAchievement(achievement) : false;
}

// static
bool SteamManager::SetOricAchievements(uint8_t achievements[ACHIEVEMENT_BYTE_COUNT])
{
	return g_SteamAchievements ? g_SteamAchievements->SetOricAchievements(achievements) : false;
}

// static
bool SteamManager::ClearAllAchievements()
{
	return g_SteamAchievements ? g_SteamAchievements->ClearAllAchievements() : false;
}

