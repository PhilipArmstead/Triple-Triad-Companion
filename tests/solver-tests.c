// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../src/triple-triad/game.h"
#include "../src/triple-triad/solver.h"
#include "test.h"


/** This test has a board that looks like the following:
 * M O -
 * O - -
 * - - -
 * It checks whether the solver is smart enough to put the weaker
 * card in the middle, knowing the stronger card needs to be saved
 * to take back on the next turn in order to maximise the score.
 * (M is my card, O is opponent's on the board above, by the way.)
 */
TEST(returns_the_optimal_move) {
	GameState state = {0};
	for (uint8_t i = 0; i < BOARD_SIZE; ++i) {
		state.grid[i] = 0xFF;
	}

	state.cards[0].attributes = (CardAttributes){0x0A, 0x0A, 0x0A, 0x0A};
	state.cards[1].attributes = (CardAttributes){0x0A, 0x01, 0x0A, 0x0A};
	state.cards[2].attributes = (CardAttributes){0x0A, 0x01, 0x0A, 0x01};
	state.cards[3].attributes = (CardAttributes){0x04, 0x04, 0x04, 0x04};
	state.cards[4].attributes = (CardAttributes){0x03, 0x03, 0x03, 0x03};
	state.cards[5].attributes = (CardAttributes){0x03, 0x02, 0x02, 0x03};

	state.grid[0] = 0;
	state.grid[1] = 1;
	state.grid[3] = 2;
	state.hand1 = (1u << 3) | (1u << 4);
	state.hand2 = 1u << 5;
	state.owner1 = 1u << 0;
	state.owner2 = (1u << 1) | (1u << 2);
	state.moveCount = 6;
	state.currentPlayer = PLAYER_1;

	CachedEntry result = solver_getOptimalMove(state);
	uint8_t encodedMove = (uint8_t)(result >> 6);
	Move move = {
		.cardId = encodedMove >> 4,
		.position = encodedMove & 0x0F,
	};
	uint8_t score = result & 0x0Fu;

	ASSERT_INT_EQ(move.cardId, 4);
	ASSERT_INT_EQ(move.position, 4);
	ASSERT_INT_EQ(score, 5);

	const GameState bestMove = game_placeCard(state, move);
	const GameState followupBestMove = game_placeCard(bestMove, (Move){.cardId = 5, .position = 6});

	result = solver_getOptimalMove(followupBestMove);
	encodedMove = (uint8_t)(result >> 6);
	move.cardId = encodedMove >> 4;
	move.position = encodedMove & 0x0F;
	score = result & 0x0Fu;

	ASSERT_INT_EQ(move.cardId, 3);
	ASSERT_INT_EQ(move.position, 7);
	ASSERT_INT_EQ(score, 5);
}

/** This test has a board that looks like the following:
 * - E -
 * E O E
 * - E -
 * It checks whether the solver is smart enough to put the next
 * card in the mid-right cell, as that's the one that will buff
 * the card's attributes to the point where it can take the centre.
 */
TEST(uses_elements_properly) {
	GameState state = {0};
	for (uint8_t i = 0; i < BOARD_SIZE; ++i) {
		state.grid[i] = 0xFF;
	}

	state.cards[0].attributes = (CardAttributes){0x05, 0x05, 0x05, 0x05};
	state.cards[1].attributes = (CardAttributes){0x05, 0x05, 0x05, 0x04};

	state.grid[4] = 0;
	state.cellElements[1] = 0x01;
	state.cellElements[3] = 0x02;
	state.cellElements[5] = 0x04;
	state.cellElements[7] = 0x08;
	state.elementalRule = true;
	state.cards[1].element = 0x04;
	state.hand1 = 1u << 1;
	state.owner2 = 1u << 0;
	state.moveCount = 1;
	state.currentPlayer = PLAYER_1;

	CachedEntry result = solver_getOptimalMove(state);
	uint8_t encodedMove = (uint8_t)(result >> 6);
	Move move = {
		.cardId = encodedMove >> 4,
		.position = encodedMove & 0x0F,
	};

	ASSERT_INT_EQ(move.cardId, 1);
	ASSERT_INT_EQ(move.position, 5);
}

void registerSolverTests(void) {
	test_run("solver returns the optimal move", test_returns_the_optimal_move);
	test_run("solver uses elements properly", test_uses_elements_properly);
}
