// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#define WIN32_LEAN_AND_MEAN

#include "app.h"

#include <windows.h>

#define IDI_APP 101


int main(int argc, char **argv) {
	(void)argc;
	(void)argv;

	HWND consoleWindow = GetConsoleWindow();
	if (consoleWindow != NULL) {
		HICON appIcon = (HICON)LoadImageW(
			GetModuleHandleW(NULL),
			MAKEINTRESOURCEW(IDI_APP),
			IMAGE_ICON,
			0,
			0,
			LR_DEFAULTSIZE | LR_SHARED
		);
		if (appIcon != NULL) {
			SendMessageW(consoleWindow, WM_SETICON, ICON_SMALL, (LPARAM)appIcon);
			SendMessageW(consoleWindow, WM_SETICON, ICON_BIG, (LPARAM)appIcon);
		}
	}

	return app_run();
}
