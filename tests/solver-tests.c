// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../src/triple-triad/game.h"
#include "../src/triple-triad/solver.h"
#include "fixtures.h"
#include "test.h"

#include <stdio.h>

#define BRUTE_FORCE_POSITIONS (8)


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

/** This test has a board that looks like the following:
 * M O O
 * - O -
 * - - -
 * It checks whether the solver is smart enough to put the next
 * card in the mid-left cell. Mid-right will win two cards but
 * mid-left will invoke the Same rule and win three.
 */
TEST(uses_the_same_rule) {
	GameState state = {0};
	for (uint8_t i = 0; i < BOARD_SIZE; ++i) {
		state.grid[i] = 0xFF;
	}

	state.cards[0].attributes = (CardAttributes){0x0A, 0x0A, 0x0A, 0x0A};
	state.cards[1].attributes = (CardAttributes){0x01, 0x01, 0x09, 0x0A};
	state.cards[2].attributes = (CardAttributes){0x01, 0x01, 0x01, 0x01};
	state.cards[3].attributes = (CardAttributes){0x0A, 0x01, 0x0A, 0x01};
	state.cards[4].attributes = (CardAttributes){0x0A, 0x0A, 0x0A, 0x0A};

	state.grid[0] = 0;
	state.grid[1] = 1;
	state.grid[2] = 2;
	state.grid[4] = 3;
	state.sameRule = true;
	state.hand1 = 1u << 4;
	state.owner1 = 1u << 0;
	state.owner2 = (1u << 1) | (1u << 2) | (1u << 3);
	state.moveCount = 8;
	state.currentPlayer = PLAYER_1;

	CachedEntry result = solver_getOptimalMove(state);
	uint8_t encodedMove = (uint8_t)(result >> 6);
	Move move = {
		.cardId = encodedMove >> 4,
		.position = encodedMove & 0x0F,
	};
	const uint8_t score = result & 0x0Fu;

	ASSERT_INT_EQ(move.cardId, 4);
	ASSERT_INT_EQ(move.position, 3);
	ASSERT_INT_EQ(score, 5);
}

/** This test has a board that looks like the following:
 * O O O
 * O O O
 * O - -
 * Whichever empty cell player 1 fills, player 2 takes it with the last
 * move, so every move scores 0. The solver must still return a legal move.
 */
TEST(returns_a_move_when_every_move_scores_zero) {
	GameState state = {0};
	state.cards[0].attributes = (CardAttributes){1, 1, 1, 1};
	state.cards[1].attributes = (CardAttributes){0x0A, 0x0A, 0x0A, 0x0A};
	for (uint8_t i = 0; i < 7; ++i) {
		state.cards[i + 2].attributes = (CardAttributes){0x0A, 0x0A, 0x0A, 0x0A};
		state.grid[i] = i + 2;
		state.owner2 |= 1u << (i + 2);
	}
	state.grid[7] = 0xFF;
	state.grid[8] = 0xFF;
	state.hand1 = 1u << 0;
	state.hand2 = 1u << 1;
	state.moveCount = 7;
	state.currentPlayer = PLAYER_1;

	const CachedEntry result = solver_getOptimalMove(state);
	const uint8_t encodedMove = (uint8_t)(result >> 6);
	const Move move = {
		.cardId = encodedMove >> 4,
		.position = encodedMove & 0x0F,
	};
	const uint8_t score = result & 0x0Fu;

	ASSERT_INT_EQ(move.cardId, 0);
	ASSERT_BOOL_EQ(move.position == 7 || move.position == 8, true);
	ASSERT_INT_EQ(score, 0);
}

/** Plain minimax with no pruning or caching, to check the solver against. */
static uint8_t bruteForceScore(const GameState state) {
	if (state.moveCount == BOARD_SIZE) {
		return game_getPlayerScore(state);
	}
	const bool maximising = state.currentPlayer == PLAYER_1;
	const uint16_t hand = maximising ? state.hand1 : state.hand2;
	uint8_t best = maximising ? 0x00 : 0xFF;
	for (uint8_t card = 0; card < CARDS_IN_HAND * 2; ++card) {
		if ((hand & (1u << card)) == 0) {
			continue;
		}
		for (uint8_t cell = 0; cell < BOARD_SIZE; ++cell) {
			if (state.grid[cell] != 0xFF) {
				continue;
			}
			const uint8_t score =
				bruteForceScore(game_placeCard(state, (Move){.cardId = card, .position = cell}));
			if (maximising ? score > best : score < best) {
				best = score;
			}
		}
	}
	return best;
}

