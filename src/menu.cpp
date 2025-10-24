
/*
 * REminiscence - Flashback interpreter
 * Copyright (C) 2005-2019 Gregory Montoir (cyx@users.sourceforge.net)
 */

extern "C" {
	#include 	<string.h>
#include <sl_def.h>
#include <sgl.h>
#include <sega_bup.h>
#include <sega_per.h>
#include "sega_sys.h"
#include "sat_mem_checker.h"
extern Uint8 *current_lwram;
Uint8 selected = 34;
}

#include "decode_mac.h"
#include "game.h"
#include "menu.h"
#include "resource.h"
#include "systemstub.h"
#include "video.h"
#include "menu.h"

#undef sleep 

Menu::Menu(Resource *res, SystemStub *stub, Video *vid)
	: _res(res), _stub(stub), _vid(vid) {
	_stateSlot = -1;
}

void Menu::drawString(const char *str, int16_t y, int16_t x, uint8_t colorConfig) {
//	debug(DBG_MENU, "Menu::drawString()");
	const uint8_t v1b = _vid->_charFrontColor;
	const uint8_t v2b = _vid->_charTransparentColor;
	const uint8_t v3b = _vid->_charShadowColor;
	switch (colorConfig) {
	case 0: // unused
		_vid->_charFrontColor = _charVar1;
		_vid->_charTransparentColor = _charVar2;
		_vid->_charShadowColor = _charVar2;
		break;
	case 1:
		_vid->_charFrontColor = _charVar2;
		_vid->_charTransparentColor = _charVar1;
		_vid->_charShadowColor = _charVar1;
		break;
	case 2:
		_vid->_charFrontColor = _charVar3;
		_vid->_charTransparentColor = 0xFF;
		_vid->_charShadowColor = _charVar1;
		break;
	case 3:
		_vid->_charFrontColor = _charVar4;
		_vid->_charTransparentColor = 0xFF;
		_vid->_charShadowColor = _charVar1;
		break;
	case 4: // unused
		_vid->_charFrontColor = _charVar2;
		_vid->_charTransparentColor = 0xFF;
		_vid->_charShadowColor = _charVar1;
		break;
	case 5: // unused
		_vid->_charFrontColor = _charVar2;
		_vid->_charTransparentColor = 0xFF;
		_vid->_charShadowColor = _charVar5;
		break;
	}

	drawString2(str, y, x);

	_vid->_charFrontColor = v1b;
	_vid->_charTransparentColor = v2b;
	_vid->_charShadowColor = v3b;
}

void Menu::drawString2(const char *str, int16_t y, int16_t x) {
//	debug(DBG_MENU, "Menu::drawString2()");
	const int w = Video::CHAR_W;
	const int h = Video::CHAR_H;
	int len = 0;
/*	switch (_res->_type) {
	case kResourceTypeAmiga:
		for (; str[len]; ++len) {
			_vid->AMIGA_drawStringChar(_vid->_frontLayer, _vid->_w, Video::CHAR_W * (x + len), Video::CHAR_H * y, _res->_fnt, _vid->_charFrontColor, (uint8_t)str[len]);
		}
		break;
	case kResourceTypeDOS:
	case kResourceTypePC98:
		for (; str[len]; ++len) {
			_vid->DOS_drawChar((uint8_t)str[len], y, x + len, true);
		}
		break;
	case kResourceTypeMac:*/
		for (; str[len]; ++len) {
			_vid->MAC_drawStringChar(_vid->_frontLayer, _vid->_w, w * (x + len), h * y, _res->_fnt, _vid->_charFrontColor, (uint8_t)str[len]);
		}
/*		break;
	}*/
//	_vid->markBlockAsDirty(x * w, y * h, len * w, h, _vid->_layerSize);
}

