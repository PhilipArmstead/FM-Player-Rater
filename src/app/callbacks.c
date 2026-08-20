// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "callbacks.h"

#include "app/data.h"
#include "app/game-status.h"
#include "app/maths.h"
#include "app/player.h"
#include "app/ui.h"
#include "core/logger.h"
#include "platform/platform.h"


extern ProcessContext processContext;
extern GameContext gameContext;
extern GtkBuilder *builder;

#ifdef ARCH_WIN
#include <windows.h>

typedef HANDLE thread_t;
typedef HANDLE event_t;

#define THREAD_COUNT 4
thread_t threads[4];

static DWORD WINAPI threadFunction(const LPVOID arg) {
	const uint8_t func_num = (uint8_t)(intptr_t)arg;

	switch (func_num) {
		case 1: cacheClubs(processContext, &gameContext);
			break;
		case 2: cacheNations(processContext, &gameContext);
			break;
		case 3: cachePlayers(processContext, &gameContext, 0);
			break;
		case 4: cachePlayers(processContext, &gameContext, 1);
			break;
	}

	WaitForSingleObject(threads[0], 0);
	WaitForSingleObject(threads[1], 0);
	WaitForSingleObject(threads[2], 0);
	WaitForSingleObject(threads[3], 0);

	return 0;
}
#else
#include <pthread.h>
#include <semaphore.h>

typedef pthread_t thread_t;
typedef sem_t event_t;

static void *threadFunction(void *arg) {
	int func_num = (int)(intptr_t)arg;

	switch (func_num) {
		case 1: cacheClubs(processContext, &gameContext);
			break;
		case 2: cacheNations(processContext, &gameContext);
			break;
		case 3: cachePlayers(processContext, &gameContext, 0);
			break;
		case 4: cachePlayers(processContext, &gameContext, 1);
			break;
	}

	// Check if all threads are done
	pthread_tryjoin_np(threads[0], NULL);
	pthread_tryjoin_np(threads[1], NULL);
	pthread_tryjoin_np(threads[2], NULL);
	pthread_tryjoin_np(threads[3], NULL);

	return NULL;
}
#endif

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

	// Prepare player array for multithreaded writing
	uint8_t bytes[4];
	readFromMemory(processContext.handle, processContext.moduleBaseAddress + PLAYER_COUNT_PTR_BASE, 4, bytes);
	const uint64_t playerCount = hexBytesToInt(bytes, 4);
	gameContext.playerCount = playerCount;
	gameContext.players = malloc(playerCount * sizeof(Player));

	uint8_t i;

	#ifdef ARCH_WIN
	for (i = 0; i < THREAD_COUNT; i++) {
		threads[i] = CreateThread(
			NULL,
			0,
			threadFunction,
			(LPVOID)(intptr_t)(i + 1),
			0,
			NULL
		);

		if (threads[i] == NULL) {
			LOG_ERROR("Failed to create thread %d", i + 1);
			return;
		}
	}

	#else
	for (i = 0; i < THREAD_COUNT; i++) {
		if (pthread_create(
			&threads[i],
			NULL,
			threadFunction,
			(void*)(intptr_t)(i + 1)
		) != 0) {
			LOG_ERROR("Failed to create thread %d", i + 1);
		}
	}
	#endif
}
