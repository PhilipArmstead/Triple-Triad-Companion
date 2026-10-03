// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "memory.h"


bool readFromMemory(HANDLE handle, LPCVOID address, size_t length, uint8_t *bytes) {
	if (!ReadProcessMemory(handle, address, bytes, length, NULL)) {
		memset(bytes, 0, length);
		return false;
	}

	return true;
}

uint8_t readByte(HANDLE handle, LPCVOID address) {
	uint8_t byte = 0;
	ReadProcessMemory(handle, address, &byte, 1, NULL);
	return byte;
}
