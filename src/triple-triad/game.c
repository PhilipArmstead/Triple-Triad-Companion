// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "game.h"

#include "../constants.h"
#include "../platform/memory.h"
#include "cards.h"

#include <intrin.h>
#include <stdint.h>


static void setOwnersAfterMove(GameState *state, Move move);
static void
createGameState(ProcessContext processContext, CachedCards *cache, GameState *gameState);
static int8_t getElementModifier(const GameState *state, uint8_t cardId, uint8_t position);
static bool aMatchesOrBeatsB(
	CardAttributes attributesA, Cell positionA, CardAttributes attributesB, Cell positionB
);
static void captureNeighbors(
	GameState *state,
	uint8_t position,
	uint16_t *owner1,
	uint16_t *owner2,
	uint8_t comboQueue[],
	uint8_t *queueLength
);
static void captureSame(
	GameState *state,
	uint8_t position,
	uint16_t *owner1,
	uint16_t *owner2,
	uint8_t comboQueue[],
	uint8_t *queueLength
);

Game game_init(const ProcessContext processContext) {
	CachedCards cache = cards_cache(processContext);

	GameState gameState;
	createGameState(processContext, &cache, &gameState);

	return (Game){.state = gameState, .cards = cache};
}

uint8_t game_getPlayerScore(const GameState state) {
	return (uint8_t)(__popcnt(state.owner1) + __popcnt(state.hand1));
}

GameState game_placeCard(GameState state, Move move) {
	const uint16_t cardMask = 1 << move.cardId;
	GameState newState = state;
	newState.grid[move.position] = move.cardId;

	// Remove the played card from the current player's hand
	if (state.currentPlayer == PLAYER_1) {
		newState.hand1 &= ~cardMask;
	} else {
		newState.hand2 &= ~cardMask;
	}
	setOwnersAfterMove(&newState, move);

	newState.currentPlayer = state.currentPlayer == PLAYER_1 ? PLAYER_2 : PLAYER_1;
	newState.moveCount++;

	return newState;
}

static void setOwnersAfterMove(GameState *state, Move move) {
	const uint16_t positionMask = 1 << move.cardId;
	uint16_t owner1 = state->owner1;
	uint16_t owner2 = state->owner2;
	uint8_t comboQueue[BOARD_SIZE];
	uint8_t queueLength = 0;
	if (state->currentPlayer == PLAYER_1) {
		owner1 |= positionMask;
	} else {
		owner2 |= positionMask;
	}

	// Same only applies to the placed card. If it flips anything, every card the placed card
	// flipped (including basic captures) starts a combo; otherwise nothing does.
	captureSame(state, move.position, &owner1, &owner2, comboQueue, &queueLength);
	const bool sameTriggered = queueLength > 0;
	captureNeighbors(state, move.position, &owner1, &owner2, comboQueue, &queueLength);
	if (!sameTriggered) {
		queueLength = 0;
	}
	// Combo cards only perform basic captures; they never trigger Same again
	for (uint8_t i = 0; i < queueLength; ++i) {
		captureNeighbors(state, comboQueue[i], &owner1, &owner2, comboQueue, &queueLength);
	}

	state->owner1 = owner1;
	state->owner2 = owner2;
}

static void captureNeighbors(
	GameState *state,
	const uint8_t position,
	uint16_t *owner1,
	uint16_t *owner2,
	uint8_t comboQueue[],
	uint8_t *queueLength
) {
	const uint8_t cardId = state->grid[position];
	const uint16_t opponentMask = state->currentPlayer == PLAYER_1 ? *owner2 : *owner1;
	int8_t adjacentPositions[4] = {
		(int8_t)(position - 3),
		(int8_t)(position + 3),
		(int8_t)(position % 3 != 0 ? position - 1 : -1),
		(int8_t)(position % 3 != 2 ? position + 1 : -1),
	};
	for (uint8_t i = 0; i < 4; ++i) {
		int8_t adjacentPosition = adjacentPositions[i];
		if (adjacentPosition < 0 || adjacentPosition >= BOARD_SIZE) {
			continue;
		}

		const uint8_t adjacentCardId = state->grid[adjacentPosition];
		if (adjacentCardId == 0xFF || (opponentMask & (uint16_t)(1 << adjacentCardId)) == 0) {
			continue;
		}

		if (!cards_aDefeatsB(
					state->cards[cardId].attributes,
					position,
					getElementModifier(state, cardId, position),
					state->cards[adjacentCardId].attributes,
					adjacentPosition,
					getElementModifier(state, adjacentCardId, (uint8_t)adjacentPosition)
				)) {
			continue;
		}

		const uint16_t adjacentMask = 1 << adjacentCardId;
		if (state->currentPlayer == PLAYER_1) {
			*owner1 |= adjacentMask;
			*owner2 &= ~adjacentMask;
		} else {
			*owner2 |= adjacentMask;
			*owner1 &= ~adjacentMask;
		}
		if (comboQueue != NULL) {
			comboQueue[(*queueLength)++] = (uint8_t)adjacentPosition;
		}
	}
}

