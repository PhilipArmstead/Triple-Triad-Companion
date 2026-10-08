// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "types.h"


void state_init(void);
void state_showTitle(void);
void state_handleDisconnected(GameStatus *status);
void state_handleNotInCards(GameStatus *status);
void state_handleSelectionPhase(GameStatus *status);
void state_handleInGame(GameStatus *status, ProcessContext context);
