
/*
 * REminiscence - Flashback interpreter
 * Copyright (C) 2005-2019 Gregory Montoir (cyx@users.sourceforge.net)
 */

#ifndef INTERN_H__
#define INTERN_H__
#include "string.h"
#include <cstdio>
//#include <cstring>
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
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

struct Point {
	int16_t x;
	int16_t y;
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
	uint16_t cutscene_id;
	uint8_t sound;
	uint8_t track;
};

struct SAT_sprite {
	uint32_t cgaddr;
	uint16_t size;
	int16_t x_flip;
	int16_t x;
	int16_t y;
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
	uint16_t type;
	int8_t dx;
	int8_t dy;
	uint16_t init_obj_type;
	uint8_t opcode2;
	uint8_t opcode1;
	uint8_t flags;
	uint8_t opcode3;
	uint16_t init_obj_number;
	int16_t opcode_arg1;
	int16_t opcode_arg2;
	int16_t opcode_arg3;
};

struct ObjectNode {
	uint16 last_obj_number;
	Object *objects;
	uint16 num_objects;
};

struct ObjectOpcodeArgs {
	LivePGE *pge; // arg0
	int16_t a; // arg2
	int16_t b; // arg4
};

struct AnimBufferState {
	int16_t x, y;
	uint8_t w, h;
	const uint8_t *dataPtr;
	LivePGE *pge;
};

struct AnimBuffers {
	AnimBufferState *_states[4];
	uint8_t _curPos[4];

	void addState(uint8_t stateNum, int16_t x, int16_t y, const uint8_t *dataPtr, LivePGE *pge, uint8_t w = 0, uint8_t h = 0);
};

struct CollisionSlot {
	int16_t ct_pos;
	CollisionSlot *prev_slot;
	LivePGE *live_pge;
	uint16_t index;
};

struct MbkEntry {
	uint16 offset;
	uint16 len;
};

struct BankSlot {
	uint16_t entryNum;
	uint8_t *ptr;
};

struct CollisionSlot2 {
	CollisionSlot2 *next_slot;
	int8_t *unk2; // grid_data_pos
	uint8_t data_size;
	uint8_t data_buf[0x10]; // <= InitPGE.collision_data_len
};

struct InventoryItem {
	uint8_t icon_num;
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

#endif // INTERN_H__
