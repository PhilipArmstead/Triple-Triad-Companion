// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once


#define MASTER_CARD_COUNT (110)
#define BOARD_SIZE (9)
#define CARDS_IN_HAND (5)
#define MAXIUMUM_POSSIBLE_MOVES (BOARD_SIZE * CARDS_IN_HAND)

/** Memory offsets */
// 110 8-byte items
// bytes 0-3 are north, south, west and east values
// byte 4 is element
//	- 0x01 = fire
//	- 0x02 = ice
//	- 0x04 = thunder
//	- 0x08 = earth
//	- 0x10 = poison
//	- 0x20 = wind
//	- 0x40 = water
//	- 0x80 = holy
// byte 5 is AI's assessment of card's strength
#define MO_CARD_LOOKUP (0x874D00)
// For a given ID, 16-bit offset from `base + 2 * (id + 1)`
// Then `base + offset`
// Decoding names:
//	- byte 0x45-0x5E;	subtract 4
//	- 0x5F-0x78;			add 2
//	- 0x21-0x2a;			subtract 0x21
//	- 0x32;						"-"
//	- 0x3C;						"&"
//	- Other bytes are unchanged
#define MO_CARD_NAME_LOOKUP (0x875074)
#define CARD_NAME_SPACE_LENGTH (0x4AE)
// mask & 0x01 = Open
// mask & 0x02 = Same
// mask & 0x04 = Plus
// mask & 0x08 = Random
// mask & 0x10 = Sudden Death
// mask & 0x42 == 42 = Same Wall (requires Same)
// mask & 0x80 = Elemental
#define MO_SPECIAL_RULES (0x19CD794)
#define SPECIAL_RULE_ELEMENTAL (0x80)
// = 1 if we're in a game
#define MO_IS_IN_CARD_GAME (0x19CD798)
// = 4 if we're choosing cards or on the victory screen;
// = 5 if we're in a game
#define MO_CARD_GAME_STATE (0x19CD7A0)
// = 1 if it's the player'sturn
#define MO_IS_MY_TURN (0x19FF420)
// 0 = None
// 1 = One
// 2 = Diff
// 3 = Direct
// 4 = All
#define MO_TRADE_RULES (0x19CD766)
// Bytes 0-4 are opponent's cards
// Bytes 5-9 are player's (0xFF while selection is in progress)
#define MO_HAND_LAYOUT (0x19CD76C)
// base + 8 * (x + 5*y)
//	- byte 0 = 0x02 if occupied
//	- byte 2 = card ID
//	- byte 3 = card record index
//	- byte 4 = current owner
//	- byte 5 = element
//		- 0x01 = Fire
//		- 0x02 = Ice
//		- 0x04 = Lightning
//		- 0x08 = Earth
//		- 0x10 = Poison
//		- 0x20 = Wind
//		- 0x40 = Water
//		- 0x80 = Holy
#define MO_BOARD (0x19FF040)
#define BOARD_SPACE_LENGTH (104)
// 0x24 byte definition of each card's ID and placement status
#define MO_HANDS (0x19FEEA8)
#define HAND_OBJECT_SIZE (0x24)
// = 1 if it is placed
// = 3 otherwise
#define MO_HAND_PLACED_BIT (0x04)
#define MO_HAND_PLACED_COLUMN (0x11)
#define MO_HAND_PLACED_ROW (0x12)
#define CARD_HANDS_SIZE (HAND_OBJECT_SIZE * CARDS_IN_HAND * 2)

static const char *positionStrings[BOARD_SIZE] = {
	"top left",
	"top centre",
	"top right",
	"mid-left",
	"mid-centre",
	"mid-right",
	"bottom left",
	"bottom centre",
	"bottom right",
};
