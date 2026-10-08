// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "app.h"

#include "constants.h"
#include "platform/memory.h"
#include "platform/process.h"
#include "triple-triad/game.h"
#include "triple-triad/solver.h"
#include "types.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <windows.h>


static LARGE_INTEGER timerFrequency;
static GameStatus gameStatus = GAME_STATUS_UNKNOWN;
static ProcessContext processContext;

static double getHighPrecisionSeconds(void);
static void update(double dt);

int app_run(void) {
	QueryPerformanceFrequency(&timerFrequency);

	const double targetFrameTime = 1.0 / 60.0;
	double previousTime = getHighPrecisionSeconds();

	bool running = true;

	while (running) {
		double frameStart = getHighPrecisionSeconds();
		double dt = frameStart - previousTime;
		previousTime = frameStart;

		update(dt);

		double elapsed = getHighPrecisionSeconds() - frameStart;
		double remaining = targetFrameTime - elapsed;

		if (remaining > 0.0) {
			// Sleep slightly less than the full remaining time because
			// Sleep may wake up later than requested.
			if (remaining > 0.002) {
				Sleep((DWORD)((remaining - 0.001) * 1000.0));
			}

			// Optional short spin for improved precision.
			while (getHighPrecisionSeconds() - frameStart < targetFrameTime) {
				YieldProcessor();
			}
		}
	}

	return 0;
}

static void update(double dt) {
	(void)dt;

	if (processContext.handle != NULL && !process_isRunning(&processContext)) {
		process_close(&processContext);
		gameStatus = GAME_STATUS_DISCONNECTED;
	}

	// If our state is unknown, try to connect
	if (gameStatus <= GAME_STATUS_DISCONNECTED) {
		process_open(&processContext);
	}

	if (processContext.handle == NULL) {
		if (gameStatus != GAME_STATUS_DISCONNECTED) {
			gameStatus = GAME_STATUS_DISCONNECTED;
			printf("Disconnected\n");
		}
		return;
	} else if (gameStatus <= GAME_STATUS_DISCONNECTED) {
		printf("Connected\n");
	}

	// Connected. What phase are we in?
	const bool isInCardGame =
		readByte(processContext.handle, processContext.moduleBaseAddress + MO_IS_IN_CARD_GAME) == 1;

	if (!isInCardGame) {
		if (gameStatus != GAME_STATUS_NOT_IN_CARDS) {
			gameStatus = GAME_STATUS_NOT_IN_CARDS;
			printf("Not in a card game\n");
		}
		return;
	}

	const uint8_t cardGameState =
		readByte(processContext.handle, processContext.moduleBaseAddress + MO_CARD_GAME_STATE);


	if (cardGameState == 4) {
		if (gameStatus != GAME_STATUS_SELECTION_PHASE) {
			gameStatus = GAME_STATUS_SELECTION_PHASE;
			printf("Choosing cards\n");
		}
		return;
	} else if (cardGameState > 4) {
		bool isMyTurn =
			readByte(processContext.handle, processContext.moduleBaseAddress + MO_IS_MY_TURN) == 1;
		if (isMyTurn) {
			if (gameStatus != GAME_STATUS_IN_GAME_ACTING) {
				gameStatus = GAME_STATUS_IN_GAME_ACTING;

				const Game game = game_init(processContext);
				const CachedEntry result = solver_getOptimalMove(game.state);
				const uint8_t score = result & 0x0Fu;
				const uint8_t move = (uint8_t)(result >> 6);
				const uint8_t position = move & 0xF;
				const uint8_t cardId = move >> 4;

				printf(
					"Your turn to act; move '%s' to %s for a score of %d\n",
					game.cards.names[game.state.cards[cardId].id],
					positionStrings[position],
					(int8_t)score - CARDS_IN_HAND
				);
			}
		} else if (gameStatus != GAME_STATUS_IN_GAME_WAITING) {
			gameStatus = GAME_STATUS_IN_GAME_WAITING;
			printf("Opponent acting\n");
		}
	}
}

static double getHighPrecisionSeconds(void) {
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);

	return (double)counter.QuadPart / (double)timerFrequency.QuadPart;
}
