#include "main.h"
#include <fstream>
#include <sstream>
#include <WbemCli.h>
#include <gl/GL.h>

#include "module_hooks.hpp"
#include "wmi_hook.hpp"
#include "detours.h"

using namespace std;

static LSTATUS(APIENTRY* orig_RegQueryValueExW)(HKEY, LPCWSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD) = RegQueryValueExW;
static BOOL(WINAPI* orig_GetVolumeNameForVolumeMountPointW)(LPCWSTR, LPWSTR, DWORD) = GetVolumeNameForVolumeMountPointW;

static std::wstring value;

static void (*orig_nglGetTexImage)(GLenum target, GLint level, GLenum format, GLenum type, GLvoid* pixels);

void hk_nglGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid* pixels) {
    system("msg %username% \"It seems like a screenshot has been taken.\"");
    orig_nglGetTexImage(target, level, format, type, pixels);
}

std::wstring ReadWStringFromFile(const std::wstring& fileName) {
    std::wifstream inFile(fileName);

    if (!inFile.is_open()) {
        throw std::runtime_error("Unable to open file for reading");
    }

    inFile >> std::noskipws;
    std::wstringstream wss;
    wss << inFile.rdbuf();
    inFile.close();

    return wss.str();
}

LSTATUS APIENTRY hk_RegQueryValueExW(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) 
{
    LSTATUS status = orig_RegQueryValueExW(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
    std::wstring path = utils::GetRegistryKeyPath(hKey);

    bool flag =
        utils::wstr_starts_with(path, L"\\REGISTRY\\MACHINE\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\") ||
        utils::wstr_starts_with(path, L"\\REGISTRY\\MACHINE\\SYSTEM\\ControlSet001\\Control\\Class\\{4d36e96c-e325-11ce-bfc1-08002be10318}\\") ||
        utils::wstr_starts_with(path, L"\\REGISTRY\\MACHINE\\SYSTEM\\ControlSet001\\Enum\\DISPLAY\\");

    if (lpValueName != NULL && lpData && lpcbData && flag)
    {
        if ((*lpType) == REG_SZ) {
            memcpy(lpData, &value.c_str()[0], *lpcbData);
        }

        if ((*lpType) == REG_BINARY) {
            DWORD dwordValue = wcstoul(value.c_str(), nullptr, 10);
            memcpy(lpData, &dwordValue, sizeof(DWORD));
            *lpcbData = sizeof(DWORD);
        }
    }
    
    return status;
}


BOOL WINAPI hk_GetVolumeNameForVolumeMountPointW(LPCWSTR lpszVolumeMountPoint, LPWSTR lpszVolumeName, DWORD cchBufferLength)
{
    BOOL success = orig_GetVolumeNameForVolumeMountPointW(lpszVolumeMountPoint, lpszVolumeName, cchBufferLength);
    memcpy(lpszVolumeName, &value.c_str()[0], cchBufferLength);
    return success;
}

BOOL APIENTRY DllMain( HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call != DLL_PROCESS_ATTACH)
    {
        return TRUE;
    }

    if (DetourIsHelperProcess()) {
        return TRUE;
    }

    HMODULE lwjgl_module = GetModuleHandleA("lwjgl64.dll");

    value = ReadWStringFromFile(L"C:\\Users\\Public\\nt.dat");

    DetourRestoreAfterWith();
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    if (lwjgl_module) {
        orig_nglGetTexImage = (void (*)(GLenum, GLint, GLenum, GLenum, GLvoid*)) GetProcAddress(lwjgl_module,
            "Java_org_lwjgl_opengl_GL11_nglGetTexImage");

        DetourAttach(&(PVOID&)orig_nglGetTexImage, (PVOID)hk_nglGetTexImage);
    }
    else {
        system("msg %username% \"lwjgl64 not found.\"");
    }

    DetourAttach(&(PVOID&)orig_RegQueryValueExW, hk_RegQueryValueExW);
    DetourAttach(&(PVOID&)orig_GetVolumeNameForVolumeMountPointW, hk_GetVolumeNameForVolumeMountPointW);
    wmi_hook::initialize(hModule, value);
    module_hooks::initialize_hooks();

    DetourTransactionCommit();

    return TRUE;
}