static void captureSame(
	GameState *state,
	const uint8_t position,
	uint16_t *owner1,
	uint16_t *owner2,
	uint8_t comboQueue[],
	uint8_t *queueLength
) {
	if (!state->sameRule) {
		return;
	}
	const uint8_t cardId = state->grid[position];
	const uint16_t opponentMask = state->currentPlayer == PLAYER_1 ? *owner2 : *owner1;
	int8_t adjacentPositions[4] = {
		(int8_t)(position - 3),
		(int8_t)(position + 3),
		(int8_t)(position % 3 != 0 ? position - 1 : -1),
		(int8_t)(position % 3 != 2 ? position + 1 : -1),
	};
	// Matches compare raw values, ignoring element modifiers, and can be against cards of either
	// owner
	uint8_t matching = 0;
	for (uint8_t i = 0; i < 4; ++i) {
		const int8_t adjacentPosition = adjacentPositions[i];
		if (adjacentPosition < 0 || adjacentPosition >= BOARD_SIZE) {
			continue;
		}
		const uint8_t adjacentCardId = state->grid[adjacentPosition];
		if (adjacentCardId == 0xFF) {
			continue;
		}
		if (!cards_aMatchesB(
					state->cards[cardId].attributes,
					position,
					state->cards[adjacentCardId].attributes,
					(uint8_t)adjacentPosition
				)) {
			continue;
		}
		++matching;
	}
	if (matching < 2) {
		return;
	}

	// Once triggered, the game flips every opponent neighbour whose raw value is matched or
	// beaten, again ignoring element modifiers
	for (uint8_t i = 0; i < 4; ++i) {
		const int8_t adjacentPosition = adjacentPositions[i];
		if (adjacentPosition < 0 || adjacentPosition >= BOARD_SIZE) {
			continue;
		}
		const uint8_t adjacentCardId = state->grid[adjacentPosition];
		if (adjacentCardId == 0xFF || (opponentMask & (uint16_t)(1 << adjacentCardId)) == 0) {
			continue;
		}
		if (!aMatchesOrBeatsB(
					state->cards[cardId].attributes,
					position,
					state->cards[adjacentCardId].attributes,
					(uint8_t)adjacentPosition
				)) {
			continue;
		}
		const uint16_t adjacentMask = 1 << adjacentCardId;
		if (state->currentPlayer == PLAYER_1) {
			*owner1 |= adjacentMask;
			*owner2 &= ~adjacentMask;
		} else {
			*owner2 |= adjacentMask;
			*owner1 &= ~adjacentMask;
		}
		comboQueue[(*queueLength)++] = (uint8_t)adjacentPosition;
	}
}

static bool aMatchesOrBeatsB(
	const CardAttributes attributesA,
	const Cell positionA,
	const CardAttributes attributesB,
	const Cell positionB
) {
	return cards_aMatchesB(attributesA, positionA, attributesB, positionB) ||
		cards_aDefeatsB(attributesA, positionA, 0, attributesB, positionB, 0);
}

static int8_t
getElementModifier(const GameState *state, const uint8_t cardId, const uint8_t position) {
	if (!state->elementalRule || state->cellElements[position] == 0) {
		return 0;
	}
	return state->cards[cardId].element == state->cellElements[position] ? 1 : -1;
}

static void
createGameState(const ProcessContext processContext, CachedCards *cache, GameState *gameState) {
	// Create initial struct
	const uint8_t filledHand = 0b11111;
	*gameState = (GameState){
		// Nobody owns anything in the grid if the grid is empty
		.owner1 = 0,
		.owner2 = 0,
		// Player 1 owns the last five cards in the array
		.hand1 = filledHand << 5,
		// Player 2 owns the first five
		.hand2 = filledHand,
		.currentPlayer = PLAYER_1,
	};
	const uint8_t specialRules =
		readByte(processContext.handle, processContext.moduleBaseAddress + MO_SPECIAL_RULES);
	gameState->sameRule = (specialRules & SPECIAL_RULE_SAME) != 0;
	gameState->elementalRule = (specialRules & SPECIAL_RULE_ELEMENTAL) != 0;

	// Cache in-use cards + attributes + availability
	{
		uint8_t buffer[CARDS_IN_HAND * 2];
		readFromMemory(
			processContext.handle,
			processContext.moduleBaseAddress + MO_HAND_LAYOUT,
			sizeof(buffer),
			buffer
		);
		for (int i = 0; i < CARDS_IN_HAND * 2; ++i) {
			uint8_t id = buffer[i];
			gameState->cards[i] = (Card){
				.id = id,
				.attributes = cache->attributes[id],
				.element = cache->elements[id],
			};
		}
	}

	// Cache grid state
	uint8_t buffer[BOARD_SPACE_LENGTH];
	readFromMemory(
		processContext.handle, processContext.moduleBaseAddress + MO_BOARD, sizeof(buffer), buffer
	);
	// For each cell
	for (int y = 0; y < 3; ++y) {
		for (int x = 0; x < 3; ++x) {
			uint8_t o = (uint8_t)(8 * (x + 5 * y));
			// Store card index in our grid object
			bool isOccupied = buffer[o] & 0x02;
			uint8_t recordIndex = buffer[o + 3];
			gameState->grid[x + y * 3] = isOccupied ? recordIndex : 0xFF;
			gameState->cellElements[x + y * 3] = buffer[o + 5];

			// If there's a card in this cell
			if (isOccupied) {
				++gameState->moveCount;

				// Nobody is holding that card any more
				gameState->hand1 &= ~(1 << recordIndex);
				gameState->hand2 &= ~(1 << recordIndex);

				bool isMine = buffer[o + 4] == 0x01;
				if (isMine) {
					gameState->owner1 |= (1 << recordIndex);
					gameState->owner2 &= ~(1 << recordIndex);
				} else {
					gameState->owner2 |= (1 << recordIndex);
					gameState->owner1 &= ~(1 << recordIndex);
				}
			}
		}
	}
}