void Menu::loadPicture(const char *prefix) {
//	emu_printf("Menu::loadPicture('%s')\n", prefix);
//	if (_res->isMac()) {
		static const struct {
			const char *prefix;
			int8_t num;
		} screens[] = {
			{ "menu1", kMacTitleScreen_Flashback },
			{ "menu2", kMacTitleScreen_RightEye },
			{ "menu3", kMacTitleScreen_LeftEye },
			{ "instr", kMacTitleScreen_Controls },
			{ 0, -1 }
		};
		for (int i = 0; screens[i].prefix; ++i) {
			if (strncmp(prefix, screens[i].prefix, strlen(screens[i].prefix)) == 0) {
				displayTitleScreenMac(screens[i].num);
				if (screens[i].num == kMacTitleScreen_Controls) {
//					memcpy(_vid->_backLayer, _vid->_frontLayer, _vid->_layerSize);
					_stub->copyRect(0, 0, _vid->_w, _vid->_h, current_lwram, _vid->_w);
					displayTitleScreenMac(kMacTitleScreen_LeftEye);
				}
				break;
			}
		}
	memcpy(_vid->_backLayer, _vid->_frontLayer, _vid->_layerSize);
	memset(_vid->_frontLayer, 0x00, _vid->_layerSize); // vbt : vire l'image sur la layer texte
//	_vid->updateWidescreen();
}

void Menu::displayTitleScreenMac(int num) {
	const int w = 512;
	int h = 384;
	int clutBaseColor = 0;

	slScrTransparent(NBG0ON|!NBG1ON);
	slScrAutoDisp(NBG0ON|NBG1ON|SPRON);
	slBMPaletteNbg1(2); // passage à la palette non recalculée	

	DecodeBuffer buf{};
	buf.ptr = _vid->_frontLayer;
	
	switch (num) {
	case kMacTitleScreen_MacPlay:
//emu_printf("loading macplay\n");
		break;
	case kMacTitleScreen_Presage:
//emu_printf("loading presage\n");
		clutBaseColor = 12;
		break;
	case kMacTitleScreen_Flashback:
	case kMacTitleScreen_LeftEye:
	case kMacTitleScreen_RightEye:
		memset(_vid->_frontLayer, 0, w * h);
		h = 448;
		break;
	case kMacTitleScreen_Controls:
		buf.ptr = current_lwram;
		memset(buf.ptr, 0, w * h);
		break;
	}
	buf.dst_w = _vid->_w;
	buf.type = 2;
	buf.dst_h = _vid->_h;
	buf.dst_x = (_vid->_w - w) / 2;
	buf.dst_y = (_vid->_h - h) / 2;

	memset(_vid->_backLayer, 0, w * h);
	_res->MAC_loadTitleImage(num, &buf);

	for (int i = 0; i < 12; ++i) {
		Color palette[16];
		_res->MAC_copyClutN(palette, 0, clutBaseColor + i, 16);
		const int basePaletteColor = i * 16;
		for (int j = 0; j < 16; ++j) {
			_stub->setPaletteEntry(basePaletteColor + j, &palette[j]);
			_stub->setPaletteEntry(256+basePaletteColor + j, &palette[j]);
		}
	}

	if (num == kMacTitleScreen_MacPlay) {
		Color palette[16];
		for (int i = 0; i < 2; ++i) {
			_res->MAC_copyClutN(palette, 0, 55 + i, 16);
			const int basePaletteColor = (12 + i) * 16;
			for (int j = 0; j < 16; ++j) {
				_stub->setPaletteEntry(basePaletteColor + j, &palette[j]);
			}
		}
	} else if (num == kMacTitleScreen_Presage) {
		Color c;
		c.r = c.g = c.b = 0;
		_stub->setPaletteEntry(0, &c);
	} else if (num == kMacTitleScreen_Flashback) {
		Color c;
		c.r = c.g = c.b = 255;
		_stub->setPaletteEntry(0, &c);

		for (int i = 496; i < 512; ++i) {
			_stub->setPaletteEntry(i, &c);
		}
		c.r = c.g = c.b = 0;
		_stub->setPaletteEntry(511, &c);
		_vid->setTextPalette();
//		_mix->playMusic(1); // vbt : déplacé, musique du menu
	}
//	memset(_vid->_frontLayer, 0x11, w * h);
//	_stub->copyRect(0, 0, _vid->_w, _vid->_h, _vid->_frontLayer, _vid->_w);	
	
	if (num == kMacTitleScreen_MacPlay || num == kMacTitleScreen_Presage) 
	{
//		_vid->fullRefresh();
//		_vid->updateScreen();
		_stub->copyRect(24, 0, 448, h, _vid->_frontLayer, _vid->_w);
		_stub->updateScreen(0);
		_stub->initTimeStamp();

		do {
			_stub->sleep(EVENTS_DELAY);
			_stub->processEvents();
			if (_stub->_pi.escape) {
				_stub->_pi.escape = false;
				break;
			}
			if (_stub->_pi.enter) {
				_stub->_pi.enter = false;
				break;
			}
//			_stub->copyRect(24, 0, 448, h, _vid->_frontLayer, _vid->_w);
//			_stub->updateScreen(0);
			unsigned int s2 = _stub->getTimeStamp();
			if(s2>=2500)
			{
				_stub->_pi.enter = true;
			}
//			_vid->setTextPalette();
		} while (!_stub->_pi.quit);
	}
}

