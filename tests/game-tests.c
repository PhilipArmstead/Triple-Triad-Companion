// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../src/triple-triad/game.h"
#include "test.h"


TEST(score_counts_owned_cards_and_remaining_hand) {
	const GameState state = {
		.owner1 = (1u << 2) | (1u << 8),
		.hand1 = 1u << 3,
	};

	ASSERT_INT_EQ(game_getPlayerScore(state), 3);
}

static GameState createSameState(void) {
	GameState state = {0};
	for (uint8_t i = 0; i < BOARD_SIZE; ++i) {
		state.grid[i] = 0xFF;
	}
	state.sameRule = true;
	state.hand1 = 1u << 0;
	state.currentPlayer = PLAYER_1;
	return state;
}

/** Board before placing card 0 in the centre:
 * O O O
 * O - -
 * - - -
 * Same flips the top-centre and mid-left cards. Top-centre then matches
 * both of its neighbours, but combo cards only perform basic captures,
 * so those neighbours must stay with the opponent.
 */
TEST(combo_cards_do_not_trigger_same) {
	GameState state = createSameState();
	state.cards[0].attributes = (CardAttributes){5, 1, 5, 1};
	state.cards[1].attributes = (CardAttributes){1, 5, 3, 3};
	state.cards[2].attributes = (CardAttributes){1, 1, 1, 5};
	state.cards[3].attributes = (CardAttributes){1, 1, 3, 1};
	state.cards[4].attributes = (CardAttributes){1, 1, 1, 3};
	state.grid[1] = 1;
	state.grid[3] = 2;
	state.grid[2] = 3;
	state.grid[0] = 4;
	state.owner2 = (1u << 1) | (1u << 2) | (1u << 3) | (1u << 4);

	const GameState result = game_placeCard(state, (Move){.cardId = 0, .position = 4});

	ASSERT_INT_EQ(result.owner1, (1u << 0) | (1u << 1) | (1u << 2));
	ASSERT_INT_EQ(result.owner2, (1u << 3) | (1u << 4));
}

/** Board before placing card 0 in the centre:
 * - O O
 * O - O
 * - - -
 * Same flips top-centre and mid-left; mid-right is a basic capture. Because
 * Same fired, the basic capture also combos and takes the top-right card.
 */
TEST(basic_captures_combo_when_same_triggers) {
	GameState state = createSameState();
	state.cards[0].attributes = (CardAttributes){5, 1, 5, 9};
	state.cards[1].attributes = (CardAttributes){1, 5, 1, 1};
	state.cards[2].attributes = (CardAttributes){1, 1, 1, 5};
	state.cards[3].attributes = (CardAttributes){9, 1, 1, 1};
	state.cards[4].attributes = (CardAttributes){1, 1, 9, 1};
	state.grid[1] = 1;
	state.grid[3] = 2;
	state.grid[5] = 3;
	state.grid[2] = 4;
	state.owner2 = (1u << 1) | (1u << 2) | (1u << 3) | (1u << 4);

	GameState result = game_placeCard(state, (Move){.cardId = 0, .position = 4});

	ASSERT_INT_EQ(result.owner1, 0x1F);
	ASSERT_INT_EQ(result.owner2, 0);

	// Without a second match Same doesn't fire, so nothing combos
	state.cards[1].attributes.south = 4;
	result = game_placeCard(state, (Move){.cardId = 0, .position = 4});

	ASSERT_INT_EQ(result.owner1, (1u << 0) | (1u << 1) | (1u << 3));
	ASSERT_INT_EQ(result.owner2, (1u << 2) | (1u << 4));
}

/** Board before placing card 0 in the centre:
 * - M O
 * M - O
 * - - -
 * Both Same matches are my own cards and the weaker mid-right card doesn't
 * match. The game still triggers Same and flips mid-right, which then combos
 * into the top-right card.
 */
TEST(same_matches_can_be_own_cards) {
	GameState state = createSameState();
	state.cards[0].attributes = (CardAttributes){5, 1, 5, 5};
	state.cards[1].attributes = (CardAttributes){1, 5, 1, 1};
	state.cards[2].attributes = (CardAttributes){1, 1, 1, 5};
	state.cards[3].attributes = (CardAttributes){9, 1, 4, 1};
	state.cards[4].attributes = (CardAttributes){1, 1, 1, 1};
	state.grid[1] = 1;
	state.grid[3] = 2;
	state.grid[5] = 3;
	state.grid[2] = 4;
	state.owner1 = (1u << 1) | (1u << 2);
	state.owner2 = (1u << 3) | (1u << 4);

	const GameState result = game_placeCard(state, (Move){.cardId = 0, .position = 4});

	ASSERT_INT_EQ(result.owner1, 0x1F);
	ASSERT_INT_EQ(result.owner2, 0);
}

/** Board before placing card 0 in the centre (on a mismatched element):
 * - O -
 * O - O
 * - - -
 * The element penalty means card 0 can't take mid-right with a basic capture,
 * but Same compares raw values and flips it anyway.
 */
TEST(same_flips_using_raw_values) {
	GameState state = createSameState();
	state.elementalRule = true;
	state.cellElements[4] = 0x01;
	state.cards[0].attributes = (CardAttributes){5, 1, 5, 5};
	state.cards[0].element = 0x02;
	state.cards[1].attributes = (CardAttributes){1, 5, 1, 1};
	state.cards[2].attributes = (CardAttributes){1, 1, 1, 5};
	state.cards[3].attributes = (CardAttributes){1, 1, 4, 1};
	state.grid[1] = 1;
	state.grid[3] = 2;
	state.grid[5] = 3;
	state.owner2 = (1u << 1) | (1u << 2) | (1u << 3);

	const GameState result = game_placeCard(state, (Move){.cardId = 0, .position = 4});

	ASSERT_INT_EQ(result.owner1, 0x0F);
	ASSERT_INT_EQ(result.owner2, 0);
}

