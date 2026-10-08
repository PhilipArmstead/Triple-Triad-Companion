// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../constants.h"
#include "../types.h"


CachedCards cards_cache(ProcessContext context);
bool cards_aDefeatsB(CardAttributes attributesA, Cell positionA, CardAttributes attributesB, Cell positionB);