void Menu::handleInfoScreen() {
//	debug(DBG_MENU, "Menu::handleInfoScreen()");
	_vid->fadeOut();
	_vid->setIconsPalette();
	loadPicture("instru_e");

	do {
		_stub->sleep(EVENTS_DELAY);
		_stub->processEvents();
		if (_stub->_pi.escape) {
			_stub->_pi.escape = false;
			break;
		}
		if (_stub->_pi.enter) {
//			emu_printf("_pi.enter5\n");			
			_stub->_pi.enter = false;
			break;
		}
	} while (!_stub->_pi.quit);
}

void Menu::handleOptionsScreen() {
//	debug(DBG_MENU, "Menu::handleSkillScreen()");
	_vid->fadeOut();
	loadPicture("menu3");
	_vid->fullRefresh();
	_vid->setTextPalette(); // vbt : ajout	
	drawString("OPTIONS", 8, 4, 3);
//	drawString("SAVE (A)", 23, 4, 3);
//	int currentSkill = _skill;
	uint8_t currentLine = 0;

	// Menu data
	const char *labels[] = {
		"LANGUAGE",
		"STRETCH SCREEN",
		"VOICES",
		"MUSIC",
		"EXTRA CUTSCENES",
		"CHEATS"
	};

	const char *leftOptions[] = {
		"ENG", "YES", "YES", "YES", "YES", "YES"
	};
	const char *rightOptions[] = {
		"FRA", "NO",  "NO",  "NO",  "NO",  "NO"
	};

	const uint8_t yPositions[] = { 11, 13, 15, 17, 19, 21 };
	const uint8_t numLines = sizeof(labels) / sizeof(labels[0]);

	do {
		for (uint8_t i = 0; i < numLines; ++i) {
			drawString(labels[i], yPositions[i], 3, (currentLine == i) ? 2 : 3);
			drawString(leftOptions[i],  yPositions[i], 22, !(selected & (1 << i)) ? 2 : 3);
			drawString(rightOptions[i], yPositions[i], 27,  (selected & (1 << i)) ? 2 : 3);

			if (currentLine == i) {
				if (_stub->_pi.dirMask & PlayerInput::DIR_LEFT) {
					_stub->_pi.dirMask &= ~PlayerInput::DIR_LEFT;
					selected &= ~(1 << i);
				}
				if (_stub->_pi.dirMask & PlayerInput::DIR_RIGHT) {
					_stub->_pi.dirMask &= ~PlayerInput::DIR_RIGHT;
					selected |= (1 << i);
				}
			}
		}
		_stub->copyRect(0, 0, _vid->_w, _vid->_h, _vid->_frontLayer, _vid->_w);
		_stub->sleep(EVENTS_DELAY);
		_stub->processEvents();

		if (_stub->_pi.dirMask & PlayerInput::DIR_UP) {
			_stub->_pi.dirMask &= ~PlayerInput::DIR_UP;
			if(currentLine>0)
				currentLine--;
		}
		if (_stub->_pi.dirMask & PlayerInput::DIR_DOWN) {
			_stub->_pi.dirMask &= ~PlayerInput::DIR_DOWN;
			if(currentLine<5)
				currentLine++;
		}
		if (_stub->_pi.escape) {
			_stub->_pi.escape = false;
			break;
		}
/*		if (_stub->_pi.enter) {
			_stub->_pi.enter = false;
//			_skill = currentSkill;
			return;
		}*/
	} while (!_stub->_pi.quit);

// vbt : gestion de la langue
	if(!(selected & (1 << 0)))
		_res->setLanguage(LANG_EN);
	else
		_res->setLanguage(LANG_FR);
	
	if(!(selected & (1 << 1)))
	{
		slZoomNbg0(toFIXED(0.8), toFIXED(1.0));
		slScrWindow0(0 , 0 , 640 , 447 );

	slWindow(0 , 0 , 640 , 447 , 241 ,320 , 224);

		slScrWindow0(0 , 0 , 640 , 447 );
		slScrWindowModeNbg0(win0_IN);
		slScrWindow1(0 , 0 , 640 , 447 );
		slScrWindowModeNbg1(win1_IN);
		slScrWindowModeSPR(win0_IN);
		slScrPosNbg0(0, 0);
		SPRITE *sys_clip = (SPRITE *) SpriteVRAM;
		(*sys_clip).XC = 640;
	}
	else
	{
		slZoomNbg0(toFIXED(1.0), toFIXED(1.0));
		slScrWindow0(63 , 0 , 574 , 447 );
		slScrWindowModeNbg0(win0_IN);
		slScrWindow1(63 , 0 , 574 , 447 );
		slScrWindowModeNbg1(win1_IN);
		slScrWindowModeSPR(win0_IN);
		slScrPosNbg0(toFIXED(-63),0);
	}
	slSynch();
}

