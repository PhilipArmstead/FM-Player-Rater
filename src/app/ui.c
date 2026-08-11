// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui.h"
#include "app/config.h"
#include "app/game-status.h"
#include "app/maths.h"
#include "app/player.h"
#include "app/helpers/date.h"
#include "core/logger.h"
#include "platform/platform.h"

#include <gtk/gtk.h>


extern ProcessContext processContext;
extern GameContext gameContext;

static void handleDisconnect(void);
static void handleConnect(void);
static inline void updateWhileConnected(void);
static inline void updateWhileDisconnected(void);

void connectToProcess(void) {
	platform_openProcess(&processContext);

	if (processContext.handle != NULL) {
		handleConnect();
	}
}

gboolean update(gpointer userData) {
	#ifndef MOCKS_MODE
	if (processContext.handle != NULL) {
		updateWhileConnected();
	} else {
		updateWhileDisconnected();
	}
	#else
	updateWhileConnected();
	#endif

	return G_SOURCE_CONTINUE;
}

void updateUi(void) {
	updateGameStatus();
}

static inline void updateWhileConnected(void) {
	// Get the current time/date
	DayMonthYear dayMonthYear = getDayMonthYear(&processContext);

	// Bail if the date is invalid and assume we're no longer connected
	if (dayMonthYear.day == 0 || dayMonthYear.year == 0) {
		handleDisconnect();
		return;
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
}

static inline void updateWhileDisconnected(void) {
	connectToProcess();
}

void updateInGameDate(void) {
	GtkLabel *dateLabel = GTK_LABEL(GTK_WIDGET(gtk_builder_get_object(gameContext.builder, "label:date")));

	#ifndef MOCKS_MODE
	if (processContext.handle == NULL || gameContext.currentDate.year <= 1970) {
		gtk_label_set_text(dateLabel, "\0");
		return;
	}
	#endif

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

	#ifndef MOCKS_MODE
	if (processContext.handle == NULL) {
		gtk_label_set_text(versionLabel, "Not connected");
		return;
	}
	#endif

	if (gameContext.gameVersion[0] == '\0') {
		gtk_label_set_text(versionLabel, "\0");
		return;
	}

	char buffer[64] = {0};
	snprintf(buffer, 64, "📝 %s", gameContext.gameVersion);
	gtk_label_set_text(versionLabel, buffer);
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
	// TODO: show all nationalities
	GtkBox *nationalityBox = GTK_BOX(GTK_WIDGET(gtk_builder_get_object(context.builder, "label:nationality")));
	char pathToFlag[256] = {0};
	snprintf(pathToFlag, sizeof(pathToFlag), "%s/assets/flags/%s.png", REPO_ROOT_DIR, player->nationality.code);
	GtkWidget *flagImage = gtk_image_new_from_file(pathToFlag);
	gtk_box_append(nationalityBox, flagImage);
	gtk_widget_set_tooltip_text(flagImage, player->nationality.name);

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

	// Attributes
	// TODO: normalise attributes
	// TODO: GK attributes
	// Technical
	GtkWidget *boxTechnical = GTK_WIDGET(gtk_builder_get_object(context.builder, "box:attribute:technical"));
	if (player->positions[0] < 12) {
		gtk_widget_set_visible(boxTechnical, true);
		// TODO: hide GK box

		// GtkLabel *accelerationLabel = GTK_LABEL(
		// GTK_WIDGET(gtk_builder_get_object(context.builder, "label:attribute:technical:acceleration"))
		// );
		GtkLabel *cornersLabel = GTK_LABEL(
			GTK_WIDGET(gtk_builder_get_object(context.builder, "label:attribute:technical:corners"))
		);
		GtkLabel *crossingLabel = GTK_LABEL(
			GTK_WIDGET(gtk_builder_get_object(context.builder, "label:attribute:technical:crossing"))
		);
		GtkLabel *dribblingLabel = GTK_LABEL(
			GTK_WIDGET(gtk_builder_get_object(context.builder, "label:attribute:technical:dribbling"))
		);
		GtkLabel *finishingLabel = GTK_LABEL(
			GTK_WIDGET(gtk_builder_get_object(context.builder, "label:attribute:technical:finishing"))
		);
		GtkLabel *firstTouchLabel = GTK_LABEL(
			GTK_WIDGET(gtk_builder_get_object(context.builder, "label:attribute:technical:first-touch"))
		);
		GtkLabel *freeKicksLabel = GTK_LABEL(
			GTK_WIDGET(gtk_builder_get_object(context.builder, "label:attribute:technical:free-kicks"))
		);
		GtkLabel *headingLabel = GTK_LABEL(
			GTK_WIDGET(gtk_builder_get_object(context.builder, "label:attribute:technical:heading"))
		);
		GtkLabel *longShotsLabel = GTK_LABEL(
			GTK_WIDGET(gtk_builder_get_object(context.builder, "label:attribute:technical:long-shots"))
		);
		GtkLabel *longThrowsLabel = GTK_LABEL(
			GTK_WIDGET(gtk_builder_get_object(context.builder, "label:attribute:technical:long-throws"))
		);
		GtkLabel *markingLabel = GTK_LABEL(
			GTK_WIDGET(gtk_builder_get_object(context.builder, "label:attribute:technical:marking"))
		);
		GtkLabel *passingLabel = GTK_LABEL(
			GTK_WIDGET(gtk_builder_get_object(context.builder, "label:attribute:technical:passing"))
		);
		GtkLabel *penaltyTakingLabel = GTK_LABEL(
			GTK_WIDGET(gtk_builder_get_object(context.builder, "label:attribute:technical:penalty-taking"))
		);
		GtkLabel *tacklingLabel = GTK_LABEL(
			GTK_WIDGET(gtk_builder_get_object(context.builder, "label:attribute:technical:tackling"))
		);
		GtkLabel *techniqueLabel = GTK_LABEL(
			GTK_WIDGET(gtk_builder_get_object(context.builder, "label:attribute:technical:technique"))
		);

		char buffer[8] = {0};
		snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[ATTR_COR]));
		gtk_label_set_text(cornersLabel, buffer);

		snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[ATTR_CRO]));
		gtk_label_set_text(crossingLabel, buffer);

		snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[ATTR_DRI]));
		gtk_label_set_text(dribblingLabel, buffer);

		snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[ATTR_FIN]));
		gtk_label_set_text(finishingLabel, buffer);

		snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[ATTR_FIR]));
		gtk_label_set_text(firstTouchLabel, buffer);

		snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[ATTR_FRE]));
		gtk_label_set_text(freeKicksLabel, buffer);

		snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[ATTR_HEA]));
		gtk_label_set_text(headingLabel, buffer);

		snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[ATTR_LON]));
		gtk_label_set_text(longShotsLabel, buffer);

		snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[ATTR_LTH]));
		gtk_label_set_text(longThrowsLabel, buffer);

		snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[ATTR_MAR]));
		gtk_label_set_text(markingLabel, buffer);

		snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[ATTR_PAS]));
		gtk_label_set_text(passingLabel, buffer);

		snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[ATTR_PEN]));
		gtk_label_set_text(penaltyTakingLabel, buffer);

		snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[ATTR_TCK]));
		gtk_label_set_text(tacklingLabel, buffer);

		snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[ATTR_TEC]));
		gtk_label_set_text(techniqueLabel, buffer);
	}

	// Mental
	GtkLabel *aggressionLabel = GTK_LABEL(
		GTK_WIDGET(gtk_builder_get_object(context.builder, "label:attribute:mental:aggression"))
	);
	GtkLabel *anticipationLabel = GTK_LABEL(
		GTK_WIDGET(gtk_builder_get_object(context.builder, "label:attribute:mental:anticipation"))
	);
	GtkLabel *braveryLabel = GTK_LABEL(
		GTK_WIDGET(gtk_builder_get_object(context.builder, "label:attribute:mental:bravery"))
	);
	GtkLabel *composureLabel = GTK_LABEL(
		GTK_WIDGET(gtk_builder_get_object(context.builder, "label:attribute:mental:composure"))
	);
	GtkLabel *concentrationLabel = GTK_LABEL(
		GTK_WIDGET(gtk_builder_get_object(context.builder, "label:attribute:mental:concentration"))
	);
	GtkLabel *decisionsLabel = GTK_LABEL(
		GTK_WIDGET(gtk_builder_get_object(context.builder, "label:attribute:mental:decisions"))
	);
	GtkLabel *determinationLabel = GTK_LABEL(
		GTK_WIDGET(gtk_builder_get_object(context.builder, "label:attribute:mental:determination"))
	);
	GtkLabel *flairLabel = GTK_LABEL(
		GTK_WIDGET(gtk_builder_get_object(context.builder, "label:attribute:mental:flair"))
	);
	GtkLabel *leadershipLabel = GTK_LABEL(
		GTK_WIDGET(gtk_builder_get_object(context.builder, "label:attribute:mental:leadership"))
	);
	GtkLabel *offTheBallLabel = GTK_LABEL(
		GTK_WIDGET(gtk_builder_get_object(context.builder, "label:attribute:mental:off-the-ball"))
	);
	GtkLabel *positioningLabel = GTK_LABEL(
		GTK_WIDGET(gtk_builder_get_object(context.builder, "label:attribute:mental:positioning"))
	);
	GtkLabel *teamworkLabel = GTK_LABEL(
		GTK_WIDGET(gtk_builder_get_object(context.builder, "label:attribute:mental:teamwork"))
	);
	GtkLabel *visionLabel = GTK_LABEL(
		GTK_WIDGET(gtk_builder_get_object(context.builder, "label:attribute:mental:vision"))
	);
	GtkLabel *workRateLabel = GTK_LABEL(
		GTK_WIDGET(gtk_builder_get_object(context.builder, "label:attribute:mental:work-rate"))
	);

	char buffer[8] = {0};
	snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[ATTR_AGG]));
	gtk_label_set_text(aggressionLabel, buffer);

	snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[ATTR_ANT]));
	gtk_label_set_text(anticipationLabel, buffer);

	snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[ATTR_BRA]));
	gtk_label_set_text(braveryLabel, buffer);

	snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[ATTR_CNT]));
	gtk_label_set_text(concentrationLabel, buffer);

	snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[ATTR_CMP]));
	gtk_label_set_text(composureLabel, buffer);

	snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[ATTR_DEC]));
	gtk_label_set_text(decisionsLabel, buffer);

	snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[ATTR_DET]));
	gtk_label_set_text(determinationLabel, buffer);

	snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[ATTR_FLA]));
	gtk_label_set_text(flairLabel, buffer);

	snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[ATTR_LDR]));
	gtk_label_set_text(leadershipLabel, buffer);

	snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[ATTR_OTB]));
	gtk_label_set_text(offTheBallLabel, buffer);

	snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[ATTR_POS]));
	gtk_label_set_text(positioningLabel, buffer);

	snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[ATTR_TEA]));
	gtk_label_set_text(teamworkLabel, buffer);

	snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[ATTR_VIS]));
	gtk_label_set_text(visionLabel, buffer);

	snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[ATTR_WOR]));
	gtk_label_set_text(workRateLabel, buffer);

	// Physical
	GtkLabel *accelerationLabel = GTK_LABEL(
		GTK_WIDGET(gtk_builder_get_object(context.builder, "label:attribute:physical:acceleration"))
	);
	GtkLabel *agilityLabel = GTK_LABEL(
		GTK_WIDGET(gtk_builder_get_object(context.builder, "label:attribute:physical:agility"))
	);
	GtkLabel *balanceLabel = GTK_LABEL(
		GTK_WIDGET(gtk_builder_get_object(context.builder, "label:attribute:physical:balance"))
	);
	GtkLabel *jumpingReachLabel = GTK_LABEL(
		GTK_WIDGET(gtk_builder_get_object(context.builder, "label:attribute:physical:jumping-reach"))
	);
	GtkLabel *naturalFitnessLabel = GTK_LABEL(
		GTK_WIDGET(gtk_builder_get_object(context.builder, "label:attribute:physical:natural-fitness"))
	);
	GtkLabel *paceLabel = GTK_LABEL(
		GTK_WIDGET(gtk_builder_get_object(context.builder, "label:attribute:physical:pace"))
	);
	GtkLabel *staminaLabel = GTK_LABEL(
		GTK_WIDGET(gtk_builder_get_object(context.builder, "label:attribute:physical:stamina"))
	);
	GtkLabel *strengthLabel = GTK_LABEL(
		GTK_WIDGET(gtk_builder_get_object(context.builder, "label:attribute:physical:strength"))
	);

	snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[ATTR_ACC]));
	gtk_label_set_text(accelerationLabel, buffer);

	snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[ATTR_AGI]));
	gtk_label_set_text(agilityLabel, buffer);

	snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[ATTR_BAL]));
	gtk_label_set_text(balanceLabel, buffer);

	snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[ATTR_JUM]));
	gtk_label_set_text(jumpingReachLabel, buffer);

	snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[ATTR_NAT]));
	gtk_label_set_text(naturalFitnessLabel, buffer);

	snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[ATTR_PAC]));
	gtk_label_set_text(paceLabel, buffer);

	snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[ATTR_STA]));
	gtk_label_set_text(staminaLabel, buffer);

	snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[ATTR_STR]));
	gtk_label_set_text(strengthLabel, buffer);

	// To colour the backgrounds of attributes,
	// get the max weight of the current groupedPosition weight system.
	// Low is max * 0.15, mid is max * 0.5, high is above this
}


static void handleDisconnect(void) {
	processContext.handle = NULL;

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
}
