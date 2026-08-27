// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "search.h"

#include "core/logger.h"
#include "helpers/vector.h"
#include "platform/platform.h"


extern GameContext gameContext;

// TODO: multithread this
uint32_t *search_findPlayers(void) {
	const int64_t timeStart = platform_getMicroseconds();

	const FilterOptions options = gameContext.filterOptions;

	uint32_t *searchResults = NULL;
	for (uint32_t i = 0; i < gameContext.playerCount; ++i) {
		const Player *player = &gameContext.players[i];

		if (
			(options.filterMask & FILTER_HAS_MIN_CA && player->ca < options.minCA) ||
			(options.filterMask & FILTER_HAS_MAX_CA && player->ca > options.maxCA) ||
			(options.filterMask & FILTER_HAS_MAX_PA && player->pa < options.minPA) ||
			(options.filterMask & FILTER_HAS_MAX_PA && player->pa > options.maxPA)
		) {
			continue;
		}

		if (
			(options.filterMask & FILTER_HAS_MIN_AGE && player->age < options.minAge) ||
			(options.filterMask & FILTER_HAS_MAX_AGE && player->age > options.maxAge)
		) {
			continue;
		}

		if (
			(options.filterMask & FILTER_HAS_MIN_VALUE && player->guideValue < options.minValue) ||
			(options.filterMask & FILTER_HAS_MAX_VALUE && player->guideValue > options.maxValue)
		) {
			continue;
		}

		if (options.filterMask & FILTER_HAS_CLUB && player->clubIndex != options.clubIndex) {
			continue;
		}

		if (options.positions > 0) {
			#define MINIMUM_POSITIONAL_PROFICIENCY 15
			bool hasPosition = false;
			if (
				(options.positions & POSITION_GROUPED_GK && player->positions[POSITION_GROUPED_GK] >=
					MINIMUM_POSITIONAL_PROFICIENCY) ||
				(options.positions & POSITION_GROUPED_FB && player->positions[POSITION_GROUPED_FB] >=
					MINIMUM_POSITIONAL_PROFICIENCY) ||
				(options.positions & POSITION_GROUPED_CB && player->positions[POSITION_GROUPED_CB] >=
					MINIMUM_POSITIONAL_PROFICIENCY) ||
				(options.positions & POSITION_GROUPED_WB && player->positions[POSITION_GROUPED_WB] >=
					MINIMUM_POSITIONAL_PROFICIENCY) ||
				(options.positions & POSITION_GROUPED_DM && player->positions[POSITION_GROUPED_DM] >=
					MINIMUM_POSITIONAL_PROFICIENCY) ||
				(options.positions & POSITION_GROUPED_MC && player->positions[POSITION_GROUPED_MC] >=
					MINIMUM_POSITIONAL_PROFICIENCY) ||
				(options.positions & POSITION_GROUPED_W && player->positions[POSITION_GROUPED_W] >=
					MINIMUM_POSITIONAL_PROFICIENCY) ||
				(options.positions & POSITION_GROUPED_AM && player->positions[POSITION_GROUPED_AM] >=
					MINIMUM_POSITIONAL_PROFICIENCY) ||
				(options.positions & POSITION_GROUPED_ST && player->positions[POSITION_GROUPED_ST] >=
					MINIMUM_POSITIONAL_PROFICIENCY)
			) {
				hasPosition = true;
			}

			if (!hasPosition) {
				continue;
			}
		}

		if (
			(options.filterMask & FILTER_HAS_MIN_RATING && player->ratings[0].value < options.minRating) ||
			(options.filterMask & FILTER_HAS_MAX_RATING && player->ratings[0].value > options.maxRating)
		) {
			continue;
		}

		vector_push(searchResults, i);
	}

	const int64_t timeEnd = platform_getMicroseconds();
	LOG_INFO("Found %d players in %llu microseconds", vector_length(searchResults), timeEnd - timeStart);

	return searchResults;
}
