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

//
static int32_t DimPercentage = 0;

//
static const int32_t* CurrentPvID = (const int32_t*)0x14CBFFE2C;

//
static inline int32_t Clamp(int32_t v, int32_t min, int32_t max)
{
	if (v > max) return max;
	else if (v < min) return min;
	return v;
}

HOOK(int64_t, __fastcall, TaskPvGameInit, 0x1405DA040, uint64_t a1)
{
	int64_t ret = originalTaskPvGameInit(a1);

	// NOTE: Reset state
	DimPercentage = DefaultDim;

	// NOTE: Attemp to retrive PV-specific setting
	if (auto it = PVSettings.find(*CurrentPvID); it != PVSettings.end())
		DimPercentage = it->second;

	return ret;
}

HOOK(uint64_t, __fastcall, TaskPvGameDisp, 0x1405DA090, uint64_t a1)
{
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
		LoadSettings();
		INSTALL_HOOK(TaskPvGameInit);
		INSTALL_HOOK(TaskPvGameDisp);
	}
};