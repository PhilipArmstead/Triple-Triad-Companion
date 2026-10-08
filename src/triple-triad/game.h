// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../types.h"


Game game_init(ProcessContext processContext);
uint8_t game_getPlayerScore(GameState state);
GameState game_placeCard(GameState state, Move move);
