// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform.h"


#if defined(ARCH_MACOS)
// Logging
#include <stdio.h>


void platform_consoleWrite(const char *message, const LogLevel colour) {
	// Fatal, error, warn, info
	const char *colourStrings[4] = {"30;41", "1;31", "1;33", "1;32"};
	printf("\033[%sm%s\033[0m", colourStrings[colour], message);
}

void platform_consoleWriteError(const char *message, const LogLevel colour) {
	// Fatal, error, warn, info
	const char *colourStrings[4] = {"30;41", "1;31", "1;33", "1;32"};
	printf("\033[%sm%s\033[0m", colourStrings[colour], message);
}

// Memory
bool readFromMemory(void *handle, uintptr_t address, size_t length, uint8_t *bytes) {
	(void)handle;
	(void)address;
	(void)length;
	(void)bytes;
	return true;
}

uint8_t readByte(void *handle, uintptr_t address) {
	(void)handle;
	(void)address;
	return 0;
}

void writeToMemory(void *handle, uintptr_t address, size_t length, const uint8_t *bytes) {
	(void)handle;
	(void)address;
	(void)length;
	(void)bytes;
}


// Process
void platform_openProcess(ProcessContext *context) {
	context->handle = NULL;
	context->pid = 0;
}

// Time
int64_t platform_getMicroseconds(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (int64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}
#endif