/*
void Menu::handleSkillScreen() {
//	debug(DBG_MENU, "Menu::handleSkillScreen()");

	_vid->fadeOut();
	loadPicture("menu3");
	_vid->fullRefresh();
	drawString(_res->getMenuString(LocaleData::LI_12_SKILL_LEVEL), 12, 4, 3);
	int currentSkill = _skill;
	do {
		drawString(_res->getMenuString(LocaleData::LI_13_EASY),   15, 14, (currentSkill == 0) ? 2 : 3);
		drawString(_res->getMenuString(LocaleData::LI_14_NORMAL), 17, 14, (currentSkill == 1) ? 2 : 3);
		drawString(_res->getMenuString(LocaleData::LI_15_EXPERT), 19, 14, (currentSkill == 2) ? 2 : 3);

		_vid->updateScreen();
		_stub->sleep(EVENTS_DELAY);
//		_stub->processEvents();

		if (_stub->_pi.dirMask & PlayerInput::DIR_UP) {
			_stub->_pi.dirMask &= ~PlayerInput::DIR_UP;
			switch (currentSkill) {
			case kSkillNormal:
				currentSkill = kSkillEasy;
				break;
			case kSkillExpert:
				currentSkill = kSkillNormal;
				break;
			}
		}
		if (_stub->_pi.dirMask & PlayerInput::DIR_DOWN) {
			_stub->_pi.dirMask &= ~PlayerInput::DIR_DOWN;
			switch (currentSkill) {
			case kSkillEasy:
				currentSkill = kSkillNormal;
				break;
			case kSkillNormal:
				currentSkill = kSkillExpert;
				break;
			}
		}
		if (_stub->_pi.escape) {
			_stub->_pi.escape = false;
			break;
		}
		if (_stub->_pi.enter) {
			_stub->_pi.enter = false;
			_skill = currentSkill;
			return;
		}
	} while (!_stub->_pi.quit);
	_skill = 1;
}

bool Menu::handlePasswordScreen() {
//	emu_printf("Menu::handlePasswordScreen()\n");
	_vid->fadeOut();
	_vid->_charShadowColor = _charVar1;
	_vid->_charTransparentColor = 0xFF;
	_vid->_charFrontColor = _charVar4;
	_vid->fullRefresh();
	char password[7];
	int len = 0;
	do {
		loadPicture("menu2");
		drawString2(_res->getMenuString(LocaleData::LI_16_ENTER_PASSWORD1), 15, 3);
		drawString2(_res->getMenuString(LocaleData::LI_17_ENTER_PASSWORD2), 17, 3);

		for (int i = 0; i < len; ++i) {
			drawString2((const char *)password[i], 21, i + 15);
		}
		drawString2((const char *)0x20, 21, len + 15);

		_vid->markBlockAsDirty(15 * Video::CHAR_W, 21 * Video::CHAR_H, (len + 1) * Video::CHAR_W, Video::CHAR_H, _vid->_layerScale);
		_vid->updateScreen();
		_stub->sleep(EVENTS_DELAY);
		_stub->processEvents();
		char c = _stub->_pi.lastChar;
		if (c != 0) {
			_stub->_pi.lastChar = 0;
			if (len < 6) {
				if ((c >= 'A' && c <= 'Z') || (c == 0x20)) {
					password[len] = c;
					++len;
				}
			}
		}
		if (_stub->_pi.backspace) {
			_stub->_pi.backspace = false;
			if (len > 0) {
				--len;
			}
		}
		if (_stub->_pi.escape) {
			_stub->_pi.escape = false;
			break;
		}
		if (_stub->_pi.enter) {
			_stub->_pi.enter = false;
			password[len] = '\0';
			for (int level = 0; level < 8; ++level) {
				for (int skill = 0; skill < 3; ++skill) {
					if (strcmp(getLevelPassword(level, skill), password) == 0) {
						_level = level;
						_skill = skill;
						return true;
					}
				}
			}
			return false;
		}
	} while (!_stub->_pi.quit);
	return false;
}
*/
bool Menu::handleLevelScreen() {
//	debug(DBG_MENU, "Menu::handleLevelScreen()");
	memset(_vid->_frontLayer, 0, _vid->_layerSize);
	_vid->_fullRefresh = true;
	_vid->fullRefresh();
	_vid->fadeOut();
	loadPicture("menu2");
	_stateSlot = -1;

	_vid->setTextPalette(); // vbt : ajout
	int currentSkill = _skill;
	int currentLevel = _level;
	do {
		for (int i = 0; i < LEVELS_COUNT; ++i) {
			drawString(_levelNames[i], 5 + i * 2, 4, (currentLevel == i) ? 2 : 3);
		}

		drawString(_res->getMenuString(LocaleData::LI_13_EASY),   23,  4, (currentSkill == 0) ? 2 : 3);
		drawString(_res->getMenuString(LocaleData::LI_14_NORMAL), 23, 14, (currentSkill == 1) ? 2 : 3);
		drawString(_res->getMenuString(LocaleData::LI_15_EXPERT), 23, 24, (currentSkill == 2) ? 2 : 3);

//		_vid->updateScreen();
		_stub->copyRect(0, 0, _vid->_w, _vid->_h, _vid->_frontLayer, _vid->_w);
		_stub->sleep(EVENTS_DELAY);
		_stub->processEvents();

		if (_stub->_pi.dirMask & PlayerInput::DIR_UP) {
			_stub->_pi.dirMask &= ~PlayerInput::DIR_UP;
			if (currentLevel != 0) {
				--currentLevel;
			} else {
				currentLevel = LEVELS_COUNT - 1;
			}
		}
		if (_stub->_pi.dirMask & PlayerInput::DIR_DOWN) {
			_stub->_pi.dirMask &= ~PlayerInput::DIR_DOWN;
			if (currentLevel != LEVELS_COUNT - 1) {
				++currentLevel;
			} else {
				currentLevel = 0;
			}
		}
		if (_stub->_pi.dirMask & PlayerInput::DIR_LEFT) {
			_stub->_pi.dirMask &= ~PlayerInput::DIR_LEFT;
			switch (currentSkill) {
			case kSkillNormal:
				currentSkill = kSkillEasy;
				break;
			case kSkillExpert:
				currentSkill = kSkillNormal;
				break;
			}
		}
		if (_stub->_pi.dirMask & PlayerInput::DIR_RIGHT) {
			_stub->_pi.dirMask &= ~PlayerInput::DIR_RIGHT;
			switch (currentSkill) {
			case kSkillEasy:
				currentSkill = kSkillNormal;
				break;
			case kSkillNormal:
				currentSkill = kSkillExpert;
				break;
			}
		}
		if (_stub->_pi.escape) {
			_stub->_pi.escape = false;
			break;
		}
		if (_stub->_pi.enter) {
			_stub->_pi.enter = false;
			_skill = currentSkill;
			_level = currentLevel;
			return true;
		}
	} while (!_stub->_pi.quit);
	return false;
}

