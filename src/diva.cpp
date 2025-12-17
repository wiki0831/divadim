#include <string.h>
#include "diva.h"
#include "Helpers.h"

namespace diva
{
	FUNCTION_PTR(diva::SprArgs*, __fastcall, DrawRectangle, 0x1405B4D40, Rect* rect, int32_t res, int32_t layer, uint32_t color, int32_t prio);

	FUNCTION_PTR(diva::SprArgs*, __fastcall, DefaultSprArgs, 0x1405B78D0, diva::SprArgs* args);
	SprArgs::SprArgs()
	{
		DefaultSprArgs(this);
	}
}