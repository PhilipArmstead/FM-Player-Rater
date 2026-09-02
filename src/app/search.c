// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "search.h"
#include "app/mocks.h"
#include "core/logger.h"
#include "helpers/vector-shared-pointer.h"
#include "helpers/vector.h"
#include "platform/platform.h"


extern GameContext gameContext;

// TODO: multithread this
uint32_t *search_findPlayers(void) {
	const int64_t timeStart = platform_getMicroseconds();

	uint32_t *playerIds = NULL;

	#ifdef PLAYER_BY_ID
	for (uint32_t i = 0; i < (uint32_t)gameContext.playerCount; i++) {
		vector_push(playerIds, i);
	}
	#else
	const FilterOptions options = gameContext.filterOptions;

	for (uint32_t i = 0; i < gameContext.playerCount; ++i) {
		const Player *player = &gameContext.players[i];

		if (
			(options.filterMask & FILTER_HAS_MIN_CA && player->ca < options.minCA) ||
			(options.filterMask & FILTER_HAS_MAX_CA && player->ca > options.maxCA) ||
			(options.filterMask & FILTER_HAS_MIN_PA && player->pa < options.minPA) ||
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
			bool hasPosition = false;
			if (
				(options.positions & POSITION_MASK_GK && player->positions[POSITION_GROUPED_GK] >=
					MINIMUM_POSITIONAL_PROFICIENCY) ||
				(options.positions & POSITION_MASK_FB && player->positions[POSITION_GROUPED_FB] >=
					MINIMUM_POSITIONAL_PROFICIENCY) ||
				(options.positions & POSITION_MASK_CB && player->positions[POSITION_GROUPED_CB] >=
					MINIMUM_POSITIONAL_PROFICIENCY) ||
				(options.positions & POSITION_MASK_WB && player->positions[POSITION_GROUPED_WB] >=
					MINIMUM_POSITIONAL_PROFICIENCY) ||
				(options.positions & POSITION_MASK_DM && player->positions[POSITION_GROUPED_DM] >=
					MINIMUM_POSITIONAL_PROFICIENCY) ||
				(options.positions & POSITION_MASK_MC && player->positions[POSITION_GROUPED_MC] >=
					MINIMUM_POSITIONAL_PROFICIENCY) ||
				(options.positions & POSITION_MASK_W && player->positions[POSITION_GROUPED_W] >=
					MINIMUM_POSITIONAL_PROFICIENCY) ||
				(options.positions & POSITION_MASK_AM && player->positions[POSITION_GROUPED_AM] >=
					MINIMUM_POSITIONAL_PROFICIENCY) ||
				(options.positions & POSITION_MASK_ST && player->positions[POSITION_GROUPED_ST] >=
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

		vector_push(playerIds, i);
	}
	#endif

	sharedPointer_unref(gameContext.searchResults);
	gameContext.searchResults = sharedPointer_new(playerIds);

	const int64_t timeEnd = platform_getMicroseconds();
	LOG_DEBUG("Found %d players in %zu microseconds", vector_length(playerIds), timeEnd - timeStart);
}
