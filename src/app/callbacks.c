// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "callbacks.h"

#include "app/data.h"
#include "app/mocks.h"
#include "app/player-table.h"
#include "app/player.h"
#include "app/search-handler.h"
#include "app/ui.h"
#include "core/logger.h"
#include "platform/platform.h"


extern ProcessContext processContext;
extern GameContext gameContext;

static void showPlayerById(uint32_t uniqueId);

void callbacks_init() {
	// Capture keypresses within sidebar to trigger search
	GtkEventControllerKey *controllerKey = GTK_EVENT_CONTROLLER_KEY(gtk_event_controller_key_new());
	g_signal_connect(controllerKey, "key-released", G_CALLBACK(callbackFilterKeypress), NULL);
	gtk_event_controller_set_propagation_phase(GTK_EVENT_CONTROLLER(controllerKey), GTK_PHASE_BUBBLE);

	GtkWidget *sidebar = GTK_WIDGET(gtk_builder_get_object(gameContext.builder, "sidebar"));
	gtk_widget_add_controller(sidebar, GTK_EVENT_CONTROLLER(controllerKey));
}

// TODO: this is slow as fuck
// TODO: sort by reputation?
#define CLUB_SEARCH_LIMIT 30
G_MODULE_EXPORT void callbackOnClubNameChange(GtkEditable *editable, const SearchDatalist *dataList) {
	const int64_t timeStart = platform_getMicroseconds();

	// Clear the list
	{
		GtkWidget *child;
		while ((child = gtk_widget_get_first_child(GTK_WIDGET(dataList->listBox))) != NULL) {
			gtk_list_box_remove(dataList->listBox, GTK_WIDGET(child));
		}
	}

	gameContext.filterOptions.filterMask &= ~FILTER_HAS_CLUB;

	const char *searchValue = gtk_editable_get_text(editable);
	if (strlen(searchValue) < 4) {
		gtk_popover_popdown(dataList->popover);
		return;
	}

	// Add matching options
	uint64_t count = 0;
	for (uint64_t i = 0; i < gameContext.clubCount && count < CLUB_SEARCH_LIMIT; i++) {
		if (
			g_str_match_string(searchValue, gameContext.clubs[i].name, TRUE)
			|| g_str_match_string(searchValue, gameContext.clubs[i].shortName,TRUE)
		) {
			GtkWidget *label = gtk_label_new(gameContext.clubs[i].shortName);

			// Danger, pointer abuse!
			g_object_set_data(G_OBJECT(label), "index", GINT_TO_POINTER(i));

			gtk_widget_set_halign(label, GTK_ALIGN_START);
			gtk_list_box_append(dataList->listBox, label);
			++count;
		}
	}

	// Show/hide popover based on matches
	if (gtk_widget_get_first_child(GTK_WIDGET(dataList->listBox)) != NULL) {
		gtk_popover_popup(dataList->popover);
	} else {
		gtk_popover_popdown(dataList->popover);
	}

	const int64_t timeEnd = platform_getMicroseconds();
	LOG_INFO("Searched %llu clubs in %llu microseconds", count, timeEnd - timeStart);
}

G_MODULE_EXPORT void callbackOnClubNameSelected(
	GtkListBox *box,
	GtkListBoxRow *row,
	const SearchDatalist *dataList
) {
	(void)box;

	if (row == NULL) {
		return;
	}

	GtkWidget *child = gtk_list_box_row_get_child(row);
	const char *text = gtk_label_get_text(GTK_LABEL(child));
	gtk_editable_set_text(GTK_EDITABLE(dataList->entry), text);
	gtk_popover_popdown(dataList->popover);

	gameContext.filterOptions.filterMask |= FILTER_HAS_CLUB;
	gameContext.filterOptions.clubIndex = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(child), "index"));
	char buffer[48] = {0};
	snprintf(buffer, 32, "Club: %s", gameContext.clubs[gameContext.filterOptions.clubIndex].shortName);
	searchHandler_doSearch(true);
	ui_clearFilterTags();
}

#define GET_ENTRY_TEXT(buffer) \
	gtk_entry_buffer_get_text(buffer)

