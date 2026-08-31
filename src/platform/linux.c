// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform.h"


#if defined(ARCH_LINUX)
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
#define _fileOffSET_BITS 64
#include <string.h>
#include <unistd.h>


bool readFromMemory(void *handle, uintptr_t address, size_t length, uint8_t *bytes) {
	if (pread((int)(intptr_t)handle, bytes, length, (off_t)address) != (ssize_t)length) {
		memset(bytes, 0, length);
		platform_consoleWriteError("Failed to read memory\n", LogLevelError);
		return false;
	}

	return true;
}

uint8_t readByte(void *handle, uintptr_t address) {
	uint8_t byte = 0;
	if (pread((int)(intptr_t)handle, &byte, 1, (off_t)address) != 1) {
		platform_consoleWriteError("Failed to read byte\n", LogLevelError);
	}
	return byte;
}

void writeToMemory(void *handle, uintptr_t address, size_t length, const uint8_t *bytes) {
	if (pwrite((int)(intptr_t)handle, bytes, length, (off_t)address) != (ssize_t)length) {
		platform_consoleWriteError("Failed to write to memory\n", LogLevelError);
	}
}


// Process
#include <fcntl.h>
#include <stdlib.h>


typedef struct {
	uintptr_t mapStart;
	uintptr_t fileOff;
} mapping_t;

static uint64_t findMemoryMap(uint64_t pid, uint64_t offset) {
	mapping_t out = {0};
	char mapsPath[64];
	snprintf(mapsPath, sizeof(mapsPath), "/proc/%lu/maps", pid);

	FILE *f = fopen(mapsPath, "r");
	if (!f) {
		platform_consoleWriteError("Could not read memory map\n", LogLevelError);
		return 0;
	}

	char line[8192];
	while (fgets(line, sizeof(line), f)) {
		uintptr_t start = 0, end = 0;
		unsigned long fileoff = 0;
		char perms[8] = {0};
		char path[4096] = {0};

		// maps line format:
		// start-end perms offset dev inode pathname...
		// pathname can contain spaces, so we parse carefully by taking the whole tail.
		const int n = sscanf(line, "%lx-%lx %7s %lx %*s %*s %4095[^\n]", &start, &end, perms, &fileoff, path);
		if (n < 4) {
			continue;
		}

		// Only consider mappings that are for fm.exe.
		// (Use substring match since full path can vary.)
		if (!strstr(path, "fm.exe")) {
			continue;
		}

		const uint64_t mapStart = start;
		const uint64_t mapEnd = end;

		if (offset < fileoff) {
			continue;
		}

		const uint64_t delta = offset - fileoff;
		const uint64_t addr = mapStart + delta;

		if (addr >= mapStart && addr < mapEnd) {
			out.mapStart = mapStart;
			out.fileOff = fileoff;
			break;
		}
	}

	fclose(f);
	return out.mapStart - out.fileOff;
}

void platform_openProcess(ProcessContext *context) {
	context->handle = NULL;
	context->pid = 0;

	FILE *fp = popen("pidof -s 'Main Thread' 2>/dev/null", "r");
	if (!fp) {
		platform_consoleWriteError("Process not found (1)\n", LogLevelError);
		return;
	}

	char path[64];
	if (!fgets(path, sizeof(path), fp)) {
		platform_consoleWriteError("Process not found (2)\n", LogLevelError);
		return;
	}
	const uint32_t pid = (uint32_t)strtol(path, NULL, 10);
	pclose(fp);

	if (pid == 0) {
		platform_consoleWriteError("Process not found (3)\n", LogLevelError);
		return;
	}

	char memoryPath[32];
	sprintf(memoryPath, "/proc/%d/mem", pid);
	const int handle = open(memoryPath, O_RDWR);

	if (handle == -1) {
		platform_consoleWriteError("Could not open process memory\n", LogLevelError);
	}

	const uint64_t baseAddr = findMemoryMap(pid, CURRENT_DATETIME_PTR_BASE);

	context->handle = (void*)(intptr_t)handle;
	context->pid = pid;
	context->moduleBaseAddress = baseAddr;
}

// Time
int64_t platform_getMicroseconds(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (int64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}
#endif
