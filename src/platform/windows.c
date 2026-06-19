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
	WriteConsoleA(consoleHandle, message, length, NULL, NULL);
}

void platform_consoleWrite(const char *message, const LogLevel colour) {
	writeToConsole(message, colour, GetStdHandle(STD_OUTPUT_HANDLE));
}

void platform_consoleWriteError(const char *message, const LogLevel colour) {
	writeToConsole(message, colour, GetStdHandle(STD_ERROR_HANDLE));
}


// Process
#include <stdint.h>
#include <stdlib.h>
#include <tlhelp32.h>

#include "core/logger.h"

void platform_openProcess(ProcessContext *context) {
	context->handle = NULL;
	context->pid = 0;

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		LOG_ERROR("Failed to create process snapshot: %s", strerror(errno));
		return;
	}

	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(PROCESSENTRY32);


	DWORD pid = 0;
	{
		BOOL hResult = Process32First(hSnapshot, &pe);
		while (hResult) {
			if (stricmp(pe.szExeFile, "fm.exe") == 0) {
				pid = pe.th32ProcessID;
				break;
			}
			hResult = Process32Next(hSnapshot, &pe);
		}
	}

	CloseHandle(hSnapshot);

	if (pid == 0) {
		LOG_ERROR("Process '%s' not found", "fm.exe");
		return;
	}

	HANDLE h = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, pid);
	if (h == NULL || h == INVALID_HANDLE_VALUE) {
		LOG_ERROR("Failed to open process with PID %u: access denied or process not found", pid);
		return;
	}

	// Get the base address of the module
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
	if (snap == INVALID_HANDLE_VALUE) {
		LOG_ERROR("Failed to create module snapshot: %s", strerror(errno));
		CloseHandle(h);
		return;
	};

	MODULEENTRY32W me;
	uintptr_t baseAddr = 0;
	bool moduleNotFound = true;
	me.dwSize = sizeof(MODULEENTRY32W);
	if (Module32FirstW(snap, &me)) {
		do {
			if (_wcsicmp(me.szModule, L"fm.exe") == 0) {
				baseAddr = (uintptr_t)me.modBaseAddr;
				moduleNotFound = false;
				break;
			}
		} while (Module32NextW(snap, &me));
	}

	CloseHandle(snap);

	if (moduleNotFound) {
		LOG_ERROR("Module '%s' not found in process with PID %u", "fm.exe", pid);
		CloseHandle(h);
		return;
	}

	context->handle = h;
	context->pid = pid;
	context->moduleBaseAddress = baseAddr;
}
#endif
