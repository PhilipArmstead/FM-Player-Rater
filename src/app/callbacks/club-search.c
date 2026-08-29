// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "club-search.h"
#include "app/search-handler.h"
#include "app/ui.h"
#include "app/callbacks/filters.h"
#include "core/logger.h"
#include "platform/platform.h"


extern GameContext gameContext;

static void runThreadedSearch(SearchContext *context);

G_MODULE_EXPORT void callbacks_OnClubNameChange(GtkEditable *editable, SearchDatalist *dataList) {
	gameContext.filterOptions.filterMask &= ~FILTER_HAS_CLUB;

	const char *searchValue = gtk_editable_get_text(editable);
	if (strlen(searchValue) < 4) {
		gtk_popover_popdown(dataList->popover);
		return;
	}

	SearchContext *context = malloc(sizeof(SearchContext));
	context->dataList = dataList;
	context->count = 0;
	strncpy(context->searchValue, searchValue, CLUB_SEARCH_NAME_LENGTH - 1);
	context->searchValue[CLUB_SEARCH_NAME_LENGTH - 1] = '\0';

	runThreadedSearch(context);
}

G_MODULE_EXPORT void callbacks_onClubNameSelected(
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

	// Block the change handler while we update the text
	g_signal_handlers_block_by_func(
		dataList->entry,
		(gpointer)callbacks_OnClubNameChange,
		(gpointer)dataList
	);

	gtk_editable_set_text(GTK_EDITABLE(dataList->entry), text);

	// Unblock the handler
	g_signal_handlers_unblock_by_func(
		dataList->entry,
		(gpointer)callbacks_OnClubNameChange,
		(gpointer)dataList
	);

	gtk_popover_popdown(dataList->popover);

	gameContext.filterOptions.filterMask |= FILTER_HAS_CLUB;
	gameContext.filterOptions.clubIndex = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(child), "index"));
	char buffer[48] = {0};
	snprintf(buffer, 32, "Club: %s", gameContext.clubs[gameContext.filterOptions.clubIndex].shortName);
	ui_createClubFilterTag(buffer, GTK_EDITABLE(dataList->entry));

	// NOTE: this is the same as the on-enter handler
	searchHandler_doSearch(true);
	callbacks_updateFilterTags();
}

// This runs on the main thread; it's safe to modify the UI
static gboolean updateUIWithResults(gpointer userData) {
	SearchContext *context = userData;
	const SearchDatalist *dataList = context->dataList;
	gtk_popover_popup(dataList->popover);

	// Clear the list
	{
		GtkWidget *child;
		while ((child = gtk_widget_get_first_child(GTK_WIDGET(dataList->listBox))) != NULL) {
			gtk_list_box_remove(dataList->listBox, GTK_WIDGET(child));
		}
	}

	// Add matching options using results from worker thread
	for (uint64_t i = 0; i < context->count; i++) {
		uint64_t clubIndex = context->clubIndices[i];
		GtkWidget *label = gtk_label_new(gameContext.clubs[clubIndex].shortName);
		g_object_set_data(G_OBJECT(label), "index", GINT_TO_POINTER(clubIndex));
		gtk_widget_set_halign(label, GTK_ALIGN_START);
		gtk_list_box_append(dataList->listBox, label);
	}

	// Show/hide popover based on matches
	if (gtk_widget_get_first_child(GTK_WIDGET(dataList->listBox)) != NULL) {
		gtk_popover_popup(dataList->popover);
	} else {
		gtk_popover_popdown(dataList->popover);
	}

	free(context);
	return G_SOURCE_REMOVE; // Don't repeat this callback
}

// This runs on the worker thread; CPU-intensive work only
// TODO: sort by name
static void runSearch(SearchContext *context) {
	const int64_t timeStart = platform_getMicroseconds();
	const char *searchValue = context->searchValue;

	// Add matching options
	context->count = 0;
	for (uint64_t i = 0; i < gameContext.clubCount && context->count < CLUB_SEARCH_LIMIT; i++) {
		if (
			g_str_match_string(searchValue, gameContext.clubs[i].name, TRUE)
			|| g_str_match_string(searchValue, gameContext.clubs[i].shortName,TRUE)
		) {
			context->clubIndices[context->count] = i;
			++context->count;
		}
	}

	const int64_t timeEnd = platform_getMicroseconds();
	LOG_INFO("Searched %llu clubs in %llu microseconds", context->count, timeEnd - timeStart);

	// Schedule UI update on the main thread
	g_idle_add(updateUIWithResults, context);
}

#ifdef ARCH_WIN
static DWORD WINAPI threadFunction(const LPVOID arg) {
	runSearch(arg);
	return 0;
}
#else
static void *threadFunction(void *arg) {
	runSearch(arg);
	return NULL;
}
#endif

static void runThreadedSearch(SearchContext *context) {
	#ifdef ARCH_WIN
	if (CreateThread(
		NULL,
		0,
		threadFunction,
		context,
		0,
		NULL
	) == NULL) {
		LOG_ERROR("Failed to create thread.");
		free(context);
	}
	#else
	for (i = 0; i < THREAD_COUNT; i++) {
		if (pthread_create(
			NULL,
			NULL,
			threadFunction,
			context
		) != 0) {
			LOG_ERROR("Failed to create thread %d", i + 1);
			free(context);
		}
	}
	#endif
}
