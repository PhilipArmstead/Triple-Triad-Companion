// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../types.h"


bool readFromMemory(HANDLE handle, LPCVOID address, size_t length, uint8_t *bytes);
uint8_t readByte(HANDLE handle, LPCVOID address);
