// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "callbacks.h"

#include "app/data.h"
#include "app/game-status.h"
#include "app/player.h"
#include "app/ui.h"
#include "core/logger.h"
#include "platform/platform.h"


extern ProcessContext processContext;
extern GameContext gameContext;
extern GtkBuilder *builder;

static void showPlayerById(uint32_t uniqueId);
static void handleDisconnect(void);
static void handleConnect(void);

void connectToProcess(void) {
	clearCaches(&gameContext);
	platform_openProcess(&processContext);

	if (processContext.handle != NULL) {
		handleConnect();
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

gboolean update(gpointer userData) {
	// If we're connected to the process...
	if (processContext.handle != NULL) {
		// Get the current time/date
		DayMonthYear dayMonthYear = getDayMonthYear(&processContext);

		// Bail if the date is invalid and assume we're no longer connected
		if (dayMonthYear.day == 0 || dayMonthYear.year == 0) {
			handleDisconnect();
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

			updateInGameDate();
		}

		// Update the game version if it's different from the last one we saw
		char versionBuffer[GAME_STATUS_STRING_BUFFER_SIZE] = {0};
		getGameVersion(&processContext, versionBuffer, GAME_STATUS_STRING_BUFFER_SIZE);
		if (strncmp(versionBuffer, gameContext.gameVersion, GAME_STATUS_STRING_BUFFER_SIZE) != 0) {
			strncpy(gameContext.gameVersion, versionBuffer, GAME_STATUS_STRING_BUFFER_SIZE);
			LOG_INFO("Game Version: %s", versionBuffer);

			updateGameStatus();
		}

		const uint32_t uniqueId = getCurrentPersonUniqueId(&processContext);
		gameContext.currentlyViewedPlayer = getPlayerByIdPartial(&processContext, uniqueId);
		updateShowCurrentPlayerButton();
	} else {
		connectToProcess();
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

static void handleDisconnect(void) {
	processContext.handle = NULL;

	gameContext.currentlyViewedPlayer = (PartialPlayer){0};
	gameContext.gameVersion[0] = '\0';
	gameContext.currentDate = (DayMonthYear){0};

	updateUi();
}

static void handleConnect(void) {
	LOG_INFO(
		"Process opened with PID: %u (base module address of %p)",
		processContext.pid,
		(void*)processContext.moduleBaseAddress
	);
	updateUi();
	update(NULL);

	// TODO: multithread this?
	cacheClubs(processContext, &gameContext);
	cacheNations(processContext, &gameContext);
	// TODO: cache forenames
	// TODO: cache surnames
	// TODO: cache common names
}
