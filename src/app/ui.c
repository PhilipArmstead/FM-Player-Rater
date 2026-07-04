// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui.h"
#include "app/types.h"
#include "app/helpers/date.h"

#include <gtk/gtk.h>


extern ProcessContext processContext;
extern GtkBuilder *builder;
extern GameContext gameContext;

void updateUi(void) {
	updateGameStatus();
}

void updateInGameDate(void) {
	GtkLabel *dateLabel = GTK_LABEL(GTK_WIDGET(gtk_builder_get_object(builder, "footer:label:date")));

	if (processContext.handle == NULL || gameContext.currentDate.year <= 1970) {
		gtk_label_set_text(dateLabel, "\0");
		return;
	}

	char buffer[64] = {0};
	snprintf(
		buffer,
		64,
		"Date: %s %d%s, %d",
		gameContext.currentDate.month,
		gameContext.currentDate.day,
		getOrdinal(gameContext.currentDate.day),
		gameContext.currentDate.year
	);
	gtk_label_set_text(dateLabel, buffer);
}

void updateGameStatus(void) {
	GtkLabel *versionLabel = GTK_LABEL(GTK_WIDGET(gtk_builder_get_object(builder, "footer:label:status")));

	if (processContext.handle == NULL) {
		gtk_label_set_text(versionLabel, "Not connected");
		return;
	}

	if (gameContext.gameVersion[0] == '\0') {
		gtk_label_set_text(versionLabel, "\0");
		return;
	}

	char buffer[64] = {0};
	snprintf(buffer, 64, "Game version %s", gameContext.gameVersion);
	gtk_label_set_text(versionLabel, buffer);
}

void updateShowCurrentPlayerButton(void) {
	const PartialPlayer currentPlayer = gameContext.currentlyViewedPlayer;
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
}
