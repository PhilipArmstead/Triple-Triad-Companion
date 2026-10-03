// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <windows.h>


typedef struct {
	HANDLE handle;
	BYTE *moduleBaseAddress;
	DWORD pid;
} ProcessContext;

#define PROCESS_NAME "FF8_EN.exe"
#define PROCESS_NAME_LONG L"FF8_EN.exe"
