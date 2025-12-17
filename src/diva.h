#pragma once

#include <stdint.h>
#include "Helpers.h"

namespace diva
{
	struct vec2
	{
		float x;
		float y;
	};

	struct vec3
	{
		float x;
		float y;
		float z;
	};

	struct vec4
	{
		float x;
		float y;
		float z;
		float w;
	};

	struct mat4
	{
		vec4 row0;
		vec4 row1;
		vec4 row2;
		vec4 row3;
	};

	struct Rect
	{
		float x;
		float y;
		float width;
		float height;
	};

	struct SprArgs
	{
		uint32_t kind;
		int32_t id;
		uint8_t color[4];
		int32_t attr;
		int32_t blend;
		int32_t index;
		int32_t priority;
		int32_t layer;
		int32_t resolution_mode_screen;
		int32_t resolution_mode_sprite;
		vec3 center;
		vec3 trans;
		vec3 scale;
		vec3 rot;
		vec2 skew_angle;
		mat4 mat;
		void* texture;
		int32_t shader;
		int32_t field_AC;
		mat4 transform;
		bool field_F0;
		void* vertex_array;
		size_t num_vertex;
		int32_t field_108;
		void* field_110;
		bool set_viewport;
		vec4 viewport;
		uint32_t flags;
		vec2 sprite_size;
		int32_t field_138;
		vec2 texture_pos;
		vec2 texture_size;
		SprArgs* next;
		void* tex;

		SprArgs();
	};

	FUNCTION_PTR_H(diva::SprArgs*, __fastcall, DrawRectangle, 0x1405B4D40, Rect* rect, int32_t res, int32_t layer, uint32_t color, int32_t prio);
}