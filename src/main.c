// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtk/gtk.h>

#include "app/game-status.h"
#include "app/player.h"
#include "core/logger.h"
#include "platform/platform.h"


static ProcessContext processContext = {0};
static GameContext gameContext = {0};
static GtkBuilder *builder;

G_MODULE_EXPORT void callbackConnect(void);
G_MODULE_EXPORT void callbackShowCurrentPlayer(void);
static gboolean updateGameDetails(gpointer userData);

static void activate(GtkApplication *app) {
	// TODO: do something about this path
	builder = gtk_builder_new_from_file("../layouts/hello-world.ui");

	GtkWidget *window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
	gtk_window_set_application(GTK_WINDOW(window), GTK_APPLICATION(app));
	// g_object_unref(builder);

	// Periodic callbacks
	g_timeout_add(3000, updateGameDetails, NULL);

	gtk_window_present(GTK_WINDOW(window));
}

int main(const int argc, char **argv) {
	logger_init();

	GtkApplication *app = gtk_application_new("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	const int status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);

	logger_shutdown();

	return status;
}

G_MODULE_EXPORT void callbackConnect(void) {
	GtkButton *button = GTK_BUTTON(GTK_WIDGET(gtk_builder_get_object(builder, "connectButton")));
	if (processContext.handle != NULL) {
		gtk_button_set_label(button, "Connect");
		processContext.handle = NULL;
		return;
	}

	platform_openProcess(&processContext);
	if (processContext.handle == NULL) {
		LOG_ERROR("Failed to open process");
		return;
	}

	LOG_INFO(
		"Process opened with PID: %u (base module address of %p)",
		processContext.pid,
		(void*)processContext.moduleBaseAddress
	);

	gtk_button_set_label(button, "Disconnect");
}

G_MODULE_EXPORT void callbackShowCurrentPlayer(void) {
	if (processContext.handle == NULL) {
		LOG_ERROR("Process handle is NULL, cannot read current player");
		return;
	}

	const uint32_t uniqueId = getCurrentPersonUniqueId(&processContext);
	LOG_INFO("Current Player Unique ID: %u", uniqueId);

	const Player player = getPlayerById(&processContext, uniqueId);
	LOG_INFO(
		"Current Player: %s (Age: %d, Ability: %d, Potential: %d)",
		player.commonName,
		player.age,
		player.ca,
		player.pa
	);
}

static gboolean updateGameDetails(gpointer userData) {
	if (processContext.handle != NULL) {
		DayMonthYear dayMonthYear = getDayMonthYear(&processContext);
		if (
			dayMonthYear.day != gameContext.currentDate.day ||
			strncmp(dayMonthYear.month, gameContext.currentDate.month, MONTH_NAME_LENGTH) != 0 ||
			dayMonthYear.year != gameContext.currentDate.year
		) {
			gameContext.currentDate = dayMonthYear;
			LOG_INFO(
				"Current Date: %s %d, %d",
				dayMonthYear.month,
				dayMonthYear.day,
				dayMonthYear.year
			);
		}

		char versionBuffer[GAME_STATUS_STRING_BUFFER_SIZE] = {0};
		getGameVersion(&processContext, versionBuffer, GAME_STATUS_STRING_BUFFER_SIZE);
		if (strncmp(versionBuffer, gameContext.gameVersion, GAME_STATUS_STRING_BUFFER_SIZE) != 0) {
			strncpy(gameContext.gameVersion, versionBuffer, GAME_STATUS_STRING_BUFFER_SIZE);
			LOG_INFO("Game Version: %s", versionBuffer);
		}
	}

	// Keep repeating
	return G_SOURCE_CONTINUE;
}

// TODO: find by rowId?
// TODO: find by person address?
