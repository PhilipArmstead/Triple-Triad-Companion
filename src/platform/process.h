// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../types.h"


void process_open(ProcessContext *context);
bool process_isRunning(const ProcessContext *context);
void process_close(ProcessContext *context);
