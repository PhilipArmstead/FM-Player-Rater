// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui.h"
#include "app/helpers/date.h"
#include "core/logger.h"

#include <gtk/gtk.h>


extern ProcessContext processContext;
extern GameContext gameContext;

void updateUi(void) {
	updateGameStatus();
}

void updateInGameDate(void) {
	GtkLabel *dateLabel = GTK_LABEL(GTK_WIDGET(gtk_builder_get_object(gameContext.builder, "label:date")));

	if (processContext.handle == NULL || gameContext.currentDate.year <= 1970) {
		gtk_label_set_text(dateLabel, "\0");
		return;
	}

	char buffer[64] = {0};
	snprintf(
		buffer,
		64,
		"📅 %s %d%s, %d",
		gameContext.currentDate.month,
		gameContext.currentDate.day,
		getOrdinal(gameContext.currentDate.day),
		gameContext.currentDate.year
	);
	gtk_label_set_text(dateLabel, buffer);
}

void updateGameStatus(void) {
	GtkLabel *versionLabel = GTK_LABEL(GTK_WIDGET(gtk_builder_get_object(gameContext.builder, "label:status")));

	if (processContext.handle == NULL) {
		gtk_label_set_text(versionLabel, "Not connected");
		return;
	}

	if (gameContext.gameVersion[0] == '\0') {
		gtk_label_set_text(versionLabel, "\0");
		return;
	}

	char buffer[64] = {0};
	snprintf(buffer, 64, "📝 %s", gameContext.gameVersion);
	gtk_label_set_text(versionLabel, buffer);
}

void updateShowCurrentPlayerButton(void) {
	const PartialPlayer currentPlayer = gameContext.currentlyViewedPlayer;
	GtkWidget *buttonWidget = GTK_WIDGET(gtk_builder_get_object(gameContext.builder, "buttonShowCurrentPlayer"));
	GtkLabel *label = GTK_LABEL(GTK_WIDGET(gtk_builder_get_object(gameContext.builder, "labelShowCurrentPlayer")));
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
}

WindowContext openWindow(const char *layoutName, const char *windowName) {
	char pathToAppLayout[256] = {0};
	snprintf(pathToAppLayout, sizeof(pathToAppLayout), "%s/%s.ui", LAYOUTS_DIR, layoutName);

	WindowContext context = {};
	context.builder = gtk_builder_new_from_file(pathToAppLayout);
	context.window = GTK_WIDGET(gtk_builder_get_object(context.builder, windowName));
	gtk_window_present(GTK_WINDOW(context.window));
	return context;
}

WindowContext createPlayerInfoWindow(const Player *player) {
	const WindowContext context = openWindow("player-info", "window:player-info");
	gtk_window_set_default_size(GTK_WINDOW(context.window), 420, 900);

	char pathToStylesheet[256] = {0};
	GtkCssProvider *css_provider = gtk_css_provider_new();
	snprintf(pathToStylesheet, sizeof(pathToStylesheet), "%s/player-info.css", LAYOUTS_DIR);
	gtk_css_provider_load_from_path(css_provider, pathToStylesheet);
	gtk_style_context_add_provider_for_display(
		gdk_display_get_default(),
		GTK_STYLE_PROVIDER(css_provider),
		GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
	);
	g_object_unref(css_provider);

	LOG_INFO(
		"Found Player: %s (Age: %d, Ability: %d, Potential: %d, Row ID: %d, Address: 0x%08X)",
		player->commonName,
		player->age,
		player->ca,
		player->pa,
		player->rowId,
		player->personAddress
	);

	return context;
}

void renderPlayerInfoWindow(WindowContext context, const Player *player) {
	// Player name
	GtkLabel *commonNameLabel = GTK_LABEL(GTK_WIDGET(gtk_builder_get_object(context.builder, "label:common-name")));
	gtk_label_set_text(commonNameLabel, player->commonName);

	// Player age
	{
		char buffer[8];
		snprintf(buffer, 8, "%d yrs", player->age);
		GtkLabel *ageLabel = GTK_LABEL(GTK_WIDGET(gtk_builder_get_object(context.builder, "label:age")));
		gtk_label_set_text(ageLabel, buffer);
	}

	// Player status
	GtkWidget *isFastLearner = GTK_WIDGET(gtk_builder_get_object(context.builder, "widget:is-fast-learner"));
	gtk_widget_set_visible(isFastLearner, player->canDevelopQuickly);
	GtkWidget *isHotProspect = GTK_WIDGET(gtk_builder_get_object(context.builder, "widget:is-hot-prospect"));
	gtk_widget_set_visible(isHotProspect, player->isHotProspect);


	// Player nationality
	// TODO: find longest nationalities
	// TODO: show all nationalities
	GtkLabel *ageLabel = GTK_LABEL(GTK_WIDGET(gtk_builder_get_object(context.builder, "label:nationality")));
	gtk_label_set_text(ageLabel, player->nationality.name);

	// Club name
	GtkLabel *clubNameLabel = GTK_LABEL(GTK_WIDGET(gtk_builder_get_object(context.builder, "label:club-name")));
	GtkLabel *divisionNameLabel = GTK_LABEL(GTK_WIDGET(gtk_builder_get_object(context.builder, "label:division-name")));
	if (player->club.name[0] == '\0') {
		gtk_label_set_text(clubNameLabel, "Free agent");
		gtk_label_set_text(divisionNameLabel, "");
	} else {
		gtk_label_set_text(clubNameLabel, player->club.name);
		gtk_label_set_text(divisionNameLabel, player->club.divisionName);
	}
}
