// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../src/triple-triad/game.h"
#include "../src/triple-triad/solver.h"
#include "fixtures.h"
#include "test.h"

#include <stdio.h>


#define PERFORMANCE_ITERATIONS (3)

/**
 * Plays a whole game with both players following the solver. Optimal play
 * shouldn't change the predicted outcome, so every round's prediction and the
 * final score must match the first round's prediction.
 */
static void playOptimalGame(const RuleSet rules) {
	GameState state = fixtures_createGame(rules);
	uint8_t expectedScore = 0;

	while (state.moveCount < BOARD_SIZE) {
		const CachedEntry result = solver_getOptimalMove(state);
		const Move move = fixtures_decodeMove(result);
		const uint8_t score = result & 0x0Fu;
		if (state.moveCount == 0) {
			expectedScore = score;
		}
		if (score != expectedScore || !fixtures_isLegalMove(state, move)) {
			fprintf(stderr, "%s: failed on move %d\n", rules.name, state.moveCount + 1);
			ASSERT_INT_EQ(score, expectedScore);
			ASSERT_BOOL_EQ(fixtures_isLegalMove(state, move), true);
			return;
		}
		state = game_placeCard(state, move);
	}

	const uint8_t finalScore = game_getPlayerScore(state);
	if (finalScore != expectedScore) {
		fprintf(stderr, "%s: final score differs from prediction\n", rules.name);
		ASSERT_INT_EQ(finalScore, expectedScore);
	}
}

TEST(solves_every_rule_combination) {
	LARGE_INTEGER frequency;
	LARGE_INTEGER start;
	LARGE_INTEGER end;
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&start);

	for (int iteration = 0; iteration < PERFORMANCE_ITERATIONS; ++iteration) {
		for (size_t i = 0; i < fixtures_ruleSetCount; ++i) {
			playOptimalGame(fixtures_ruleSets[i]);
		}
	}

	QueryPerformanceCounter(&end);
	const double elapsedMs =
		(double)(end.QuadPart - start.QuadPart) * 1000.0 / (double)frequency.QuadPart;
	printf(
		"%d iterations of %zu games in %.1f ms (%.1f ms per iteration)\n",
		PERFORMANCE_ITERATIONS,
		fixtures_ruleSetCount,
		elapsedMs,
		elapsedMs / PERFORMANCE_ITERATIONS
	);
}

void registerPerformanceTests(void) {
	test_run("solver solves every rule combination", test_solves_every_rule_combination);
}
