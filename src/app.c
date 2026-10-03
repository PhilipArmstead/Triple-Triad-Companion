// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "app.h"

#include "memory.h"
#include "process.h"
#include "types.h"

#include <stdbool.h>
#include <stdio.h>
#include <windows.h>


typedef enum {
	GAME_STATE_UNKNOWN,
	GAME_STATE_DISCONNECTED,
	GAME_STATE_NOT_IN_CARDS,
	GAME_STATE_PRE_PHASE,
	GAME_STATE_SELECTION_PHASE,
	GAME_STATE_IN_GAME_WAITING,
	GAME_STATE_IN_GAME_ACTING,
	GAME_STATE_POST_PHASE,
} GameState;

static LARGE_INTEGER timerFrequency;
static GameState gameState = GAME_STATE_UNKNOWN;
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

	// If our state is unknown, try to connect
	if (gameState <= GAME_STATE_DISCONNECTED) {
		process_open(&processContext);
	}

	if (processContext.handle == NULL) {
		if (gameState != GAME_STATE_DISCONNECTED) {
			gameState = GAME_STATE_DISCONNECTED;
			printf("Disconnected\n");
		}
		return;
	} else if (gameState <= GAME_STATE_DISCONNECTED) {
		printf("Connected\n");
	}

	// Connected. What phase are we in?
	const bool isInCardGame =
		readByte(processContext.handle, processContext.moduleBaseAddress + 0x19CD798) == 1;

	if (!isInCardGame) {
		if (gameState != GAME_STATE_NOT_IN_CARDS) {
			gameState = GAME_STATE_NOT_IN_CARDS;
			printf("Not in a card game\n");
		}
		return;
	}

	const uint8_t cardGameState =
		readByte(processContext.handle, processContext.moduleBaseAddress + 0x19CD7A0);


	if (cardGameState == 4) {
		if (gameState != GAME_STATE_SELECTION_PHASE) {
			gameState = GAME_STATE_SELECTION_PHASE;
			printf("Choosing cards\n");
		}
		return;
	} else if (cardGameState > 4) {
		bool isMyTurn =
			readByte(processContext.handle, processContext.moduleBaseAddress + 0x19FF420) == 1;
		if (isMyTurn) {
			if (gameState != GAME_STATE_IN_GAME_ACTING) {
				gameState = GAME_STATE_IN_GAME_ACTING;
				printf("Your turn to act\n");
			}
		} else if (gameState != GAME_STATE_IN_GAME_WAITING) {
			gameState = GAME_STATE_IN_GAME_WAITING;
			printf("Opponent acting\n");
		}
	}
}

static double getHighPrecisionSeconds(void) {
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);

	return (double)counter.QuadPart / (double)timerFrequency.QuadPart;
}
