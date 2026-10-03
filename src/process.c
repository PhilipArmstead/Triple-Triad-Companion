// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "process.h"

#include "types.h"

#include <tlhelp32.h>
#include <windows.h>


void process_open(ProcessContext *context) {
	context->handle = NULL;
	context->pid = 0;

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		return;
	}

	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(PROCESSENTRY32);

	DWORD pid = 0;
	BOOL hResult = Process32First(hSnapshot, &pe);
	while (hResult) {
		if (_stricmp(pe.szExeFile, PROCESS_NAME) == 0) {
			pid = pe.th32ProcessID;
			break;
		}
		hResult = Process32Next(hSnapshot, &pe);
	}

	CloseHandle(hSnapshot);

	if (pid == 0) {
		return;
	}

	HANDLE h =
		OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, pid);
	if (h == NULL || h == INVALID_HANDLE_VALUE) {
		return;
	}

	// Get the base address of the module
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
	if (snap == INVALID_HANDLE_VALUE) {
		CloseHandle(h);
		return;
	}

	MODULEENTRY32W me;
	BYTE *baseAddr = 0;
	bool moduleNotFound = true;
	me.dwSize = sizeof(MODULEENTRY32W);
	if (Module32FirstW(snap, &me)) {
		do {
			if (_wcsicmp(me.szModule, PROCESS_NAME_LONG) == 0) {
				baseAddr = me.modBaseAddr;
				moduleNotFound = false;
				break;
			}
		} while (Module32NextW(snap, &me));
	}

	CloseHandle(snap);

	if (moduleNotFound) {
		CloseHandle(h);
		return;
	}

	context->handle = h;
	context->pid = pid;
	context->moduleBaseAddress = baseAddr;
}
