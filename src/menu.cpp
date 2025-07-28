
/*
 * REminiscence - Flashback interpreter
 * Copyright (C) 2005-2019 Gregory Montoir (cyx@users.sourceforge.net)
 */

extern "C" {
	#include 	<string.h>
#include <sl_def.h>
#include <sgl.h>
#include "sega_sys.h"
extern Uint8 *current_lwram;
}

#include "decode_mac.h"
#include "game.h"
#include "menu.h"
#include "resource.h"
#include "systemstub.h"
#include "video.h"
#include "menu.h"
#include "saturn_print.h"

#undef sleep 

Menu::Menu(Resource *res, SystemStub *stub, Video *vid)
	: _res(res), _stub(stub), _vid(vid) {
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
		_res->MAC_copyClut16(palette, 0, clutBaseColor + i);
		const int basePaletteColor = i * 16;
		for (int j = 0; j < 16; ++j) {
			_stub->setPaletteEntry(basePaletteColor + j, &palette[j]);
			_stub->setPaletteEntry(256+basePaletteColor + j, &palette[j]);
		}
	}

	if (num == kMacTitleScreen_MacPlay) {
		Color palette[16];
		for (int i = 0; i < 2; ++i) {
			_res->MAC_copyClut16(palette, 0, 55 + i);
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
	_vid->fadeOut();
	loadPicture("menu2");
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

void Menu::handleTitleScreen() {
//	debug(DBG_MENU, "Menu::handleTitleScreen()");

	_charVar1 = 0;
	_charVar2 = 0;
	_charVar3 = 0;
	_charVar4 = 0;
	_charVar5 = 0;

	static const int MAX_MENU_ITEMS = 6;
	Item menuItems[MAX_MENU_ITEMS];
	int menuItemsCount = 0;

	menuItems[menuItemsCount].str = LocaleData::LI_07_START;
	menuItems[menuItemsCount].opt = MENU_OPTION_ITEM_START;
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
	menuItems[menuItemsCount].str = LocaleData::LI_23_DEMO;
	menuItems[menuItemsCount].opt = MENU_OPTION_ITEM_DEMO;
	++menuItemsCount;
	menuItems[menuItemsCount].str = LocaleData::LI_11_QUIT;
	menuItems[menuItemsCount].opt = MENU_OPTION_ITEM_QUIT;
	++menuItemsCount;

	_selectedOption = -1;
	_nextScreen = SCREEN_TITLE;

	int currentEntry = 0;

	static const struct {
		Language lang;
//		const uint8_t *bitmap16x12;
	} languages[] = {
		{ LANG_EN   },
		{ LANG_FR   },
/*		{ LANG_DE },
		{ LANG_SP },
		{ LANG_IT },
		{ LANG_JP },*/
	};
	int currentLanguage = 0;
	for (int i = 0; i < ARRAYSIZE(languages); ++i) {
		if (languages[i].lang == _res->_lang) {
			currentLanguage = i;
			break;
		}
	}

	_stub->initTimeStamp();

	while (!_stub->_pi.quit) {

		int selectedItem = -1;
		int previousLanguage = currentLanguage;

		unsigned int s2 = _stub->getTimeStamp();
		if(s2>=45000)
		{
			_stub->_pi.enter = true;
			currentEntry = 3;
		}
			
		if (_nextScreen == SCREEN_TITLE) {
			_vid->fadeOut();
			loadPicture("menu1");
//			_vid->fullRefresh();
			_charVar1 = /*_res->isMac() ?*/ 0xE0 /*: 0*/; // shadowColor
			_charVar3 = /*_res->isMac() ?*/ 0xE4 /*: 1*/; // selectedColor
			_charVar4 = /*_res->isMac() ?*/ 0xE5 /*: 2*/; // defaultColor
			currentEntry = 0;
			_nextScreen = -1;
		}

//		if (g_options.enable_language_selection) 
		{
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
		}
		if (_stub->_pi.dirMask & PlayerInput::DIR_UP) {
			_stub->_pi.dirMask &= ~PlayerInput::DIR_UP;
			if (currentEntry != 0) {
				--currentEntry;
			} else {
				currentEntry = menuItemsCount - 1;
			}
		}
		if (_stub->_pi.dirMask & PlayerInput::DIR_DOWN) {
			_stub->_pi.dirMask &= ~PlayerInput::DIR_DOWN;
			if (currentEntry != menuItemsCount - 1) {
				++currentEntry;
			} else {
				currentEntry = 0;
			}
		}
		if (_stub->_pi.enter) {
			_stub->_pi.enter = false;
			selectedItem = currentEntry;
		}
		
		if (selectedItem != -1) {
			_selectedOption = menuItems[selectedItem].opt;
			switch (_selectedOption) {
			case MENU_OPTION_ITEM_START:
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
			case MENU_OPTION_ITEM_LEVEL:
				if (handleLevelScreen()) {
					return;
				}
				break;
			case MENU_OPTION_ITEM_INFO:
				handleInfoScreen();
				break;
			case MENU_OPTION_ITEM_DEMO:
				return;
			case MENU_OPTION_ITEM_QUIT:
				SYS_Exit(0);
				return;
			}
			_nextScreen = SCREEN_TITLE;
			continue;
		}

		if (previousLanguage != currentLanguage) {
			_res->setLanguage(languages[currentLanguage].lang);
			// clear previous language text
			memcpy(_vid->_frontLayer, _vid->_backLayer, _vid->GAMESCREEN_W * _vid->GAMESCREEN_H * 4);
		}

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
