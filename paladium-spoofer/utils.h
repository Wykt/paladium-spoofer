#pragma once

#include "includes.h"
#include <vector>
#include <ntstatus.h>

#pragma comment(lib, "ntdll")


namespace utils {
	std::wstring GetRegistryKeyPath(HKEY hKey);
	bool ends_with(std::wstring mainStr, std::wstring toMatch);
	std::wstring to_lower(const std::wstring& str);
	bool wstr_starts_with(const std::wstring& mainStr, const std::wstring& toMatch);
	HMODULE find_or_load_library(LPCSTR str);
	void message_alert(const char* chars);
}