// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../src/triple-triad/game.h"
#include "test.h"

TEST(score_counts_owned_cards_and_remaining_hand) {
	const GameState state = {
		.owner1 = (1u << 2) | (1u << 8),
		.hand1 = 1u << 3,
	};

	ASSERT_INT_EQ(3, game_getPlayerScore(state));
}

void register_game_tests(void) {
	test_run(
		"score counts owned cards and remaining hand", test_score_counts_owned_cards_and_remaining_hand
	);
}
