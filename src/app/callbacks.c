// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "callbacks.h"
#include "app/data.h"
#include "app/game-status.h"
#include "app/maths.h"
#include "app/mocks.h"
#include "app/player.h"
#include "app/ui.h"
#include "core/logger.h"
#include "platform/platform.h"


extern ProcessContext processContext;
extern GameContext gameContext;
extern GtkBuilder *builder;

static void showPlayerById(uint32_t uniqueId);

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