/** Board before placing card 0 in the top centre:
 * - - -
 * - O -
 * - - -
 * Card 0's north side is an A facing the wall, and its south side matches
 * the centre card. With Same Wall that is two matches, so the centre flips.
 */
TEST(same_wall_counts_an_a_facing_the_wall) {
	GameState state = createSameState();
	state.sameWallRule = true;
	state.cards[0].attributes = (CardAttributes){10, 5, 1, 1};
	state.cards[1].attributes = (CardAttributes){5, 1, 1, 1};
	state.grid[4] = 1;
	state.owner2 = 1u << 1;

	GameState result = game_placeCard(state, (Move){.cardId = 0, .position = 1});

	ASSERT_INT_EQ(result.owner1, (1u << 0) | (1u << 1));
	ASSERT_INT_EQ(result.owner2, 0);

	// Only an A matches the wall
	state.cards[0].attributes.north = 9;
	result = game_placeCard(state, (Move){.cardId = 0, .position = 1});

	ASSERT_INT_EQ(result.owner1, 1u << 0);
	ASSERT_INT_EQ(result.owner2, 1u << 1);

	// Without Same Wall the wall is ignored
	state.cards[0].attributes.north = 10;
	state.sameWallRule = false;
	result = game_placeCard(state, (Move){.cardId = 0, .position = 1});

	ASSERT_INT_EQ(result.owner1, 1u << 0);
	ASSERT_INT_EQ(result.owner2, 1u << 1);
}

static GameState createPlusState(void) {
	GameState state = createSameState();
	state.sameRule = false;
	state.plusRule = true;
	return state;
}

/** Board before placing card 0 in the centre:
 * O O -
 * O - -
 * - - -
 * Card 0 loses to both the top-centre and mid-left cards, but the touching
 * sides add up to 10 with each, so Plus flips them. Top-centre then combos
 * into the top-left card.
 */
TEST(plus_flips_cards_with_equal_sums) {
	GameState state = createPlusState();
	state.cards[0].attributes = (CardAttributes){3, 1, 4, 1};
	state.cards[1].attributes = (CardAttributes){1, 7, 9, 1};
	state.cards[2].attributes = (CardAttributes){1, 1, 1, 6};
	state.cards[3].attributes = (CardAttributes){1, 1, 1, 1};
	state.grid[1] = 1;
	state.grid[3] = 2;
	state.grid[0] = 3;
	state.owner2 = (1u << 1) | (1u << 2) | (1u << 3);

	GameState result = game_placeCard(state, (Move){.cardId = 0, .position = 4});

	ASSERT_INT_EQ(result.owner1, 0x0F);
	ASSERT_INT_EQ(result.owner2, 0);

	// One of the pair can be my own card
	state.owner1 = 1u << 2;
	state.owner2 = (1u << 1) | (1u << 3);
	result = game_placeCard(state, (Move){.cardId = 0, .position = 4});

	ASSERT_INT_EQ(result.owner1, 0x0F);
	ASSERT_INT_EQ(result.owner2, 0);
}

/** Board before placing card 0 in the centre:
 * - O -
 * O - O
 * - O -
 * The sums are 10 (north), 12 (south), 12 (west) and 10 (east). Checking
 * north, south, west then east, the 12s pair up first, so only those flip.
 */
TEST(plus_uses_only_the_first_pair_to_form) {
	GameState state = createPlusState();
	state.cards[0].attributes = (CardAttributes){1, 2, 3, 4};
	state.cards[1].attributes = (CardAttributes){1, 9, 1, 1};
	state.cards[2].attributes = (CardAttributes){10, 1, 1, 1};
	state.cards[3].attributes = (CardAttributes){1, 1, 1, 9};
	state.cards[4].attributes = (CardAttributes){1, 1, 6, 1};
	state.grid[1] = 1;
	state.grid[7] = 2;
	state.grid[3] = 3;
	state.grid[5] = 4;
	state.owner2 = 0x1E;

	const GameState result = game_placeCard(state, (Move){.cardId = 0, .position = 4});

	ASSERT_INT_EQ(result.owner1, (1u << 0) | (1u << 2) | (1u << 3));
	ASSERT_INT_EQ(result.owner2, (1u << 1) | (1u << 4));
}

void registerGameTests(void) {
	test_run(
		"score counts owned cards and remaining hand", test_score_counts_owned_cards_and_remaining_hand
	);
	test_run("combo cards do not trigger Same", test_combo_cards_do_not_trigger_same);
	test_run(
		"basic captures combo when Same triggers", test_basic_captures_combo_when_same_triggers
	);
	test_run("Same matches can be own cards", test_same_matches_can_be_own_cards);
	test_run("Same flips using raw values", test_same_flips_using_raw_values);
	test_run("Same Wall counts an A facing the wall", test_same_wall_counts_an_a_facing_the_wall);
	test_run("Plus flips cards with equal sums", test_plus_flips_cards_with_equal_sums);
	test_run("Plus uses only the first pair to form", test_plus_uses_only_the_first_pair_to_form);
}
