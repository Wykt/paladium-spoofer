#pragma once

#include <Windows.h>
#include <string>

namespace wmi_hook
{
	void initialize(HMODULE hSelf, std::wstring unique_value);
};