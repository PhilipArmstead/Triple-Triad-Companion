// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "app.h"

#include "constants.h"
#include "states.h"
#include "types.h"
#include "platform/memory.h"
#include "platform/process.h"

#include <stdbool.h>
#include <stdint.h>
#include <windows.h>


static LARGE_INTEGER timerFrequency;
static GameStatus gameStatus = GAME_STATUS_UNKNOWN;
static ProcessContext processContext;

static double getHighPrecisionSeconds(void);
static void update(double dt);

int app_run(void) {
	state_init();

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
		state_handleDisconnected(&gameStatus);
		return;
	}

	// Connected. What phase are we in?
	const bool isInCardGame =
		readByte(processContext.handle, processContext.moduleBaseAddress + MO_IS_IN_CARD_GAME) == 1;

	if (!isInCardGame) {
		state_handleNotInCards(&gameStatus);
		return;
	}

	const uint8_t cardGameState =
		readByte(processContext.handle, processContext.moduleBaseAddress + MO_CARD_GAME_STATE);


	if (cardGameState == 4) {
		state_handleSelectionPhase(&gameStatus);
		return;
	} else if (cardGameState > 4) {
		state_handleInGame(&gameStatus, processContext);
	}
}

static double getHighPrecisionSeconds(void) {
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);

	return (double)counter.QuadPart / (double)timerFrequency.QuadPart;
}