static Move getPseudoRandomMove(const GameState state, uint32_t *seed) {
	const uint16_t hand = state.currentPlayer == PLAYER_1 ? state.hand1 : state.hand2;
	Move moves[MAXIUMUM_POSSIBLE_MOVES];
	uint8_t count = 0;
	for (uint8_t card = 0; card < CARDS_IN_HAND * 2; ++card) {
		for (uint8_t cell = 0; cell < BOARD_SIZE; ++cell) {
			if ((hand & (1u << card)) != 0 && state.grid[cell] == 0xFF) {
				moves[count++] = (Move){.cardId = card, .position = cell};
			}
		}
	}
	*seed = *seed * 1103515245u + 12345u;
	return moves[(*seed >> 16) % count];
}

/**
 * Plays a few pseudo-random opening moves under every rule combination, then
 * checks the solver's score against brute force, and that its move achieves it.
 */
TEST(matches_brute_force_search) {
	uint32_t seed = 1;
	for (size_t rules = 0; rules < fixtures_ruleSetCount; ++rules) {
		for (uint8_t i = 0; i < BRUTE_FORCE_POSITIONS; ++i) {
			GameState state = fixtures_createGame(fixtures_ruleSets[rules]);
			// Alternate between player 1 and player 2 being next to move
			const uint8_t openingMoves = 3 + (i & 1);
			while (state.moveCount < openingMoves) {
				state = game_placeCard(state, getPseudoRandomMove(state, &seed));
			}

			const CachedEntry result = solver_getOptimalMove(state);
			const Move move = fixtures_decodeMove(result);
			const uint8_t score = result & 0x0Fu;
			const uint8_t expectedScore = bruteForceScore(state);
			const bool isLegal = fixtures_isLegalMove(state, move);
			const uint8_t moveScore = isLegal ? bruteForceScore(game_placeCard(state, move)) : 0xFF;
			if (score != expectedScore || moveScore != expectedScore) {
				fprintf(stderr, "%s: position %d differs\n", fixtures_ruleSets[rules].name, i);
				ASSERT_INT_EQ(score, expectedScore);
				ASSERT_INT_EQ(moveScore, expectedScore);
			}
		}
	}
}

/**
 * Gives both players a pair of identical cards, which the solver only searches
 * once, and checks the result still matches brute force under every rule set.
 */
TEST(handles_identical_cards) {
	uint32_t seed = 7;
	for (size_t rules = 0; rules < fixtures_ruleSetCount; ++rules) {
		GameState state = fixtures_createGame(fixtures_ruleSets[rules]);
		state.cards[2] = state.cards[0];
		state.cards[3] = state.cards[1];
		while (state.moveCount < 3) {
			state = game_placeCard(state, getPseudoRandomMove(state, &seed));
		}

		const CachedEntry result = solver_getOptimalMove(state);
		const Move move = fixtures_decodeMove(result);
		const uint8_t score = result & 0x0Fu;
		const uint8_t expectedScore = bruteForceScore(state);
		const bool isLegal = fixtures_isLegalMove(state, move);
		const uint8_t moveScore = isLegal ? bruteForceScore(game_placeCard(state, move)) : 0xFF;
		if (score != expectedScore || moveScore != expectedScore) {
			fprintf(stderr, "%s: identical cards differ\n", fixtures_ruleSets[rules].name);
			ASSERT_INT_EQ(score, expectedScore);
			ASSERT_INT_EQ(moveScore, expectedScore);
		}
	}
}

void registerSolverTests(void) {
	test_run("solver returns the optimal move", test_returns_the_optimal_move);
	test_run("solver uses elements properly", test_uses_elements_properly);
	test_run("solver uses the Same rule", test_uses_the_same_rule);
	test_run("solver matches brute-force search", test_matches_brute_force_search);
	test_run("solver handles identical cards", test_handles_identical_cards);
	test_run(
		"solver returns a move when every move scores zero",
		test_returns_a_move_when_every_move_scores_zero
	);
}
