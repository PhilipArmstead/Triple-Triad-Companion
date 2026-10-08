// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "states.h"

#include "platform/memory.h"
#include "triple-triad/game.h"
#include "triple-triad/solver.h"
#include "types.h"

#include <stdio.h>


static bool virtualTerminalEnabled;

static void clearConsole(void);
static void printTitle(void);

#define APPLY_TERMINAL_STYLES(styles)                                                              \
	if (virtualTerminalEnabled)                                                                      \
		printf(styles);

void state_init(void) {
	HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD consoleMode;
	if (GetConsoleMode(output, &consoleMode)) {
		virtualTerminalEnabled =
			SetConsoleMode(output, consoleMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
	}
}

void state_showTitle(void) {
	clearConsole();
	printTitle();
}

void state_handleDisconnected(GameStatus *status) {
	if (*status != GAME_STATUS_DISCONNECTED) {
		*status = GAME_STATUS_DISCONNECTED;
		state_showTitle();
		printf("Disconnected; is FF8_EN.exe running?\n");
	}
}

void state_handleNotInCards(GameStatus *status) {
	if (*status != GAME_STATUS_NOT_IN_CARDS) {
		*status = GAME_STATUS_NOT_IN_CARDS;
		state_showTitle();
		printf("Waiting for a card game to begin\n");
	}
}

void state_handleSelectionPhase(GameStatus *status) {
	if (*status != GAME_STATUS_SELECTION_PHASE) {
		*status = GAME_STATUS_SELECTION_PHASE;
	}
}

void state_handleInGame(GameStatus *status, ProcessContext context) {
	bool isMyTurn = readByte(context.handle, context.moduleBaseAddress + MO_IS_MY_TURN) == 1;

	if (!isMyTurn) {
		*status = GAME_STATUS_IN_GAME_WAITING;
		return;
	}

	if (*status != GAME_STATUS_IN_GAME_ACTING) {
		*status = GAME_STATUS_IN_GAME_ACTING;

		const Game game = game_init(context);

		if (game.state.moveCount <= 1) {
			state_showTitle();
		}

		const CachedEntry result = solver_getOptimalMove(game.state);
		const uint8_t score = result & 0x0Fu;
		const uint8_t move = (uint8_t)(result >> 6);
		const uint8_t position = move & 0xF;
		const uint8_t cardId = move >> 4;

		printf("Play ");
		APPLY_TERMINAL_STYLES("\x1b[35m");
		printf("%s", game.cards.names[game.state.cards[cardId].id]);
		APPLY_TERMINAL_STYLES("\x1b[0m");
		printf(" at ");
		APPLY_TERMINAL_STYLES("\x1b[33m");
		printf("%s", positionStrings[position]);
		APPLY_TERMINAL_STYLES("\x1b[0m");
		const int8_t relativeScore = (int8_t)(score - CARDS_IN_HAND);
		if (relativeScore == 0) {
			printf(" for an expected draw");
		} else {
			printf(" for an expected result of ");
			if (relativeScore > 0) {
				APPLY_TERMINAL_STYLES("\x1b[1;32m");
				printf("+%d", relativeScore);
			} else {
				APPLY_TERMINAL_STYLES("\x1b[1;31m");
				printf("%d", relativeScore);
			}

			APPLY_TERMINAL_STYLES("\x1b[0m");
		}
		printf("\n");
	}
}

static void clearConsole(void) {
	if (virtualTerminalEnabled) {
		printf("\x1b[2J\x1b[H");
		(void)fflush(stdout);
		return;
	}

	HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO info;
	if (GetConsoleScreenBufferInfo(output, &info)) {
		DWORD written;
		DWORD cells = (DWORD)info.dwSize.X * (DWORD)info.dwSize.Y;
		COORD origin = {0, 0};
		FillConsoleOutputCharacterW(output, L' ', cells, origin, &written);
		FillConsoleOutputAttribute(output, info.wAttributes, cells, origin, &written);
		SetConsoleCursorPosition(output, origin);
	}
}

static void printTitle(void) {
	if (virtualTerminalEnabled) {
		printf("\x1b[1;4;36mTriple Triad Solver\x1b[0m\n\n");
	} else {
		printf("Triple Triad Solver\n\n");
	}
}
