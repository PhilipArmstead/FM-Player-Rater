// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform.h"


#if defined(ARCH_WIN)
// Logging
#include <stdio.h>
#include <string.h>
#include <windows.h>


static void writeToConsole(const char *message, const LogLevel colour, HANDLE consoleHandle) {
	if (consoleHandle == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "%s", message);
		printf("\n");
		return;
	}

	// Fatal, error, warn, info
	static uint8_t levels[4] = {64, 4, 6, 2};
	SetConsoleTextAttribute(consoleHandle, levels[colour]);

	OutputDebugStringA(message);

	const DWORD length = (DWORD)strlen(message);
	WriteConsoleA(consoleHandle, message, length, nullptr, nullptr);
}

void platform_consoleWrite(const char *message, const LogLevel colour) {
	writeToConsole(message, colour, GetStdHandle(STD_OUTPUT_HANDLE));
}

void platform_consoleWriteError(const char *message, const LogLevel colour) {
	writeToConsole(message, colour, GetStdHandle(STD_ERROR_HANDLE));
}
#endif
