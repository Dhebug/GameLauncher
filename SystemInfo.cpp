#include "common.h"

#include <dxgi.h>
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "winmm.lib")
#include <mmsystem.h>


std::wstring GetWindowsVersionInfo()
{
	std::wstring result = L"Unknown";
	HKEY hKey;
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		wchar_t productName[256]   = {};
		wchar_t displayVersion[64] = {};
		wchar_t currentBuild[32]   = {};
		DWORD size;
		size = sizeof(productName);    RegQueryValueExW(hKey, L"ProductName",    nullptr, nullptr, (LPBYTE)productName,    &size);
		size = sizeof(displayVersion); RegQueryValueExW(hKey, L"DisplayVersion", nullptr, nullptr, (LPBYTE)displayVersion, &size);
		size = sizeof(currentBuild);   RegQueryValueExW(hKey, L"CurrentBuild",   nullptr, nullptr, (LPBYTE)currentBuild,   &size);
		RegCloseKey(hKey);

		result = productName;
		if (displayVersion[0]) { result += L" "; result += displayVersion; }
		if (currentBuild[0])   { result += L" (Build "; result += currentBuild; result += L")"; }
	}
	return result;
}

std::wstring GetCpuName()
{
	std::wstring result = L"Unknown";
	HKEY hKey;
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		wchar_t name[256] = {};
		DWORD size = sizeof(name);
		if (RegQueryValueExW(hKey, L"ProcessorNameString", nullptr, nullptr, (LPBYTE)name, &size) == ERROR_SUCCESS)
		{
			result = name;
			size_t s = result.find_first_not_of(L' ');
			size_t e = result.find_last_not_of(L' ');
			if (s != std::wstring::npos) result = result.substr(s, e - s + 1);
		}
		RegCloseKey(hKey);
	}
	return result;
}

std::wstring GetGpuName()
{
	std::wstring result = L"Unknown";
	IDXGIFactory* factory = nullptr;
	if (SUCCEEDED(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory)))
	{
		IDXGIAdapter* adapter = nullptr;
		if (SUCCEEDED(factory->EnumAdapters(0, &adapter)))
		{
			DXGI_ADAPTER_DESC desc = {};
			if (SUCCEEDED(adapter->GetDesc(&desc)))
			{
				result = desc.Description;
				SIZE_T vramMB = desc.DedicatedVideoMemory / (1024 * 1024);
				if (vramMB > 0)
				{
					wchar_t vram[32];
					swprintf_s(vram, L" (%zu MB VRAM)", vramMB);
					result += vram;
				}
			}
			adapter->Release();
		}
		factory->Release();
	}
	return result;
}

std::wstring GetAudioDevices()
{
	std::wstring result;
	UINT n = waveOutGetNumDevs();
	for (UINT i = 0; i < n; i++)
	{
		WAVEOUTCAPSW caps = {};
		if (waveOutGetDevCapsW(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR)
		{
			if (!result.empty()) result += L", ";
			result += caps.szPname;
		}
	}
	return result.empty() ? L"None" : result;
}

std::wstring GetMemoryInfo()
{
	MEMORYSTATUSEX ms = { sizeof(ms) };
	if (GlobalMemoryStatusEx(&ms))
	{
		wchar_t buf[64];
		ULONGLONG totalGB = ms.ullTotalPhys / (1024ULL * 1024 * 1024);
		ULONGLONG availMB = ms.ullAvailPhys / (1024ULL * 1024);
		swprintf_s(buf, L"%llu GB total, %llu MB available", totalGB, availMB);
		return buf;
	}
	return L"Unknown";
}

std::wstring GetScreenInfo()
{
	int cx  = GetSystemMetrics(SM_CXSCREEN);
	int cy  = GetSystemMetrics(SM_CYSCREEN);
	int vx  = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	int vy  = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	int mon = GetSystemMetrics(SM_CMONITORS);
	wchar_t buf[128];
	if (mon > 1)
		swprintf_s(buf, L"%dx%d (primary), %d monitors, virtual %dx%d", cx, cy, mon, vx, vy);
	else
		swprintf_s(buf, L"%dx%d", cx, cy);
	return buf;
}

std::wstring LocaleToEnglishName(LCID lcid)
{
	wchar_t lang[128] = {}, country[128] = {};
	GetLocaleInfoW(lcid, LOCALE_SENGLISHLANGUAGENAME, lang, 128);
	GetLocaleInfoW(lcid, LOCALE_SENGLISHCOUNTRYNAME,  country, 128);
	std::wstring r = lang;
	if (country[0] && wcscmp(lang, country) != 0)
		r += std::wstring(L" (") + country + L")";
	return r.empty() ? L"Unknown" : r;
}

std::wstring GetKeyboardLanguages()
{
	std::wstring result;
	std::vector<DWORD> seen;

	// Helper: resolve a KLID through Keyboard Layout\Substitutes if an entry exists
	// (e.g. "d0010409" -> "00000414" for Norwegian Bokmål on a US-based system).
	HKEY hSubstitutes = nullptr;
	RegOpenKeyExW(HKEY_CURRENT_USER, L"Keyboard Layout\\Substitutes", 0, KEY_READ, &hSubstitutes);

	auto resolveKlid = [hSubstitutes](const wchar_t* klid, wchar_t* out, DWORD outSize)
	{
		wcsncpy_s(out, outSize, klid, _TRUNCATE);
		if (!hSubstitutes) return;
		wchar_t substitute[16] = {};
		DWORD size = sizeof(substitute);
		if (RegQueryValueExW(hSubstitutes, klid, nullptr, nullptr, (LPBYTE)substitute, &size) == ERROR_SUCCESS)
			wcsncpy_s(out, outSize, substitute, _TRUNCATE);
	};

	// Put the currently active layout first so it appears as the primary entry
	wchar_t activeKlid[KL_NAMELENGTH] = {};
	if (GetKeyboardLayoutNameW(activeKlid))
	{
		wchar_t resolved[16] = {};
		resolveKlid(activeKlid, resolved, _countof(resolved));
		DWORD langId = wcstoul(resolved, nullptr, 16) & 0xFFFF;
		seen.push_back(langId);
		result = LocaleToEnglishName(MAKELCID(langId, SORT_DEFAULT));
	}

	// Append any remaining layouts from Preload that weren't the active one
	HKEY hPreload;
	if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Keyboard Layout\\Preload", 0, KEY_READ, &hPreload) == ERROR_SUCCESS)
	{
		for (DWORD index = 0; ; index++)
		{
			wchar_t valueName[16] = {};
			wchar_t klid[16]      = {};
			DWORD nameSize = _countof(valueName);
			DWORD dataSize = sizeof(klid);
			DWORD type;
			if (RegEnumValueW(hPreload, index, valueName, &nameSize, nullptr, &type, (LPBYTE)klid, &dataSize) != ERROR_SUCCESS)
				break;
			if (type != REG_SZ) continue;

			wchar_t resolved[16] = {};
			resolveKlid(klid, resolved, _countof(resolved));
			DWORD langId = wcstoul(resolved, nullptr, 16) & 0xFFFF;
			if (std::find(seen.begin(), seen.end(), langId) != seen.end()) continue;
			seen.push_back(langId);
			if (!result.empty()) result += L", ";
			result += LocaleToEnglishName(MAKELCID(langId, SORT_DEFAULT));
		}
		RegCloseKey(hPreload);
	}

	if (hSubstitutes) RegCloseKey(hSubstitutes);

	return result.empty() ? L"Unknown" : result;
}
