// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#define FORMATION_NAME_LENGTH 32
#define OPTIONS_MAX_FORMATIONS 32

#define FORMATION_POSITION_COUNT 11

typedef struct {
	char name[FORMATION_NAME_LENGTH];
	PositionCode positions[FORMATION_POSITION_COUNT];
} Formation;

typedef struct {
	bool darkMode;
	Formation formations[OPTIONS_MAX_FORMATIONS];
	uint8_t formationCount;
} Options;
