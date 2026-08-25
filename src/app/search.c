// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "search.h"

#include "core/logger.h"
#include "helpers/vector.h"
#include "platform/platform.h"


extern GameContext gameContext;

SearchOptions search_createContext(void) {
	const SearchOptions options = {
		.minAge = 0xFF,
		.maxAge = 0xFF,
		.minCA = 0xFF,
		.maxCA = 0xFF,
		.minPA = 0xFF,
		.maxPA = 0xFF,
		.minHomeReputation = 0xFFFF,
		.maxHomeReputation = 0xFFFF,
		.minCurrentReputation = 0xFFFF,
		.maxCurrentReputation = 0xFFFF,
		.minWorldReputation = 0xFFFF,
		.maxWorldReputation = 0xFFFF,
		.minRating = -1,
		.maxRating = -1,
		.minValue = 0xFFFFFFFF,
		.maxValue = 0xFFFFFFFF,
		.positions = 0,
	};
	return options;
}

// TODO: multithread this
uint32_t *search_findPlayers(SearchOptions options) {
	const int64_t timeStart = platform_getMicroseconds();

	const uint8_t minAge = options.minAge;
	const uint8_t maxAge = options.maxAge;
	const uint8_t minCA = options.minCA;
	const uint8_t maxCA = options.maxCA;
	const uint8_t minPA = options.minPA;
	const uint8_t maxPA = options.maxPA;
	const uint16_t minHomeReputation = options.minHomeReputation;
	const uint16_t maxHomeReputation = options.maxHomeReputation;
	const uint16_t minCurrentReputation = options.minCurrentReputation;
	const uint16_t maxCurrentReputation = options.maxCurrentReputation;
	const uint16_t minWorldReputation = options.minWorldReputation;
	const uint16_t maxWorldReputation = options.maxWorldReputation;
	const float minRating = options.minRating;
	const float maxRating = options.maxRating;
	const uint32_t minValue = options.minValue;
	const uint32_t maxValue = options.maxValue;
	const uint16_t positions = options.positions;

	uint8_t positionsCount = 0;
	{
		uint8_t i = 0;
		while (i < POSITION_GROUPED_COUNT) {
			if ((positions >> i) & 1) {
				++positionsCount;
			}
			++i;
		}
	}

	uint32_t *searchResults = NULL;
	for (uint32_t i = 0; i < gameContext.playerCount; ++i) {
		const Player *player = &gameContext.players[i];
		if (minPA != 0xFF || maxPA != 0xFF || minCA != 0xFF || maxCA != 0xFF) {
			if (
				(minCA != 0xFF && player->ca < minCA) ||
				(maxCA != 0xFF && player->ca > maxCA) ||
				(minPA != 0xFF && player->pa < minPA) ||
				(maxPA != 0xFF && player->pa > maxPA)
			) {
				continue;
			}
		}
		if (minAge != 0xFF || maxAge != 0xFF) {
			if (
				(minAge != 0xFF && player->age < minAge) ||
				(maxAge != 0xFF && player->age > maxAge)
			) {
				continue;
			}
		}
		if (minValue != 0xFFFFFFFF || maxValue != 0xFFFFFFFF) {
			if (
				(minValue != 0xFFFFFFFF && player->guideValue < minValue) ||
				(maxValue != 0xFFFFFFFF && player->guideValue > maxValue)
			) {
				continue;
			}
		}
		if (positionsCount) {
			#define MINIMUM_POSITIONAL_PROFICIENCY 15
			bool hasPosition = false;
			if (
				(positions & POSITION_GROUPED_GK && player->positions[POSITION_GROUPED_GK] >= MINIMUM_POSITIONAL_PROFICIENCY) ||
				(positions & POSITION_GROUPED_FB && player->positions[POSITION_GROUPED_FB] >= MINIMUM_POSITIONAL_PROFICIENCY) ||
				(positions & POSITION_GROUPED_CB && player->positions[POSITION_GROUPED_CB] >= MINIMUM_POSITIONAL_PROFICIENCY) ||
				(positions & POSITION_GROUPED_WB && player->positions[POSITION_GROUPED_WB] >= MINIMUM_POSITIONAL_PROFICIENCY) ||
				(positions & POSITION_GROUPED_DM && player->positions[POSITION_GROUPED_DM] >= MINIMUM_POSITIONAL_PROFICIENCY) ||
				(positions & POSITION_GROUPED_MC && player->positions[POSITION_GROUPED_MC] >= MINIMUM_POSITIONAL_PROFICIENCY) ||
				(positions & POSITION_GROUPED_W && player->positions[POSITION_GROUPED_W] >= MINIMUM_POSITIONAL_PROFICIENCY) ||
				(positions & POSITION_GROUPED_AM && player->positions[POSITION_GROUPED_AM] >= MINIMUM_POSITIONAL_PROFICIENCY) ||
				(positions & POSITION_GROUPED_ST && player->positions[POSITION_GROUPED_ST] >= MINIMUM_POSITIONAL_PROFICIENCY)
			) {
				hasPosition = true;
			}

			if (!hasPosition) {
				continue;
			}
		}

		if (
			minHomeReputation != 0xFFFF ||
			maxHomeReputation != 0xFFFF ||
			minCurrentReputation != 0xFFFF ||
			maxCurrentReputation != 0xFFFF ||
			minWorldReputation != 0xFFFF ||
			maxWorldReputation != 0xFFFF
		) {
			if (
				(minHomeReputation != 0xFFFF && player->homeReputation < minHomeReputation) ||
				(maxHomeReputation != 0xFFFF && player->homeReputation > maxHomeReputation) ||
				(minCurrentReputation != 0xFFFF && player->currentReputation < minCurrentReputation) ||
				(maxCurrentReputation != 0xFFFF && player->currentReputation > maxCurrentReputation) ||
				(minWorldReputation != 0xFFFF && player->worldReputation < minWorldReputation) ||
				(maxWorldReputation != 0xFFFF && player->worldReputation > maxWorldReputation)
			) {
				continue;
			}
		}

		if (
			(minRating != -1 && player->ratings[0].value < minRating) ||
			(maxRating != -1 && player->ratings[0].value > maxRating)
		) {
			continue;
		}

		vector_push(searchResults, i);
	}

	const int64_t timeEnd = platform_getMicroseconds();
	LOG_INFO("Found %d players in %llu microseconds", vector_length(searchResults), timeEnd - timeStart);

	return searchResults;
}