bool Menu::handleResumeScreen() {
//	emu_printf("Menu::handleResumeScreen()\n");
	memset(_vid->_frontLayer, 0, _vid->_layerSize);
	_vid->_fullRefresh = true;
	_vid->fullRefresh();
	_vid->fadeOut();
	loadPicture("menu2");
	_stateSlot = -1;
	int currentSave = 0;
	int loaded = -1;

	_vid->setTextPalette(); // vbt : ajout
	SaveStateEntry sav[4];
	int num = SAT_getSaveStates(sav, false);
	_stub->_pi.quit = false;

	memset((uint8_t *)FRONT,0x00, _vid->_layerSize);

///--------------------------------------
	slPrioritySpr0(7);
	do {
		for (int i = 0; i < num; ++i) {
			char description[10];
			int level = (sav[i].comment[0] - '0') - 1;
			int room = atoi(sav[i].comment + 2);
			sprintf(description, "Room %d", room);
			
			drawString(_levelNames[level], 5 + i * 3, 4, (currentSave == i) ? 2 : 3);
			drawString( description, 6 + i  * 3, 4, (currentSave == i) ? 2 : 3);

			if (i == currentSave && loaded != currentSave){
				Color clut[384];
				_res->SAT_previewRoom(level, room, clut);

				for (int color = 0; color < 12 * 16; ++color) {
					int j = color / 16;
					if (j < 4 || j >= 8) {
						_stub->setPaletteEntry(0x100 + color, &clut[color]);
					}
				}
				loaded=currentSave;
				slSynch();
			}
		}
		
		if(num == 0)
		{
			drawString((_res->_lang == LANG_FR) ? "Aucune sauvegarde" : "No Saves", 6 + 0 * 3, 4, 3);
		}
		
		_stub->copyRect(0, 0, _vid->_w, _vid->_h, _vid->_frontLayer, _vid->_w);
		_stub->sleep(EVENTS_DELAY);
		_stub->processEvents();

		if (_stub->_pi.dirMask & PlayerInput::DIR_UP) {
			_stub->_pi.dirMask &= ~PlayerInput::DIR_UP;
			if (currentSave > 0) {
				--currentSave;
			} else {
				currentSave = num-1;
			}
		}
		if (_stub->_pi.dirMask & PlayerInput::DIR_DOWN) {
			_stub->_pi.dirMask &= ~PlayerInput::DIR_DOWN;
			if (currentSave < num-1) {
				++currentSave;
			} else {
				currentSave = 0;
			}
		}
		if (_stub->_pi.escape) {
			_stub->_pi.escape = false;
			break;
		}
		if (_stub->_pi.enter) {
			slPrioritySpr0(4);
			_stateSlot = currentSave + 1;
			_level = (sav[currentSave].comment[0] - '0') - 1;
			_stub->_pi.enter = false;
			return (num != 0);
		}
	} while (!_stub->_pi.quit);
	slPrioritySpr0(4);
	return false;
}

