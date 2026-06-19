// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "types.h"

#include <stdlib.h>


bool logger_init(void);
void logger_shutdown(void);
void logger_output(LogLevel level, const char *message, ...);

#define LOG_FATAL(message, ...) logger_output(LogLevelFatal, message, ##__VA_ARGS__)
#define LOG_ERROR(message, ...) logger_output(LogLevelError, message, ##__VA_ARGS__)
#define LOG_WARN(message, ...) logger_output(LogLevelWarn, message, ##__VA_ARGS__)
#define LOG_INFO(message, ...) logger_output(LogLevelInfo, message, ##__VA_ARGS__)
#define ERROR_EXIT(message, ...) { LOG_FATAL(message, ##__VA_ARGS__); exit(1); }
#define ERROR_RETURN(R, message, ...) { LOG_FATAL(message, ##__VA_ARGS__); return (R); }
