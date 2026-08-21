// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/config.h"


#ifdef MOCKS_MODE
#define PLAYER_BY_ID ((Player){ \
	.attributes = {40, 65, 90, 85, 60, 35, 90, 65, 95, 35, 70, 10, 20, 10, 15, 15, 5, 85, 85, 10, 35, 5, 90, 80, 65, 100, 70, 15, 70, 75, 15, 15, 10, 10, 65, 75, 80, 80, 70, 75, 75, 40, 90, 75, 75, 70, 65, 75, 10, 40, 90, 90, 85, 85 }, \
	.ratings = { { .value = 75.4058762, .position = POSITION_GROUPED_ST } }, \
	.club = { .name = "Barcelona", .address = 0x5b0eefe0, .teamType = 0 }, \
	.commonName = "Robert Lewandowski", \
	.forename = "Robert", \
	.surname = "Lewandowski", \
	.nationality = { {.name = "Poland", .code = "POL"} }, \
	.rowId = 1152, \
	.rid = 719601, \
	.uid = 719601, \
	.personAddress = 0x9a7f2578, \
	.playerAddress = 0x9a7f2300, \
	.guideValue = 300000000, \
	.annualWage = 0, \
	.sharpness = 5950, \
	.fatigue = -125, \
	.condition = 10000, \
	.homeReputation = 9100, \
	.currentReputation = 8900, \
	.worldReputation = 8600, \
	.positions = {1, 1, 1, 1, 1, 1, 1, 1, 1, 10, 12, 8, 20, 1, 1 }, \
	.age = 34, \
	.ca = 178, \
	.pa = 190, \
	.personality = {15, 18, 10, 16, 18, 10, 11, 12 }, \
	.canDevelopQuickly = false, \
	.isHotProspect = false, \
	})
// todo fix club
#endif
