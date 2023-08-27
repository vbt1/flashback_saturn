/* REminiscence - Flashback interpreter
 * Copyright (C) 2005-2007 Gregory Montoir
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef __INTERN_H__
#define __INTERN_H__

#include <cstdio>
#include <cstring>
#include <stdlib.h>
#include <cassert>

#include "sl_def.h"

#include "sys.h"
#include "util.h"

//#define MAX(x,y) ((x)>(y)?(x):(y))
//#define MIN(x,y) ((x)<(y)?(x):(y))
#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

template<typename T>
inline void SWAP(T &a, T &b) {
	T tmp = a;
	a = b;
	b = tmp;
}

template<typename T>
inline T CLIP(const T& val, const T& a, const T& b) {
	if (val < a) {
		return a;
	} else if (val > b) {
		return b;
	}
	return val;
}

enum Language {
	LANG_FR,
	LANG_EN,
	LANG_DE,
	LANG_SP,
	LANG_IT,
	LANG_JP,
};

enum ResourceType {
	kResourceTypeDOS,
	kResourceTypeMac,
};

enum Skill {
	kSkillEasy = 0,
	kSkillNormal,
	kSkillExpert,
};

struct Color {
	uint8 r;
	uint8 g;
	uint8 b;
};

struct Point {
	int16 x;
	int16 y;
};

struct Demo {
	const char *name;
	int level;
	int room;
	int x, y;
};

struct Level {
	const char *name;
	const char *name2;
	uint16 cutscene_id;
};

struct InitPGE {
	uint16_t type;
	int16_t pos_x;
	int16_t pos_y;
	uint16_t obj_node_number;
	uint16_t life;
	int16_t data[4];
	uint8_t object_type; // 1:conrad, 10:monster
	uint8_t init_room;
	uint8_t room_location;
	uint8_t init_flags;
	uint8_t colliding_icon_num;
	uint8_t icon_num;
	uint8_t object_id;
	uint8_t skill;
	uint8_t mirror_x;
	uint8_t flags; // 1:xflip 4:active
	uint8_t collision_data_len;
	uint16_t text_num;
};

struct LivePGE {
	uint16_t obj_type;
	int16_t pos_x;
	int16_t pos_y;
	uint8_t anim_seq;
	uint8_t room_location;
	int16_t life;
	int16_t counter_value;
	uint8_t collision_slot;
	uint8_t next_inventory_PGE;
	uint8_t current_inventory_PGE;
	uint8_t ref_inventory_PGE;
	uint16_t anim_number;
	uint8_t flags;
	uint8_t index;
	uint16_t first_obj_number;
	LivePGE *next_PGE_in_room;
	InitPGE *init_PGE;
};

struct MessagePGE {
	MessagePGE *next_entry;
	uint16_t src_pge;
	uint16_t msg_num;
};

struct Object {
	uint16 type;
	int8 dx;
	int8 dy;
	uint16 init_obj_type;
	uint8 opcode2;
	uint8 opcode1;
	uint8 flags;
	uint8 opcode3;
	uint16 init_obj_number;
	int16 opcode_arg1;
	int16 opcode_arg2;
	int16 opcode_arg3;
};

struct ObjectNode {
	uint16 last_obj_number;
	Object *objects;
	uint16 num_objects;
};

struct ObjectOpcodeArgs {
	LivePGE *pge; // arg0
	int16 a; // arg2
	int16 b; // arg4
};

struct AnimBufferState {
	int16_t x, y;
	uint8_t w, h;
	const uint8_t *dataPtr;
	LivePGE *pge;
};

struct AnimBuffers {
	AnimBufferState *_states[4];
	uint8 _curPos[4];

	void addState(uint8_t stateNum, int16_t x, int16_t y, const uint8_t *dataPtr, LivePGE *pge, uint8_t w = 0, uint8_t h = 0);
};

struct CollisionSlot {
	int16 ct_pos;
	CollisionSlot *prev_slot;
	LivePGE *live_pge;
	uint16 index;
};

struct MbkEntry {
	uint16 offset;
	uint16 len;
};

struct BankSlot {
	uint16 entryNum;
	uint8 *ptr;
};

struct CollisionSlot2 {
	CollisionSlot2 *next_slot;
	int8 *unk2;
	uint8 data_size;
	uint8 data_buf[0x10]; // XXX check size
};

struct InventoryItem {
	uint8 icon_num;
	InitPGE *init_pge;
	LivePGE *live_pge;
};

struct SoundFx {
	uint32_t offset;
	uint16_t len;
	uint16_t freq;
	uint8_t *data;
	int8_t peak;
};

#endif // __INTERN_H__
