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
	float weights[ATTRIBUTE_COUNT];
	PositionGrouped role; // POSITION_GROUPED_COUNT means all roles unless otherwise explicitly defined
	float scale;
} PositionWeights;

typedef struct {
	Formation formations[OPTIONS_MAX_FORMATIONS];
	PositionWeights weights[POSITION_GROUPED_COUNT];
	uint8_t formationCount;
	bool darkMode;
} Options;
