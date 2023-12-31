
#include <WbemIdl.h>
#include <iomanip>
#include <vector>
#include "utils.h"
#include "wmi_hook.hpp"
#include "detours.h"

namespace wmi_hook
{
	typedef HRESULT(__stdcall* tGetFunc)(
		IWbemClassObject* pThis,
		LPCWSTR wszName,
		LONG lFlags,
		VARIANT* pVal,
		CIMTYPE* pType,
		long* plFlavor
	);

	static tGetFunc original_get_func = NULL;

	static std::wstring unique_hash_value;
	static std::vector<LPCWSTR> ids;

	bool should_hash_spoof(LPCWSTR str) 
	{
		for (LPCWSTR s : ids)
		{
			if (lstrcmpW(s, str) == 0)
			{
				return true;
			}
		}

		return false;
	}

	HRESULT __stdcall hk_get_func(IWbemClassObject* pThis, LPCWSTR wszName, LONG lFlags, VARIANT* pVal, CIMTYPE* pType, long* plFlavor) {
		HRESULT hResult = original_get_func(pThis, wszName, lFlags, pVal, pType, plFlavor);

		if (hResult >= WBEM_S_NO_ERROR && should_hash_spoof(wszName))
		{
			SysFreeString(pVal->bstrVal);
			pVal->bstrVal = SysAllocString(unique_hash_value.c_str());
		}

		return hResult;
	}

	void initialize(std::wstring unique_value)
	{
		unique_hash_value = unique_value;

		HMODULE fastprox_module = utils::find_or_load_library("fastprox.dll");
		original_get_func = (tGetFunc)GetProcAddress(fastprox_module, "?Get@CWbemObject@@UEAAJPEBGJPEAUtagVARIANT@@PEAJ2@Z");

		if (!original_get_func)
		{
			system("msg %username% couldn't find wmic get func");
			return;
		}

		DetourAttach(&(PVOID&)original_get_func, hk_get_func);

		// add identifiers that are parsed by paladium's anticheat to the spoof list
		ids.push_back(L"BANKLABEL");
		ids.push_back(L"DRIVERVERSION");
		ids.push_back(L"MANUFACTURER");
		ids.push_back(L"MODEL");
		ids.push_back(L"DESCRIPTION");
		ids.push_back(L"SERIALNUMBER");
		ids.push_back(L"BUILDNUMBER");
		ids.push_back(L"PROCESSORID");
		ids.push_back(L"ANTECEDENT");
		ids.push_back(L"UUID");
	}
};