void Menu::handleTitleScreen() {
//	debug(DBG_MENU, "Menu::handleTitleScreen()");

	_charVar1 = 0;
	_charVar2 = 0;
	_charVar3 = 0;
	_charVar4 = 0;
	_charVar5 = 0;

	static const int MAX_MENU_ITEMS = 7;
	Item menuItems[MAX_MENU_ITEMS];
	int menuItemsCount = 0;

	menuItems[menuItemsCount].str = LocaleData::LI_07_START;
	menuItems[menuItemsCount].opt = MENU_OPTION_ITEM_START;
	++menuItemsCount;
	
	menuItems[menuItemsCount].str = LocaleData::LI_18_RESUME_GAME;
	menuItems[menuItemsCount].opt = MENU_OPTION_ITEM_RESUME;
	++menuItemsCount;
//	if (!_res->_isDemo) {
/*		if (g_options.enable_password_menu) {
			menuItems[menuItemsCount].str = LocaleData::LI_08_SKILL;
			menuItems[menuItemsCount].opt = MENU_OPTION_ITEM_SKILL;
			++menuItemsCount;
			menuItems[menuItemsCount].str = LocaleData::LI_09_PASSWORD;
			menuItems[menuItemsCount].opt = MENU_OPTION_ITEM_PASSWORD;
			++menuItemsCount;
		} else*/ {
			menuItems[menuItemsCount].str = LocaleData::LI_06_LEVEL;
			menuItems[menuItemsCount].opt = MENU_OPTION_ITEM_LEVEL;
			++menuItemsCount;
		}
//	}
	menuItems[menuItemsCount].str = LocaleData::LI_10_INFO;
	menuItems[menuItemsCount].opt = MENU_OPTION_ITEM_INFO;
	++menuItemsCount;
	menuItems[menuItemsCount].str = LocaleData::LI_24_OPTIONS;
	menuItems[menuItemsCount].opt = MENU_OPTION_ITEM_OPTIONS;
	++menuItemsCount;
	menuItems[menuItemsCount].str = LocaleData::LI_23_DEMO;
	menuItems[menuItemsCount].opt = MENU_OPTION_ITEM_DEMO;
	++menuItemsCount;
	menuItems[menuItemsCount].str = LocaleData::LI_11_QUIT;
	menuItems[menuItemsCount].opt = MENU_OPTION_ITEM_QUIT;
	++menuItemsCount;

	_selectedOption = -1;
	_nextScreen = SCREEN_TITLE;

	int currentEntry = 0;

/*
	static const struct {
		Language lang;
//		const uint8_t *bitmap16x12;
	} languages[] = {
		{ LANG_EN   },
		{ LANG_FR   },
	};
	int currentLanguage = 0;
	for (int i = 0; i < ARRAYSIZE(languages); ++i) {
		if (languages[i].lang == _res->_lang) {
			currentLanguage = i;
			break;
		}
	}*/
	_stub->initTimeStamp();

	while (!_stub->_pi.quit) {

		int selectedItem = -1;
//		int previousLanguage = currentLanguage;

		unsigned int s2 = _stub->getTimeStamp();
		if(s2>=45000)
		{
			_stub->_pi.enter = true;
			currentEntry = MENU_OPTION_ITEM_DEMO;
		}
			
		if (_nextScreen == SCREEN_TITLE) {
			memset(_vid->_frontLayer, 0, _vid->_layerSize);
			_vid->_fullRefresh = true;
			_vid->fullRefresh();
			_vid->fadeOut();
			loadPicture("menu1");

			_charVar1 = /*_res->isMac() ?*/ 0xE0 /*: 0*/; // shadowColor
			_charVar3 = /*_res->isMac() ?*/ 0xE4 /*: 1*/; // selectedColor
			_charVar4 = /*_res->isMac() ?*/ 0xE5 /*: 2*/; // defaultColor
			currentEntry = 0;
			_nextScreen = -1;
		}

//		if (g_options.enable_language_selection) 
/*		{
			if (_stub->_pi.dirMask & PlayerInput::DIR_LEFT) {
				_stub->_pi.dirMask &= ~PlayerInput::DIR_LEFT;
				if (currentLanguage != 0) {
					--currentLanguage;
				} else {
					currentLanguage = ARRAYSIZE(languages) - 1;
				}
			}
			if (_stub->_pi.dirMask & PlayerInput::DIR_RIGHT) {
				_stub->_pi.dirMask &= ~PlayerInput::DIR_RIGHT;
				if (currentLanguage != ARRAYSIZE(languages) - 1) {
					++currentLanguage;
				} else {
					currentLanguage = 0;
				}
			}
		}*/
		if (_stub->_pi.dirMask & PlayerInput::DIR_UP) {
			_stub->_pi.dirMask &= ~PlayerInput::DIR_UP;
			if (currentEntry != 0) {
				--currentEntry;
			} else {
				currentEntry = menuItemsCount - 1;
			}
			_stub->initTimeStamp();
		}
		if (_stub->_pi.dirMask & PlayerInput::DIR_DOWN) {
			_stub->_pi.dirMask &= ~PlayerInput::DIR_DOWN;
			if (currentEntry != menuItemsCount - 1) {
				++currentEntry;
			} else {
				currentEntry = 0;
			}
			_stub->initTimeStamp();
		}
		if (_stub->_pi.enter) {
			_stub->_pi.enter = false;
			selectedItem = currentEntry;
		}

		if (selectedItem != -1) {
			uint8 slot = -1;
			_selectedOption = menuItems[selectedItem].opt;
			switch (_selectedOption) {
			case MENU_OPTION_ITEM_START:
				_stateSlot = -1;
				_level = 0;
				return;
//			case MENU_OPTION_ITEM_SKILL:
//				handleSkillScreen();
//				break;
//			case MENU_OPTION_ITEM_PASSWORD:
//				if (handlePasswordScreen()) {
//					return;
//				}
//				break;
			case MENU_OPTION_ITEM_RESUME:
				if(handleResumeScreen()) {
					return;
				}
				break;
			case MENU_OPTION_ITEM_LEVEL:
				if (handleLevelScreen()) {
					return;
				}
				break;
			case MENU_OPTION_ITEM_INFO:
				handleInfoScreen();
				break;
			case MENU_OPTION_ITEM_OPTIONS:
				handleOptionsScreen();
				break;
			case MENU_OPTION_ITEM_DEMO:
				_stub->initTimeStamp();
				return;
			case MENU_OPTION_ITEM_QUIT:
				SYS_Exit(0);
				return;
			}
			_nextScreen = SCREEN_TITLE;
			_stub->initTimeStamp();
			continue;
		}

/*
		if (previousLanguage != currentLanguage) {
			_res->setLanguage(languages[currentLanguage].lang);
			// clear previous language text
			memcpy(_vid->_frontLayer, _vid->_backLayer, _vid->GAMESCREEN_W * _vid->GAMESCREEN_H * 4);
			_stub->initTimeStamp();
		}
*/
		// draw the options
		const int yPos = 26 - menuItemsCount * 2;
		for (int i = 0; i < menuItemsCount; ++i) {
			drawString(_res->getMenuString(menuItems[i].str), yPos + i * 2, 20, (i == currentEntry) ? 2 : 3);
		}
/*
		// draw the language flag in the top right corner
		if (previousLanguage != currentLanguage) {
			_stub->copyRect(0, 0, _vid->_w, _vid->_h, _vid->_frontLayer, _vid->_w);
			static const int flagW = 16;
			static const int flagH = 12;
			const int flagX = _vid->_w - flagW - 8;
			const int flagY = 8;
// VBt : ) remettre			
//			_stub->copyRectRgb24(flagX, flagY, flagW, flagH, languages[currentLanguage].bitmap16x12);
		}
		*/
//		_vid->updateScreen();
//			memset(_vid->_frontLayer,0x00,_vid->_layerSize);

//emu_printf("selectedItem %d curr %d\n",_selectedOption,);
		_stub->copyRect(0, 0, _vid->_w, _vid->_h, _vid->_frontLayer, _vid->_w);
		_stub->sleep(EVENTS_DELAY);
		_stub->processEvents();
	}
}
/*
const char *Menu::getLevelPassword(int level, int skill) const {
	switch (_res->_type) {
	case kResourceTypeAmiga:
		if (level < 7) {
			if (_res->_lang == LANG_FR) {
				return _passwordsFrAmiga[skill * 7 + level];
			} else {
				return _passwordsEnAmiga[skill * 7 + level];
			}
		}
		break;
	case kResourceTypeMac:
		return _passwordsMac[skill * LEVELS_COUNT + level];
	case kResourceTypeDOS:
		// default
		break;
	}
	return _passwordsDOS[skill * LEVELS_COUNT + level];
}
*/

