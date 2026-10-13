// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../src/types.h"

#include <stddef.h>


typedef struct {
	const char *name;
	bool same;
	bool sameWall;
	bool plus;
	bool elemental;
} RuleSet;

// Every valid combination of rules (Same Wall requires Same)
extern const RuleSet fixtures_ruleSets[];
extern const size_t fixtures_ruleSetCount;

/** An empty board with the first 10 cards in the game's card table dealt alternately. */
GameState fixtures_createGame(RuleSet rules);
bool fixtures_isLegalMove(GameState state, Move move);
Move fixtures_decodeMove(CachedEntry entry);
