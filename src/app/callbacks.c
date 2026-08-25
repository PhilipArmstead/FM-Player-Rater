// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "callbacks.h"
#include "app/data.h"
#include "app/mocks.h"
#include "app/player-table.h"
#include "app/player.h"
#include "app/search.h"
#include "app/ui.h"
#include "core/logger.h"
#include "platform/platform.h"


extern ProcessContext processContext;
extern GameContext gameContext;

static void showPlayerById(uint32_t uniqueId);

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
}

#define GET_ENTRY_BUFFER(builder, name) \
	gtk_entry_get_buffer(GTK_ENTRY(gtk_builder_get_object(builder, name)))
#define GET_ENTRY_TEXT(builder, name) \
	gtk_entry_buffer_get_text(GET_ENTRY_BUFFER(builder, name))

G_MODULE_EXPORT gboolean callbackFilterKeypress(
	GtkEventControllerKey *controller,
	guint keyval,
	guint keycode,
	GdkModifierType state,
	gpointer user_data
) {
	if (keyval == GDK_KEY_Return) {
		SearchOptions options = search_createContext();
		GtkBuilder *b = gameContext.builder;
		const gchar *minAge = GET_ENTRY_TEXT(b, "entry:age:min");
		const gchar *maxAge = GET_ENTRY_TEXT(b, "entry:age:max");
		const gchar *minCA = GET_ENTRY_TEXT(b, "entry:ca:min");
		const gchar *maxCA = GET_ENTRY_TEXT(b, "entry:ca:max");
		const gchar *minPA = GET_ENTRY_TEXT(b, "entry:pa:min");
		const gchar *maxPA = GET_ENTRY_TEXT(b, "entry:pa:max");
		const gchar *minRating = GET_ENTRY_TEXT(b, "entry:rating:min");
		const gchar *maxRating = GET_ENTRY_TEXT(b, "entry:rating:max");
		if (minAge != NULL && *minAge != '\0') {
			options.minAge = (uint8_t)g_ascii_strtoull(minAge, NULL, 10);
		}
		if (maxAge != NULL && *maxAge != '\0') {
			options.maxAge = (uint8_t)g_ascii_strtoull(maxAge, NULL, 10);
		}
		if (minCA != NULL && *minCA != '\0') {
			options.minCA = (uint8_t)g_ascii_strtoull(minCA, NULL, 10);
		}
		if (maxCA != NULL && *maxCA != '\0') {
			options.maxCA = (uint8_t)g_ascii_strtoull(maxCA, NULL, 10);
		}
		if (minPA != NULL && *minPA != '\0') {
			options.minPA = (uint8_t)g_ascii_strtoull(minPA, NULL, 10);
		}
		if (maxPA != NULL && *maxPA != '\0') {
			options.maxPA = (uint8_t)g_ascii_strtoull(maxPA, NULL, 10);
		}
		if (minRating != NULL && *minRating != '\0') {
			options.minRating = (uint8_t)g_ascii_strtoull(minRating, NULL, 10);
		}
		if (maxRating != NULL && *maxRating != '\0') {
			options.maxRating = (uint8_t)g_ascii_strtoull(maxRating, NULL, 10);
		}
		const uint32_t *playerIds = search_findPlayers(options);
		playerTable_populate(gameContext.table, playerIds);
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
	GtkBuilder *b = gameContext.builder;
	GtkEntryBuffer *minAge = GET_ENTRY_BUFFER(b, "entry:age:min");
	GtkEntryBuffer *maxAge = GET_ENTRY_BUFFER(b, "entry:age:max");
	GtkEntryBuffer *minCA = GET_ENTRY_BUFFER(b, "entry:ca:min");
	GtkEntryBuffer *maxCA = GET_ENTRY_BUFFER(b, "entry:ca:max");
	GtkEntryBuffer *minPA = GET_ENTRY_BUFFER(b, "entry:pa:min");
	GtkEntryBuffer *maxPA = GET_ENTRY_BUFFER(b, "entry:pa:max");
	GtkEntryBuffer *minRating = GET_ENTRY_BUFFER(b, "entry:rating:min");
	GtkEntryBuffer *maxRating = GET_ENTRY_BUFFER(b, "entry:rating:max");
	GtkEntryBuffer *clubSearch = GET_ENTRY_BUFFER(b, "entry:club-name");

	gtk_entry_buffer_set_text(minAge, "", 1);
	gtk_entry_buffer_set_text(maxAge, "", 1);
	gtk_entry_buffer_set_text(minCA, "", 1);
	gtk_entry_buffer_set_text(maxCA, "", 1);
	gtk_entry_buffer_set_text(minPA, "", 1);
	gtk_entry_buffer_set_text(maxPA, "", 1);
	gtk_entry_buffer_set_text(minRating, "", 1);
	gtk_entry_buffer_set_text(maxRating, "", 1);
	gtk_entry_buffer_set_text(clubSearch, "", 1);

	GtkBox *filterTags = GTK_BOX(gtk_builder_get_object(gameContext.builder, "box:filter-tags"));

	GtkWidget *child;
	while ((child = gtk_widget_get_first_child(GTK_WIDGET(filterTags))) != NULL) {
		gtk_box_remove(filterTags, GTK_WIDGET(child));
	}

	playerTable_populate(gameContext.table, NULL);
}