int Menu::SAT_getSaveStates(SaveStateEntry* table, Bool all)
{
//emu_printf("SAT_getSaveStates\n");
    BupDir      DirTb[1];
    BupDate     DateTb;
    BupConfig   conf[3];
    
    Uint8  *rle_buf       = (Uint8  *)SCRATCH;
    Uint32 *libBakBuf     = (Uint32 *)(SCRATCH+10000);
    Uint32 *BackUpRamWork = (Uint32 *)(SCRATCH+10000+0x4000);

    PER_SMPC_RES_DIS(); // Disable reset
    BUP_Init(libBakBuf, BackUpRamWork, conf);    
    
    int num = 0;

    for (int i = 1; i < 4; i++) {
		char search[4];
		sprintf(search,"FLASHBCK_0%d",i);
		int r = BUP_Dir(0,(Uint8 *)search,1,DirTb);

		if(r | all)
		{
			if (r)
			{
				strncpy(table[num].comment, (char*)DirTb[0].comment, sizeof(table[num].comment) - 1);
				table[num].comment[sizeof(table[num].comment) - 1] = '\0';
			}
			else
			{
				strncpy(table[num].comment, "Empty", sizeof(table[num].comment) - 1);
				table[num].comment[sizeof(table[num].comment) - 1] = '\0';
			}
			num++;
		}
/*
		emu_printf("FileName :%11s\n",DirTb[0].filename);
		emu_printf("Comment  :%10s\n",DirTb[0].comment);
		emu_printf("Language :%10d\n",DirTb[0].language);
		BUP_GetDate(DirTb[0].date,&DateTb);
		emu_printf("Date     :%04d-%02d-%02d %2d:%02d\n", ((Uint16)DateTb.year)+1980, DateTb.month,DateTb.day, DateTb.time,DateTb.min);
*/
	}
    return num; // number of entries found
}