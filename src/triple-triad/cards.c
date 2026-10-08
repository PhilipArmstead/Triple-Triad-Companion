// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cards.h"

#include "../platform/memory.h"

#include <stddef.h>


static char decodeCharacter(uint8_t code);

CachedCards cards_cache(ProcessContext context) {
	CachedCards cache = {0};

	{
		uint8_t buffer[MASTER_CARD_COUNT * 8];
		readFromMemory(
			context.handle, context.moduleBaseAddress + MO_CARD_LOOKUP, sizeof(buffer), buffer
		);
		for (int i = 0; i < MASTER_CARD_COUNT; ++i) {
			cache.attributes[i].north = buffer[(ptrdiff_t)i * 8 + 0];
			cache.attributes[i].south = buffer[(ptrdiff_t)i * 8 + 1];
			cache.attributes[i].west = buffer[(ptrdiff_t)i * 8 + 2];
			cache.attributes[i].east = buffer[(ptrdiff_t)i * 8 + 3];
			cache.elements[i] = buffer[(ptrdiff_t)i * 8 + 4];
		}
	}

	uint8_t buffer[CARD_NAME_SPACE_LENGTH] = {0};
	readFromMemory(
		context.handle, context.moduleBaseAddress + MO_CARD_NAME_LOOKUP, sizeof(buffer), buffer
	);
	for (int i = 0; i < MASTER_CARD_COUNT; ++i) {
		int o = 2 * (i + 1);
		uint16_t offset = buffer[o] + buffer[o + 1] * 0x100;
		for (int j = 0; j < 16; ++j) {
			uint8_t letter = buffer[offset + j];
			if (letter) {
				cache.names[i][j] = decodeCharacter(letter);
			} else {
				break;
			}
		}
	}

	return cache;
}

bool cards_aDefeatsB(
	const CardAttributes attributesA,
	const Cell positionA,
	const int8_t modifierA,
	const CardAttributes attributesB,
	const Cell positionB,
	const int8_t modifierB
) {
	int8_t diff = (int8_t)(positionA - positionB);
	switch (diff) {
		case 3: return attributesA.north + modifierA > attributesB.south + modifierB;
		case -3: return attributesA.south + modifierA > attributesB.north + modifierB;
		case 1: return attributesA.west + modifierA > attributesB.east + modifierB;
		case -1: return attributesA.east + modifierA > attributesB.west + modifierB;
		default: return false;
	}
}

static char decodeCharacter(uint8_t code) {
	if (code == 0x32) {
		return '-';
	}
	if (code == 0x3C) {
		return '&';
	}
	if (code >= 0x45 && code <= 0x5E) {
		return (char)(code - 4);
	}
	if (code >= 0x5F && code <= 0x78) {
		return (char)(code + 2);
	}
	if (code >= 0x21 && code <= 0x2A) {
		return (char)(code + 0x0F);
	}
	return (char)code;
}
