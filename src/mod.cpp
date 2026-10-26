#include <stdio.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <detours.h>
#include "thirdparty/toml.hpp"
#include "diva.h"

// NOTE: Settings from `config.toml`
static int32_t DefaultDim = 0;
static bool DrawBehind = false;
static std::map<int32_t, int32_t> PVSettings;

// NOTE: Hotkey settings
static int32_t HotKeyDarker = 0xBD;   // -
static int32_t HotKeyBrighter = 0xBB; // =
static int32_t HotKeyStep = 10;

// NOTE: Hotkey state tracking to prevent key repeat
static bool KeyDarkerPressed = false;
static bool KeyBrighterPressed = false;

//
static int32_t DimPercentage = 0;

// NOTE: Same address in every known version
static const int32_t* CurrentPvID = (const int32_t*)0x14CBFFE2C;

//
static inline int32_t Clamp(int32_t v, int32_t min, int32_t max)
{
	if (v > max) return max;
	else if (v < min) return min;
	return v;
}

// NOTE: Function addresses differ between game versions, see DECODING_NEW_EXE.md
struct GameVersion
{
	uintptr_t taskPvGameInit;
	uintptr_t taskPvGameDisp;
	uintptr_t drawRectangle;
};

static const GameVersion Versions[] =
{
	{ 0x1405DA040, 0x1405DA090, 0x1405B4D40 }, // Current Steam version
	{ 0x1405DA000, 0x1405DA050, 0x1405B4D00 }, // Early 2023 version
};

// NOTE: Start of DrawRectangle, identical in every known version
static const uint8_t DrawRectangleSignature[] =
{
	0x48, 0x89, 0x5C, 0x24, 0x08, 0x44, 0x89, 0x4C, 0x24, 0x20,
	0x55, 0x56, 0x57, 0x48, 0x81, 0xEC, 0xA0, 0x01, 0x00, 0x00
};

static bool Matches(uintptr_t address, const uint8_t* bytes, size_t size)
{
	__try
	{
		return memcmp((const void*)address, bytes, size) == 0;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}
}

HOOK(int64_t, __fastcall, TaskPvGameInit, 0, uint64_t a1)
{
	int64_t ret = originalTaskPvGameInit(a1);

	// NOTE: Reset state
	DimPercentage = DefaultDim;

	// NOTE: Attemp to retrive PV-specific setting
	if (auto it = PVSettings.find(*CurrentPvID); it != PVSettings.end())
		DimPercentage = it->second;

	return ret;
}

HOOK(uint64_t, __fastcall, TaskPvGameDisp, 0, uint64_t a1)
{
	// NOTE: Check for hotkey inputs
	bool darkerPressed = (GetAsyncKeyState(HotKeyDarker) & 0x8000) != 0;
	bool brighterPressed = (GetAsyncKeyState(HotKeyBrighter) & 0x8000) != 0;

	// NOTE: Handle darker (only trigger once per key press)
	if (darkerPressed && !KeyDarkerPressed)
	{
		DimPercentage = Clamp(DimPercentage + HotKeyStep, 0, 100);
		KeyDarkerPressed = true;
	}
	else if (!darkerPressed)
	{
		KeyDarkerPressed = false;
	}

	// NOTE: Handle brighter (only trigger once per key press)
	if (brighterPressed && !KeyBrighterPressed)
	{
		DimPercentage = Clamp(DimPercentage - HotKeyStep, 0, 100);
		KeyBrighterPressed = true;
	}
	else if (!brighterPressed)
	{
		KeyBrighterPressed = false;
	}

	// NOTE: Draw dim rectangle
	if (DimPercentage > 0)
	{
		diva::Rect rect = { };
		rect.x = 0.0f;
		rect.y = 0.0f;
		rect.width = 1920.0f;
		rect.height = 1080.0f;

		uint32_t alpha = Clamp(static_cast<float>(DimPercentage) / 100 * 255, 0, 255);
		uint32_t color = (alpha & 0xFF) << 24;

		int32_t layer = DrawBehind ? 0 : 2;

		diva::DrawRectangle(&rect, 14, layer, color, 0);
	}

	return originalTaskPvGameDisp(a1);
}

static void LoadSettings()
{
	PVSettings.clear();

	toml::table tbl = toml::parse_file("config.toml");
	DefaultDim = tbl["settings"]["default_dim"].value_or<int32_t>(0);
	DrawBehind = tbl["settings"]["behind_pv_aet"].value_or<bool>(false);

	// NOTE: Load hotkey settings
	HotKeyDarker = tbl["settings"]["hotkey_darker"].value_or<int32_t>(0xBD);   // -
	HotKeyBrighter = tbl["settings"]["hotkey_brighter"].value_or<int32_t>(0xBB); // =
	HotKeyStep = tbl["settings"]["hotkey_step"].value_or<int32_t>(10);

	if (toml::array* arr = tbl["settings"]["pv_settings"].as_array())
	{
		for (size_t i = 0; i < arr->size(); i++)
		{
			toml::array* info = arr->at(i).as_array();

			if (info->size() == 2)
			{
				int32_t pvId = info->at(0).value_or<int32_t>(0);
				int32_t perc = info->at(1).value_or<int32_t>(1);
				PVSettings[pvId] = perc;
			}
		}
	}
}

extern "C"
{
	void __declspec(dllexport) Init()
	{
		for (const GameVersion& version : Versions)
		{
			if (!Matches(version.drawRectangle, DrawRectangleSignature, sizeof(DrawRectangleSignature)))
				continue;

			LoadSettings();

			diva::DrawRectangle = (decltype(diva::DrawRectangle))version.drawRectangle;
			originalTaskPvGameInit = (TaskPvGameInitDelegate*)version.taskPvGameInit;
			originalTaskPvGameDisp = (TaskPvGameDispDelegate*)version.taskPvGameDisp;
			INSTALL_HOOK(TaskPvGameInit);
			INSTALL_HOOK(TaskPvGameDisp);
			return;
		}

		// NOTE: Unknown game version, do nothing instead of crashing
	}
};
