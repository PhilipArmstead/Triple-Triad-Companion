// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../src/triple-triad/game.h"
#include "../src/triple-triad/solver.h"
#include "test.h"

TEST(returns_the_optimal_move) {
	GameState state = {0};
	for (uint8_t i = 0; i < BOARD_SIZE; ++i) {
		state.grid[i] = 0xFF;
	}

	state.grid[0] = 2;
	state.grid[1] = 3;
	state.grid[2] = 5;
	state.grid[3] = 6;
	state.grid[4] = 4;
	state.grid[5] = 7;
	state.grid[6] = 9;
	state.hand1 = 1u << 8;
	state.hand2 = (1u << 0) | (1u << 1);
	state.owner1 = (1u << 2) | (1u << 3) | (1u << 5) | (1u << 6) | (1u << 7) | (1u << 9);
	state.owner2 = 1u << 4;
	state.moveCount = 7;
	state.currentPlayer = PLAYER_1;

	state.cards[8].attributes.north = 9;
	state.cards[8].attributes.east = 9;
	state.cards[8].attributes.west = 9;
	state.cards[4].attributes.south = 1;

	const CachedEntry result = solver_getOptimalMove(state);
	const uint8_t encodedMove = (uint8_t)(result >> 6);
	const Move move = {
		.cardId = encodedMove >> 4,
		.position = encodedMove & 0x0F,
	};

	ASSERT_INT_EQ(8, move.cardId);
	ASSERT_INT_EQ(7, move.position);

	const GameState bestMove = game_placeCard(state, (Move){.cardId = 8, .position = 7});
	const GameState otherMove = game_placeCard(state, (Move){.cardId = 8, .position = 8});
	ASSERT_BOOL_EQ(true, game_getPlayerScore(bestMove) > game_getPlayerScore(otherMove));
}

void register_solver_tests(void) {
	test_run("solver returns the optimal move", test_returns_the_optimal_move);
}
