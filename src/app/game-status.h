// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/types.h"


DayMonthYear getDayMonthYear(const ProcessContext *context);
Date getDate(const ProcessContext *context);
void getGameVersion(const ProcessContext *context, char *versionBuffer, uint8_t bufferSize);