G_MODULE_EXPORT gboolean callbackFilterKeypress(
	GtkEventControllerKey *controller,
	guint keyval,
	guint keycode,
	GdkModifierType state,
	gpointer data
) {
	(void)controller;
	(void)keycode;
	(void)state;
	(void)data;

	if (keyval == GDK_KEY_Return) {
		searchHandler_doSearch(true);
		ui_clearFilterTags();

		// TODO: move this logic somewhere the club selector can use it
		//  also create club tag
		const FilterOptions options = gameContext.filterOptions;
		const FilterBuffer fb = gameContext.filterBuffer;

		char buffer[32] = {0};
		if (options.filterMask & FILTER_HAS_MIN_AGE) {
			snprintf(buffer, 32, "Age ≥ %d", options.minAge);
			ui_createFilterTag(buffer, fb.minAge);
		} else {
			gtk_entry_buffer_set_text(fb.minAge, "", 1);
		}
		if (options.filterMask & FILTER_HAS_MAX_AGE) {
			snprintf(buffer, 32, "Age ≤ %d", options.maxAge);
			ui_createFilterTag(buffer, fb.maxAge);
		} else {
			gtk_entry_buffer_set_text(fb.maxAge, "", 1);
		}
		if (options.filterMask & FILTER_HAS_MIN_CA) {
			snprintf(buffer, 32, "CA ≥ %d", options.minCA);
			ui_createFilterTag(buffer, fb.minCA);
		} else {
			gtk_entry_buffer_set_text(fb.minCA, "", 1);
		}
		if (options.filterMask & FILTER_HAS_MAX_CA) {
			snprintf(buffer, 32, "CA ≤ %d", options.maxCA);
			ui_createFilterTag(buffer, fb.maxCA);
		} else {
			gtk_entry_buffer_set_text(fb.maxCA, "", 1);
		}
		if (options.filterMask & FILTER_HAS_MIN_PA) {
			snprintf(buffer, 32, "PA ≥ %d", options.minPA);
			ui_createFilterTag(buffer, fb.minPA);
		} else {
			gtk_entry_buffer_set_text(fb.minPA, "", 1);
		}
		if (options.filterMask & FILTER_HAS_MAX_PA) {
			snprintf(buffer, 32, "PA ≤ %d", options.maxPA);
			ui_createFilterTag(buffer, fb.maxPA);
		} else {
			gtk_entry_buffer_set_text(fb.maxPA, "", 1);
		}
		if (options.filterMask & FILTER_HAS_MIN_RATING) {
			snprintf(buffer, 32, "Rating ≥ %f.2%%", options.minRating);
			ui_createFilterTag(buffer, fb.minRating);
		} else {
			gtk_entry_buffer_set_text(fb.minRating, "", 1);
		}
		if (options.filterMask & FILTER_HAS_MAX_RATING) {
			snprintf(buffer, 32, "Rating ≤ %f.2%%", options.maxRating);
			ui_createFilterTag(buffer, fb.maxRating);
		} else {
			gtk_entry_buffer_set_text(fb.maxRating, "", 1);
		}

		return TRUE;
	}

	return FALSE;
}

G_MODULE_EXPORT void callbackShowCurrentPlayer(void) {
	#ifndef MOCKS_MODE
	if (processContext.handle == NULL) {
		LOG_ERROR("Process handle is NULL, cannot read current player");
		return;
	}
	#endif

	const uint32_t uniqueId = getCurrentPersonUniqueId(&processContext);
	showPlayerById(uniqueId);
}

static void showPlayerById(uint32_t uniqueId) {
	if (uniqueId == 0) {
		LOG_INFO("Unique ID not found.");
		return;
	}

	LOG_INFO("Searching for Player Unique ID: %u", uniqueId);

	#ifdef PLAYER_BY_ID
	const Player player = PLAYER_BY_ID;
	#else
	const Player player = getPlayerById(&processContext, uniqueId);
	#endif

	if (player.personAddress == 0) {
		LOG_ERROR("Player with Unique ID %u not found", uniqueId);
		return;
	}

	const WindowContext context = createPlayerInfoWindow(&player);
	renderPlayerInfoWindow(context, &player);
}

G_MODULE_EXPORT void callbackClearFilters(void) {
	searchHandler_clearFilters();

	ui_clearFilterTags();
	playerTable_populate(gameContext.table, NULL);
}
