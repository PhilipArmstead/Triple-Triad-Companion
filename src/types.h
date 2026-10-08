// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <windows.h>

#include "constants.h"


typedef struct {
	HANDLE handle;
	BYTE *moduleBaseAddress;
} ProcessContext;

#define PROCESS_NAME "FF8_EN.exe"
#define PROCESS_NAME_LONG L"FF8_EN.exe"

typedef enum {
	GAME_STATUS_UNKNOWN,
	GAME_STATUS_DISCONNECTED,
	GAME_STATUS_NOT_IN_CARDS,
	GAME_STATUS_PRE_PHASE,
	GAME_STATUS_SELECTION_PHASE,
	GAME_STATUS_IN_GAME_WAITING,
	GAME_STATUS_IN_GAME_ACTING,
	GAME_STATUS_POST_PHASE,
} GameStatus;

typedef enum {
	PLAYER_1,
	PLAYER_2,
} Player;

// Card stuff
typedef struct {
	uint8_t north;
	uint8_t south;
	uint8_t west;
	uint8_t east;
} CardAttributes;

typedef struct {
	CardAttributes attributes[MASTER_CARD_COUNT];
	uint8_t elements[MASTER_CARD_COUNT];
	char names[MASTER_CARD_COUNT][16];
} CachedCards;

typedef struct {
	CardAttributes attributes;
	uint8_t element;
	uint8_t id;
} Card;

// Board
typedef uint8_t Cell;

typedef struct {
	uint8_t cardId;
	uint8_t position;
} Move;

// Solver
typedef uint16_t CachedEntry;

// Context
typedef struct {
	// 80 bytes; information on all cards in this match
	Card cards[CARDS_IN_HAND * 2];
	// 9 bytes; which card index is in each cell
	Cell grid[BOARD_SIZE];
	uint8_t cellElements[BOARD_SIZE];
	bool elementalRule;
	// Which of the 10 cards are in p1's hand,
	uint16_t hand1;
	// and which are in p2.
	uint16_t hand2;
	// Which of the 9 cards in the grid p1 owns,
	uint16_t owner1;
	// and which are owned by p2.
	uint16_t owner2;
	uint8_t moveCount;
	Player currentPlayer;
} GameState;

typedef struct {
	GameState state;
	CachedCards cards;
} Game;
