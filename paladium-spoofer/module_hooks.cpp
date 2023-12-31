#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <algorithm>
#include <string>

#pragma comment(lib, "Wtsapi32.lib")
#include <WtsApi32.h>

#include "utils.h"
#include "detours.h"
#include "module_hooks.hpp"

// These DLLs are loaded by the spoofer
const wchar_t* names[] = {
    L"fastprox.dll",
    L"wbemcomn.dll",
};

// china solution for a china-tier anticheat
std::wstring whitelisted_names[] = {
    L"svchost.exe",
    L"cefsharp.browsersubprocess.exe",
    L"audiodg.exe",
    L"windowsterminal.exe",
    L"servicehub.settingshost.exe",
    L"spotify.exe",
    L"lghub_agent.exe",
    L"wininit.exe",
    L"chrome.exe",
    L"servicehub.indexingservice.exe",
    L"servicehub.host.netfx.x86.exe",
    L"java.exe",
    L"javaw.exe",
    L"runtimebroker.exe",
    L"servicehub.testWindowstorehost.exe",
    L"textinputhost.exe",
    L"unsecapp.exe",
    L"razer central.exe",
    L"rzsdkservice.exe",
    L"servicehub.indexingservice.exe",
    L"nvcontainer.exe",
    L"dllhost.exe",
    L"discord.exe",
    L"discordcanary.exe",
    L"discordptb.exe",
    L"rzchromastreamserver.exe",
    L"jcef_helper.exe",
    L"translucenttb.exe",
    L"ctfmon.exe",
    L"icue.exe",
    L"csrss.exe",
    L"lsalso.exe",
    L"gameinputsvc.exe",
    L"conhost.exe",
    L"applicationframehost.exe",
    L"nvidia web helper.exe",
    L"gamingservices.exe",
    L"icueupdateservice.exe",
    L"dwm.exe",
    L"firefox.exe",
    L"nvidia share.exe",
    L"wudfhost.exe",
    L"searchhost.exe",
    L"applemobiledeviceservice.exe",
    L"corsair.service.exe",
    L"servicehub.host.anycpu.exe",
    L"corsaircpuidservice.exe",
    L"corsairgamingaudiocfgservice64.exe",
    L"lghub_updater.exe"
};

typedef BOOL(WINAPI* DefOrig_Module32NextW)(
    HANDLE hSnapshot,
    LPMODULEENTRY32W lpme
    );

DefOrig_Module32NextW Orig_Module32NextW = nullptr;

// Original function pointer
typedef BOOL (WINAPI* DefOrig_WTSEnumerateProcessesW)(
    _In_ HANDLE    hServer,
    _Inout_ DWORD* pLevel,
    _In_ DWORD SessionId,
    _Out_ LPWSTR* ppProcessInfo,
    _Out_ DWORD* pCount
    );

DefOrig_WTSEnumerateProcessesW Orig_WTSEnumerateProcessesExW = nullptr;


BOOL WINAPI hk_WTSEnumerateProcessesW(
    _In_ HANDLE hServer, _Inout_ DWORD* pLevel, _In_ DWORD SessionId, _Out_ LPWSTR* ppProcessInfoEx, _Out_ DWORD* pCount
)
{
    BOOL res = Orig_WTSEnumerateProcessesExW(hServer, pLevel, SessionId, ppProcessInfoEx, pCount);
    std::vector<_WTS_PROCESS_INFO_EXW> vec;
    PWTS_PROCESS_INFO_EXW ppProcessInfo = (PWTS_PROCESS_INFO_EXW)*ppProcessInfoEx;

    for (int i = 0; i < *pCount; ++i)
    {
        std::wstring wstr = std::wstring(ppProcessInfo[i].pProcessName);

        for (std::wstring whitelisted_name : whitelisted_names) {
            if (utils::to_lower(wstr) == whitelisted_name) {
                vec.push_back(ppProcessInfo[i]);
                break;
            }
        }
    }

    PWTS_PROCESS_INFO_EXW pNewProcessInfo = (PWTS_PROCESS_INFO_EXW)LocalAlloc(LMEM_FIXED, vec.size() * sizeof(_WTS_PROCESS_INFO_EXW));
    if (!pNewProcessInfo) {
        return FALSE;
    }

    for (size_t i = 0; i < vec.size(); ++i) {
        pNewProcessInfo[i] = vec[i];
    }

    if (*ppProcessInfoEx) {
        WTSFreeMemory(*ppProcessInfoEx);
    }

    *ppProcessInfoEx = (LPWSTR)pNewProcessInfo;
    *pCount = static_cast<DWORD>(vec.size());

    return res;
}

BOOL WINAPI hk_Module32NextW(HANDLE hSnapshot, LPMODULEENTRY32W lpme)
{
    BOOL b = Orig_Module32NextW(hSnapshot, lpme);

    if (!b) {
        return FALSE;
    }

    wchar_t name[MAX_PATH] = { 0 };
    GetModuleFileNameW(lpme->hModule, name, MAX_PATH);
    std::wstring module_path = std::wstring(name);

    for (const wchar_t* blacklisted_module_name : names) {
        if (utils::ends_with(utils::to_lower(module_path), blacklisted_module_name)) {
            return Module32NextW(hSnapshot, lpme);
        }
    }

    return TRUE;
}

void module_hooks::initialize_hooks()
{
    Orig_Module32NextW = reinterpret_cast<DefOrig_Module32NextW>(GetProcAddress(utils::find_or_load_library("kernel32.dll"), "Module32NextW"));
    Orig_WTSEnumerateProcessesExW = reinterpret_cast<DefOrig_WTSEnumerateProcessesW>(GetProcAddress(utils::find_or_load_library("wtsapi32.dll"), "WTSEnumerateProcessesExW"));

    DetourAttach(&(PVOID&)Orig_Module32NextW, hk_Module32NextW);
    DetourAttach(&(PVOID&)Orig_WTSEnumerateProcessesExW, hk_WTSEnumerateProcessesW);
}