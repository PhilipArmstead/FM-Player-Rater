// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "search-handler.h"
#include "app/player-table.h"
#include "app/search.h"

#include <gtk/gtk.h>


extern GameContext gameContext;

static inline bool valueFitsOneByte(int64_t value) {
	return value >= 0 && value <= 0xFF;
}

void searchHandler_cacheFilters(void) {
	FilterOptions *options = &gameContext.filterOptions;
	options->filterMask = 0;

	const gchar *minAge = gtk_entry_buffer_get_text(gameContext.filterBuffer.minAge);
	const gchar *maxAge = gtk_entry_buffer_get_text(gameContext.filterBuffer.maxAge);
	const gchar *minCA = gtk_entry_buffer_get_text(gameContext.filterBuffer.minCA);
	const gchar *maxCA = gtk_entry_buffer_get_text(gameContext.filterBuffer.maxCA);
	const gchar *minPA = gtk_entry_buffer_get_text(gameContext.filterBuffer.minPA);
	const gchar *maxPA = gtk_entry_buffer_get_text(gameContext.filterBuffer.maxPA);
	const gchar *minRating = gtk_entry_buffer_get_text(gameContext.filterBuffer.minRating);
	const gchar *maxRating = gtk_entry_buffer_get_text(gameContext.filterBuffer.maxRating);

	int64_t value;

	if (minAge != NULL && *minAge != '\0') {
		value = g_ascii_strtoll(minAge, NULL, 10);
		if (valueFitsOneByte(value)) {
			options->minAge = (uint8_t)value;
			options->filterMask |= FILTER_HAS_MIN_AGE;
		}
	}
	if (maxAge != NULL && *maxAge != '\0') {
		value = g_ascii_strtoll(maxAge, NULL, 10);
		if (valueFitsOneByte(value)) {
			options->maxAge = (uint8_t)value;
			options->filterMask |= FILTER_HAS_MAX_AGE;
		}
	}
	if (minCA != NULL && *minCA != '\0') {
		value = g_ascii_strtoll(minCA, NULL, 10);
		if (valueFitsOneByte(value)) {
			options->minCA = (uint8_t)value;
			options->filterMask |= FILTER_HAS_MIN_CA;
		}
	}
	if (maxCA != NULL && *maxCA != '\0') {
		value = g_ascii_strtoll(maxCA, NULL, 10);
		if (valueFitsOneByte(value)) {
			options->maxCA = (uint8_t)value;
			options->filterMask |= FILTER_HAS_MAX_CA;
		}
	}
	if (minPA != NULL && *minPA != '\0') {
		value = g_ascii_strtoll(minPA, NULL, 10);
		if (valueFitsOneByte(value)) {
			options->minPA = (uint8_t)value;
			options->filterMask |= FILTER_HAS_MIN_PA;
		}
	}
	if (maxPA != NULL && *maxPA != '\0') {
		value = g_ascii_strtoll(maxPA, NULL, 10);
		if (valueFitsOneByte(value)) {
			options->maxPA = (uint8_t)value;
			options->filterMask |= FILTER_HAS_MAX_PA;
		}
	}
	if (minRating != NULL && *minRating != '\0') {
		value = g_ascii_strtoll(minRating, NULL, 10);
		if (valueFitsOneByte(value)) {
			options->minRating = (uint8_t)value;
			options->filterMask |= FILTER_HAS_MIN_RATING;
		}
	}
	if (maxRating != NULL && *maxRating != '\0') {
		value = g_ascii_strtoll(maxRating, NULL, 10);
		if (valueFitsOneByte(value)) {
			options->maxRating = (uint8_t)value;
			options->filterMask |= FILTER_HAS_MAX_RATING;
		}
	}
}

void searchHandler_doSearch(bool refreshFilterCache) {
	if (refreshFilterCache) {
		searchHandler_cacheFilters();
	}
	playerTable_populate(
		gameContext.table,
		search_findPlayers()
	);
}

void searchHandler_clearFilters(void) {
	gtk_entry_buffer_set_text(gameContext.filterBuffer.minAge, "", 1);
	gtk_entry_buffer_set_text(gameContext.filterBuffer.maxAge, "", 1);
	gtk_entry_buffer_set_text(gameContext.filterBuffer.minCA, "", 1);
	gtk_entry_buffer_set_text(gameContext.filterBuffer.maxCA, "", 1);
	gtk_entry_buffer_set_text(gameContext.filterBuffer.minPA, "", 1);
	gtk_entry_buffer_set_text(gameContext.filterBuffer.maxPA, "", 1);
	gtk_entry_buffer_set_text(gameContext.filterBuffer.minRating, "", 1);
	gtk_entry_buffer_set_text(gameContext.filterBuffer.maxRating, "", 1);
	gtk_entry_buffer_set_text(gameContext.filterBuffer.clubSearch, "", 1);
}
