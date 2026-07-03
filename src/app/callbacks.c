// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "callbacks.h"
#include "app/game-status.h"
#include "app/player.h"
#include "core/logger.h"
#include "platform/platform.h"


extern ProcessContext processContext;
extern GameContext gameContext;
extern GtkBuilder *builder;

static void showPlayerById(uint32_t uniqueId);

// TODO: move this to a helper or something
void connectToProcess(void) {
	platform_openProcess(&processContext);

	if (processContext.handle != NULL) {
		LOG_INFO(
			"Process opened with PID: %u (base module address of %p)",
			processContext.pid,
			(void*)processContext.moduleBaseAddress
		);
	}
}

G_MODULE_EXPORT void callbackShowCurrentPlayer(void) {
	if (processContext.handle == NULL) {
		LOG_ERROR("Process handle is NULL, cannot read current player");
		return;
	}

	const uint32_t uniqueId = getCurrentPersonUniqueId(&processContext);
	showPlayerById(uniqueId);
}

G_MODULE_EXPORT void callbackSearchByUid(void) {
	if (processContext.handle == NULL) {
		LOG_ERROR("Process handle is NULL, cannot search for player");
		return;
	}

	GtkEntry *entry = GTK_ENTRY(GTK_WIDGET(gtk_builder_get_object(builder, "uidEntry")));
	GtkEntryBuffer *entryBuffer = gtk_entry_get_buffer(entry);
	const char *buffer = gtk_entry_buffer_get_text(entryBuffer);
	if (buffer == NULL) {
		LOG_ERROR("UID entry is empty");
		return;
	}

	const uint32_t uniqueId = (uint32_t)strtoul(buffer, NULL, 10);
	showPlayerById(uniqueId);
}

gboolean update(gpointer userData) {
	// If we're connected to the process...
	if (processContext.handle != NULL) {
		// Get the current time/date
		DayMonthYear dayMonthYear = getDayMonthYear(&processContext);

		// Bail if the date is invalid and assume we're no longer connected
		if (dayMonthYear.day == 0 || dayMonthYear.year == 0) {
			processContext.handle = NULL;
			return G_SOURCE_CONTINUE;
		}

		// Cache the new date if it's different from the last one we saw
		if (
			dayMonthYear.day != gameContext.currentDate.day ||
			dayMonthYear.year != gameContext.currentDate.year ||
			strncmp(dayMonthYear.month, gameContext.currentDate.month, MONTH_NAME_LENGTH) != 0
		) {
			gameContext.currentDate = dayMonthYear;
			LOG_INFO(
				"Current Date: %s %d, %d",
				dayMonthYear.month,
				dayMonthYear.day,
				dayMonthYear.year
			);
		}

		// Update the game version if it's different from the last one we saw
		char versionBuffer[GAME_STATUS_STRING_BUFFER_SIZE] = {0};
		getGameVersion(&processContext, versionBuffer, GAME_STATUS_STRING_BUFFER_SIZE);
		if (strncmp(versionBuffer, gameContext.gameVersion, GAME_STATUS_STRING_BUFFER_SIZE) != 0) {
			strncpy(gameContext.gameVersion, versionBuffer, GAME_STATUS_STRING_BUFFER_SIZE);
			LOG_INFO("Game Version: %s", versionBuffer);
		}

		const uint32_t uniqueId = getCurrentPersonUniqueId(&processContext);
		PartialPlayer currentPlayer = getPlayerByIdPartial(&processContext, uniqueId);
		GtkWidget *buttonWidget = GTK_WIDGET(gtk_builder_get_object(builder, "buttonShowCurrentPlayer"));
		GtkLabel *label = GTK_LABEL(GTK_WIDGET(gtk_builder_get_object(builder, "labelShowCurrentPlayer")));
		if (currentPlayer.uid != 0) {
			#define PLAYER_NAME_SIZE 64
			char playerName[PLAYER_NAME_SIZE] = {0};
			if (currentPlayer.commonName[0] != '\0') {
				snprintf(playerName, PLAYER_NAME_SIZE, "%s", currentPlayer.commonName);
			} else {
				snprintf(playerName, PLAYER_NAME_SIZE, "%s %s", currentPlayer.forename, currentPlayer.surname);
			}
			char buffer[128] = {0};
			snprintf(
				buffer,
				sizeof(buffer),
				"%s\n(%s, ID: %u)",
				"Show current player",
				playerName,
				currentPlayer.uid
			);
			gtk_label_set_text(label, buffer);
			gtk_widget_set_sensitive(buttonWidget, true);
		} else {
			gtk_label_set_text(label, "Show current player");
			gtk_widget_set_sensitive(buttonWidget, false);
		}
	} else {
		// If we're not connected, try to connect and run this function again
		connectToProcess();
		if (processContext.handle != NULL) {
			update(userData);
		}
	}

	// Keep repeating
	return G_SOURCE_CONTINUE;
}

static void showPlayerById(uint32_t uniqueId) {
	if (uniqueId == 0) {
		LOG_INFO("Unique ID not found.");
		return;
	}

	LOG_INFO("Searching for Player Unique ID: %u", uniqueId);

	const Player player = getPlayerById(&processContext, uniqueId);
	if (player.personAddress == 0) {
		LOG_ERROR("Player with Unique ID %u not found", uniqueId);
		return;
	}

	LOG_INFO(
		"Found Player: %s (Age: %d, Ability: %d, Potential: %d, Row ID: %d, Address: 0x%08X)",
		player.commonName,
		player.age,
		player.ca,
		player.pa,
		player.rowId,
		player.personAddress
	);
}
