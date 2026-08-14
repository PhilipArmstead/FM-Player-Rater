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

// static bool hasPolledNationalities = false;

void updateUi(void) {
	updateGameStatus();

	// if (!hasPolledNationalities) {
	// 	uint8_t bytes[4];
	// 	hasPolledNationalities = true;
	// 	readFromMemory(
	// 		processContext.handle,
	// 		processContext.moduleBaseAddress + COMPETITION_LIST_PTR_BASE,
	// 		4,
	// 		bytes
	// 	);
	// 	const uint32_t allCompetitions = (uint32_t)hexBytesToInt(bytes, 4);
	// 	char longestCompetitionName[64] = {0};
	// 	uint32_t a = 0;
	// 	size_t longestCompetitionNameLength = 0;
	//
	// 	for (uint8_t i = 0; i < 200; ++i) {
	// 		readFromMemory(processContext.handle, allCompetitions + COMPETITION_LIST_PTR_OFFSET_1, 4, bytes);
	// 		readFromMemory(
	// 			processContext.handle,
	// 			(uint32_t)hexBytesToInt(bytes, 4) + COMPETITION_LIST_PTR_OFFSET_2,
	// 			4,
	// 			bytes
	// 		);
	// 		readFromMemory(
	// 			processContext.handle,
	// 			(uint32_t)hexBytesToInt(bytes, 4) + COMPETITION_LIST_PTR_OFFSET_3 * i,
	// 			4,
	// 			bytes
	// 		);
	// 		const uint32_t competitionAddress = (uint32_t)hexBytesToInt(bytes, 4);
	// 		char name[CLUB_NAME_LENGTH] = {0};
	// 		readFromMemory(processContext.handle, competitionAddress + COMPETITION_OFFSET_LONG_NAME_ADDRESS, 4, bytes);
	// 		readFromMemory(
	// 			processContext.handle,
	// 			hexBytesToInt(bytes, 4) + GENERAL_OFFSET_NAME,
	// 			CLUB_NAME_LENGTH,
	// 			(uint8_t*)name
	// 		);
	// 		const size_t length = strlen(name);
	// 		if (length > longestCompetitionNameLength) {
	// 			longestCompetitionNameLength = length;
	// 			strncpy(longestCompetitionName, name, CLUB_NAME_LENGTH - 1);
	// 			longestCompetitionName[length] = '\0'; // Ensure null-termination
	// 			a = competitionAddress;
	// 		}
	// 	}
	//
	// 	LOG_INFO(
	// 		"Longest Competition Name: %s (%d chars, Address: 0x%" PRIx32 ")",
	// 		longestCompetitionName,
	// 		longestCompetitionNameLength,
	// 		a
	// 	);
	// }
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


	// Player nationalities
	uint8_t nationalityIndex = 0;
	while (nationalityIndex < 4 && player->nationality[nationalityIndex].name[0] != '\0') {
		GtkBox *nationalityBox = GTK_BOX(GTK_WIDGET(gtk_builder_get_object(context.builder, "label:nationality")));
		char pathToFlag[256] = {0};
		snprintf(
			pathToFlag,
			sizeof(pathToFlag),
			"%s/assets/flags/%s.png",
			REPO_ROOT_DIR,
			player->nationality[nationalityIndex].code
		);
		GtkWidget *flagImage = gtk_image_new_from_file(pathToFlag);
		gtk_box_append(nationalityBox, flagImage);
		gtk_widget_set_tooltip_text(flagImage, player->nationality[nationalityIndex].name);
		nationalityIndex++;
	}

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

	float attr_weights[ATTRIBUTE_COUNT];
	float pers_weights[8];
	float totalWeight = 1.0f;
	// TODO: this position should not be hardcoded
	getWeightsForPosition(POSITION_GROUPED_ST, attr_weights, pers_weights, &totalWeight);
	float max = 0;
	for (int i = 0; i < ATTRIBUTE_COUNT; ++i) {
		if (attr_weights[i] > max) {
			max = attr_weights[i];
		}
	}

	char buffer[8] = {0};
	char widgetId[64];
	GtkLabel *label;
	GtkWidget *widget;
	#define setRowTextAndHighlight(id, attributeIndex) {																	\
		snprintf(buffer, 8, "%d", convertTo20Scale(player->attributes[attributeIndex]));		\
		snprintf(widgetId, 64, "label:%s", id);																							\
		label = GTK_LABEL(GTK_WIDGET(gtk_builder_get_object(context.builder, widgetId)));		\
		gtk_label_set_text(label, buffer);																									\
		snprintf(widgetId, 64, "row:%s", id);																								\
		widget = GTK_WIDGET(gtk_builder_get_object(context.builder, widgetId));							\
		if (attr_weights[attributeIndex] > max * 0.5f) {																		\
			gtk_widget_add_css_class(widget, "attribute-row--high");													\
		} else if (attr_weights[attributeIndex] > max * 0.15f) {														\
			gtk_widget_add_css_class(widget, "attribute-row--mid");														\
		}																																										\
	}

	// Attributes
	// TODO: GK attributes
	// Technical
	GtkWidget *boxTechnical = GTK_WIDGET(gtk_builder_get_object(context.builder, "box:attribute:technical"));
	if (player->positions[0] < 12) {
		gtk_widget_set_visible(boxTechnical, true);
		// TODO: hide GK box

		setRowTextAndHighlight("attribute:technical:corners", ATTR_COR);
		setRowTextAndHighlight("attribute:technical:crossing", ATTR_CRO);
		setRowTextAndHighlight("attribute:technical:dribbling", ATTR_DRI);
		setRowTextAndHighlight("attribute:technical:finishing", ATTR_FIN);
		setRowTextAndHighlight("attribute:technical:first-touch", ATTR_FIR);
		setRowTextAndHighlight("attribute:technical:free-kicks", ATTR_FRE);
		setRowTextAndHighlight("attribute:technical:heading", ATTR_HEA);
		setRowTextAndHighlight("attribute:technical:long-shots", ATTR_LON);
		setRowTextAndHighlight("attribute:technical:long-throws", ATTR_LTH);
		setRowTextAndHighlight("attribute:technical:marking", ATTR_MAR);
		setRowTextAndHighlight("attribute:technical:passing", ATTR_PAS);
		setRowTextAndHighlight("attribute:technical:penalty-taking", ATTR_PEN);
		setRowTextAndHighlight("attribute:technical:tackling", ATTR_TCK);
		setRowTextAndHighlight("attribute:technical:technique", ATTR_TEC);
	}

	setRowTextAndHighlight("attribute:mental:aggression", ATTR_COR);
	setRowTextAndHighlight("attribute:mental:anticipation", ATTR_ANT);
	setRowTextAndHighlight("attribute:mental:bravery", ATTR_BRA);
	setRowTextAndHighlight("attribute:mental:concentration", ATTR_CNT);
	setRowTextAndHighlight("attribute:mental:composure", ATTR_CMP);
	setRowTextAndHighlight("attribute:mental:decisions", ATTR_DEC);
	setRowTextAndHighlight("attribute:mental:determination", ATTR_DET);
	setRowTextAndHighlight("attribute:mental:flair", ATTR_FLA);
	setRowTextAndHighlight("attribute:mental:leadership", ATTR_LDR);
	setRowTextAndHighlight("attribute:mental:off-the-ball", ATTR_OTB);
	setRowTextAndHighlight("attribute:mental:positioning", ATTR_POS);
	setRowTextAndHighlight("attribute:mental:teamwork", ATTR_TEA);
	setRowTextAndHighlight("attribute:mental:vision", ATTR_VIS);
	setRowTextAndHighlight("attribute:mental:work-rate", ATTR_WOR);

	setRowTextAndHighlight("attribute:physical:acceleration", ATTR_ACC);
	setRowTextAndHighlight("attribute:physical:agility", ATTR_AGI);
	setRowTextAndHighlight("attribute:physical:balance", ATTR_BAL);
	setRowTextAndHighlight("attribute:physical:jumping-reach", ATTR_JUM);
	setRowTextAndHighlight("attribute:physical:natural-fitness", ATTR_NAT);
	setRowTextAndHighlight("attribute:physical:pace", ATTR_PAC);
	setRowTextAndHighlight("attribute:physical:stamina", ATTR_STA);
	setRowTextAndHighlight("attribute:physical:strength", ATTR_STR);

	// Ratings
	{
		static const char *labels[POSITION_GROUPED_COUNT] = {
			"Goalkeeper",
			"Full back",
			"Centre back",
			"Wing back",
			"Defensive midfielder",
			"Midfielder",
			"Winger",
			"Attacking midfielder",
			"Striker"
		};
		GtkBox *boxRoles = GTK_BOX(GTK_WIDGET(gtk_builder_get_object(context.builder, "box:top-roles")));
		uint8_t i = 0;
		while (i < POSITION_GROUPED_COUNT && player->ratings[i].value > 0.f) {
			GtkWidget *parent = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
			gtk_box_append(boxRoles, parent);

			GtkWidget *labelContainer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
			GtkWidget *roleLabel = gtk_label_new(labels[player->ratings[i].position]);
			gtk_label_set_xalign(GTK_LABEL(roleLabel), 0);
			gtk_widget_add_css_class(roleLabel, "heading");
			gtk_widget_set_hexpand(roleLabel, true);
			gtk_box_append(GTK_BOX(labelContainer), roleLabel);

			char valueBuffer[8] = {0};
			snprintf(valueBuffer, 8, "%.1f", player->ratings[i].value);
			GtkWidget *roleValue = gtk_label_new(valueBuffer);
			gtk_widget_add_css_class(roleValue, "heading");
			gtk_widget_add_css_class(roleValue, "accent-orange");
			gtk_box_append(GTK_BOX(labelContainer), roleValue);

			GtkWidget *progressBar = gtk_progress_bar_new();
			gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressBar), player->ratings[i].value / 100.0f);
			gtk_widget_add_css_class(progressBar, "role-bar");

			gtk_box_append(GTK_BOX(parent), labelContainer);
			gtk_box_append(GTK_BOX(parent), progressBar);
			//  child: GtkBox
			//    child: GtkLabel
			//    child: GtkLabel
			//    value: value
			//  child: GtkProgressBar
			//  <property name="fraction">value as float, normalise to 1.0</property>

			i++;
		}
	}
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
