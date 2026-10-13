// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "fixtures.h"


const RuleSet fixtures_ruleSets[] = {
	{"None", false, false, false, false},
	{"Same", true, false, false, false},
	{"Plus", false, false, true, false},
	{"Same + Plus", true, false, true, false},
	{"Same + Wall", true, true, false, false},
	{"Same + Plus + Wall", true, true, true, false},
	{"Elemental", false, false, false, true},
	{"Same + Elemental", true, false, false, true},
	{"Plus + Elemental", false, false, true, true},
	{"Same + Plus + Elemental", true, false, true, true},
	{"Same + Wall + Elemental", true, true, false, true},
	{"Same + Plus + Wall + Elemental", true, true, true, true},
};
const size_t fixtures_ruleSetCount = sizeof fixtures_ruleSets / sizeof fixtures_ruleSets[0];

// The first 10 cards in the game's card table
static const Card firstCards[CARDS_IN_HAND * 2] = {
	{.id = 0, .attributes = {1, 1, 5, 4}},                  // Geezard
	{.id = 1, .attributes = {5, 1, 3, 1}},                  // Funguar
	{.id = 2, .attributes = {1, 3, 5, 3}},                  // Bite Bug
	{.id = 3, .attributes = {6, 1, 2, 1}},                  // Red Bat
	{.id = 4, .attributes = {2, 1, 5, 3}},                  // Blobra
	{.id = 5, .attributes = {2, 4, 4, 1}, .element = 0x04}, // Gayla (thunder)
	{.id = 6, .attributes = {1, 4, 1, 5}},                  // Gesper
	{.id = 7, .attributes = {3, 2, 1, 5}, .element = 0x08}, // Fastitocalon-F (earth)
	{.id = 8, .attributes = {2, 6, 1, 1}},                  // Blood Soul
	{.id = 9, .attributes = {4, 4, 3, 2}},                  // Caterchipillar
};

GameState fixtures_createGame(const RuleSet rules) {
	GameState state = {0};
	for (uint8_t i = 0; i < BOARD_SIZE; ++i) {
		state.grid[i] = 0xFF;
	}
	for (uint8_t i = 0; i < CARDS_IN_HAND * 2; ++i) {
		state.cards[i] = firstCards[i];
	}
	// The cards alternate between the players, starting with player 1
	state.hand1 = 0x155;
	state.hand2 = 0x2AA;
	state.sameRule = rules.same;
	state.sameWallRule = rules.sameWall;
	state.plusRule = rules.plus;
	state.elementalRule = rules.elemental;
	if (rules.elemental) {
		state.cellElements[0] = 0x04;
		state.cellElements[4] = 0x08;
		state.cellElements[8] = 0x01;
	}
	state.currentPlayer = PLAYER_1;
	return state;
}

bool fixtures_isLegalMove(const GameState state, const Move move) {
	const uint16_t hand = state.currentPlayer == PLAYER_1 ? state.hand1 : state.hand2;
	return move.cardId < CARDS_IN_HAND * 2 && (hand & (1u << move.cardId)) != 0 &&
				 move.position < BOARD_SIZE && state.grid[move.position] == 0xFF;
}

Move fixtures_decodeMove(const CachedEntry entry) {
	const uint8_t encodedMove = (uint8_t)(entry >> 6);
	return (Move){
		.cardId = encodedMove >> 4,
		.position = encodedMove & 0x0F,
	};
}
