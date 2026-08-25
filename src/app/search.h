// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/types.h"


#define POSITION_MASK_GK (1 << 0),
#define POSITION_MASK_FB (1 << 1),
#define POSITION_MASK_CB (1 << 2),
#define POSITION_MASK_WB (1 << 3),
#define POSITION_MASK_DM (1 << 4),
#define POSITION_MASK_MC (1 << 5),
#define POSITION_MASK_W (1 << 6),
#define POSITION_MASK_AM (1 << 7),
#define POSITION_MASK_ST (1 << 8),

typedef struct {
	uint16_t positions; // Masked, one bit per position
	uint32_t minValue;
	uint32_t maxValue;
	float minRating;
	float maxRating;
	uint16_t minHomeReputation;
	uint16_t maxHomeReputation;
	uint16_t minCurrentReputation;
	uint16_t maxCurrentReputation;
	uint16_t minWorldReputation;
	uint16_t maxWorldReputation;
	uint8_t minAge;
	uint8_t maxAge;
	uint8_t minCA;
	uint8_t maxCA;
	uint8_t minPA;
	uint8_t maxPA;
} SearchOptions;

SearchOptions search_createContext(void);
uint32_t *search_findPlayers(SearchOptions options);
