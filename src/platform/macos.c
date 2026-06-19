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
#endif
