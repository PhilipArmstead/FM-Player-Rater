// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui.h"
#include "app/config.h"
#include "app/data.h"
#include "app/game-status.h"
#include "app/maths.h"
#include "app/player.h"
#include "app/search-handler.h"
#include "app/helpers/date.h"
#include "core/logger.h"
#include "platform/platform.h"

#include <gtk/gtk.h>

#include "player-table.h"


extern ProcessContext processContext;
extern GameContext gameContext;

static void handleDisconnect(void);
static void handleConnect(void);
static inline void updateWhileConnected(void);
static inline void updateWhileDisconnected(void);
static void onFilterTagClick(GtkWidget *self, GtkEntryBuffer *buffer);
static void onClubFilterTagClick(GtkWidget *self, GtkEditable *buffer);
static void loadStylesheet(const char *fileName);

void ui_init(GtkApplication *app) {
	loadStylesheet("show-players.css");

	// Show main window
	const WindowContext context = openWindow("show-players", "window:show-players");
	gameContext.builder = context.builder;
	gtk_window_set_application(GTK_WINDOW(context.window), GTK_APPLICATION(app));

	#ifdef MOCKS_MODE
	// In mock mode, we never have the process-connected callback run
	runMultiThreadedCache();
	#endif

	// Periodic callbacks
	g_timeout_add(1000, update, NULL);
	update(NULL);

	// Create datalist box
	SearchDatalist *dataList = g_new0(SearchDatalist, 1);

	dataList->entry = GTK_SEARCH_ENTRY(gtk_search_entry_new());
	dataList->popover = GTK_POPOVER(gtk_popover_new());
	dataList->listBox = GTK_LIST_BOX(gtk_list_box_new());

	gtk_search_entry_set_placeholder_text(dataList->entry, "Search club");
	gtk_widget_add_css_class(GTK_WIDGET(dataList->entry), "sidebar-search");

	gtk_popover_set_child(dataList->popover, GTK_WIDGET(dataList->listBox));
	gtk_widget_set_parent(GTK_WIDGET(dataList->popover), GTK_WIDGET(dataList->entry));

	gtk_popover_set_pointing_to(dataList->popover, &(GdkRectangle){102, 27, 1, 1});
	gtk_popover_set_position(dataList->popover, GTK_POS_BOTTOM);

	gtk_popover_set_autohide(dataList->popover, FALSE);
	gtk_widget_set_can_focus(GTK_WIDGET(dataList->popover), FALSE);
	gtk_widget_set_can_focus(GTK_WIDGET(dataList->listBox), FALSE);
	gtk_list_box_set_selection_mode(dataList->listBox, GTK_SELECTION_NONE);

	GtkBox *container = GTK_BOX(gtk_builder_get_object(gameContext.builder, "box:club-search-container"));
	gtk_box_append(container, GTK_WIDGET(dataList->entry));
	gameContext.dataList = dataList;

	// Cache field buffers
	GtkBuilder *b = gameContext.builder;
	FilterBuffer *buf = &gameContext.filterBuffer;
	buf->minAge = gtk_entry_get_buffer(GTK_ENTRY(gtk_builder_get_object(b, "entry:age:min")));
	buf->maxAge = gtk_entry_get_buffer(GTK_ENTRY(gtk_builder_get_object(b, "entry:age:max")));
	buf->minCA = gtk_entry_get_buffer(GTK_ENTRY(gtk_builder_get_object(b, "entry:ca:min")));
	buf->maxCA = gtk_entry_get_buffer(GTK_ENTRY(gtk_builder_get_object(b, "entry:ca:max")));
	buf->minPA = gtk_entry_get_buffer(GTK_ENTRY(gtk_builder_get_object(b, "entry:pa:min")));
	buf->maxPA = gtk_entry_get_buffer(GTK_ENTRY(gtk_builder_get_object(b, "entry:pa:max")));
	buf->minRating = gtk_entry_get_buffer(GTK_ENTRY(gtk_builder_get_object(b, "entry:rating:min")));
	buf->maxRating = gtk_entry_get_buffer(GTK_ENTRY(gtk_builder_get_object(b, "entry:rating:max")));
}

void connectToProcess(void) {
	clearCaches();
	platform_openProcess(&processContext);

	if (processContext.handle != NULL) {
		handleConnect();
	}
}

gboolean update(gpointer userData) {
	(void)userData;

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
		gtk_label_set_text(dateLabel, "");
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

WindowContext createPlayerInfoWindow(void) {
	const WindowContext context = openWindow("player-info", "window:player-info");
	gtk_window_set_default_size(GTK_WINDOW(context.window), 420, 900);

	loadStylesheet("player-info.css");

	return context;
}

void renderPlayerInfoWindow(WindowContext context, const Player *player) {
	// Player name
	GtkLabel *commonNameLabel = GTK_LABEL(GTK_WIDGET(gtk_builder_get_object(context.builder, "label:common-name")));
	if (player->commonName[0] == '\0') {
		char buffer[PERSON_FORENAME_LENGTH + PERSON_SURNAME_LENGTH + 2];
		snprintf(buffer, sizeof(buffer), "%s %s", player->forename, player->surname);
		gtk_label_set_text(commonNameLabel, buffer);
	} else {
		gtk_label_set_text(commonNameLabel, player->commonName);
	}

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
	while (nationalityIndex < 4 && player->nationality[nationalityIndex] != 0xFF) {
		GtkBox *nationalityBox = GTK_BOX(GTK_WIDGET(gtk_builder_get_object(context.builder, "label:nationality")));
		char pathToFlag[256] = {0};
		const Nation nation = gameContext.nations[player->nationality[nationalityIndex]];
		snprintf(
			pathToFlag,
			sizeof(pathToFlag),
			"%s/assets/flags/%s.png",
			REPO_ROOT_DIR,
			nation.code
		);
		GtkWidget *flagImage = gtk_image_new_from_file(pathToFlag);
		gtk_box_append(nationalityBox, flagImage);
		gtk_widget_set_tooltip_text(flagImage, nation.name);
		nationalityIndex++;
	}

	// Club name
	GtkLabel *clubNameLabel = GTK_LABEL(GTK_WIDGET(gtk_builder_get_object(context.builder, "label:club-name")));
	if (player->clubIndex == -1) {
		gtk_label_set_text(clubNameLabel, "Free agent");
	} else {
		const Club club = gameContext.clubs[player->clubIndex];
		gtk_label_set_text(clubNameLabel, club.name);
	}

	float attrWeights[ATTRIBUTE_COUNT];
	float persWeights[8];
	float totalWeight = 1.0f;
	getWeightsForPosition(player->ratings[0].position, attrWeights, persWeights, &totalWeight);
	float max = 0;
	for (int i = 0; i < ATTRIBUTE_COUNT; ++i) {
		if (attrWeights[i] > max) {
			max = attrWeights[i];
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
		if (attrWeights[attributeIndex] > max * 0.5f) {																		\
			gtk_widget_add_css_class(widget, "attribute-row--high");													\
		} else if (attrWeights[attributeIndex] > max * 0.15f) {														\
			gtk_widget_add_css_class(widget, "attribute-row--mid");														\
		}																																										\
	}

	// Ability scores
	{
		GtkLabel *caLabel = GTK_LABEL(GTK_WIDGET(gtk_builder_get_object(context.builder, "label:ca")));
		GtkLabel *paLabel = GTK_LABEL(GTK_WIDGET(gtk_builder_get_object(context.builder, "label:pa")));
		char ability[4] = {0};
		snprintf(ability, 4, "%d", player->ca);
		gtk_label_set_label(caLabel, ability);
		snprintf(ability, 4, "%d", player->pa);
		gtk_label_set_label(paLabel, ability);
	}

	// Attributes
	GtkWidget *boxGoalkeeper = GTK_WIDGET(gtk_builder_get_object(context.builder, "box:attribute:goalkeeper"));
	GtkWidget *boxTechnical = GTK_WIDGET(gtk_builder_get_object(context.builder, "box:attribute:technical"));

	if (player->positions[0] < 12) {
		gtk_widget_set_visible(boxTechnical, true);
		gtk_widget_set_visible(boxGoalkeeper, false);

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
	} else {
		gtk_widget_set_visible(boxGoalkeeper, true);
		gtk_widget_set_visible(boxTechnical, false);

		setRowTextAndHighlight("attribute:goalkeeper:aerial-reach", ATTR_AER);
		setRowTextAndHighlight("attribute:goalkeeper:command-of-area", ATTR_CMD);
		setRowTextAndHighlight("attribute:goalkeeper:communication", ATTR_COM);
		setRowTextAndHighlight("attribute:goalkeeper:eccentricity", ATTR_ECC);
		setRowTextAndHighlight("attribute:goalkeeper:first-touch", ATTR_FIR);
		setRowTextAndHighlight("attribute:goalkeeper:handling", ATTR_HAN);
		setRowTextAndHighlight("attribute:goalkeeper:kicking", ATTR_KIC);
		setRowTextAndHighlight("attribute:goalkeeper:one-on-ones", ATTR_ONE);
		setRowTextAndHighlight("attribute:goalkeeper:passing", ATTR_PAS);
		setRowTextAndHighlight("attribute:goalkeeper:punching-tendency", ATTR_TTP);
		setRowTextAndHighlight("attribute:goalkeeper:reflexes", ATTR_REF);
		setRowTextAndHighlight("attribute:goalkeeper:rushing-out-tendency", ATTR_TRO);
		setRowTextAndHighlight("attribute:goalkeeper:throwing", ATTR_THR);
	}

	setRowTextAndHighlight("attribute:mental:aggression", ATTR_AGG);
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

	setRowTextAndHighlight("attribute:hidden:adaptability", ATTR_ADA);
	setRowTextAndHighlight("attribute:hidden:ambition", ATTR_AMB);
	setRowTextAndHighlight("attribute:hidden:consistency", ATTR_CON);
	setRowTextAndHighlight("attribute:hidden:controversy", ATTR_CNY);
	setRowTextAndHighlight("attribute:hidden:dirtiness", ATTR_DIR);
	setRowTextAndHighlight("attribute:hidden:important-matches", ATTR_IMP);
	setRowTextAndHighlight("attribute:hidden:injury-proneness", ATTR_INJ);
	setRowTextAndHighlight("attribute:hidden:loyalty", ATTR_LOY);
	setRowTextAndHighlight("attribute:hidden:pressure", ATTR_PRE);
	setRowTextAndHighlight("attribute:hidden:professionalism", ATTR_PRO);
	setRowTextAndHighlight("attribute:hidden:sportsmanship", ATTR_SPO);
	setRowTextAndHighlight("attribute:hidden:temperament", ATTR_TEM);
	setRowTextAndHighlight("attribute:hidden:versatility", ATTR_VER);

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
			gtk_widget_add_css_class(roleValue, "role-value");
			gtk_box_append(GTK_BOX(labelContainer), roleValue);

			GtkWidget *progressBar = gtk_progress_bar_new();
			gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressBar), player->ratings[i].value / 100.0f);
			gtk_widget_add_css_class(progressBar, "role-bar");

			gtk_box_append(GTK_BOX(parent), labelContainer);
			gtk_box_append(GTK_BOX(parent), progressBar);

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

	runMultiThreadedCache();
}

typedef struct {
	GtkWidget *box;
	GtkWidget *label;
	GtkWidget *closeButton;
} FilterTag;

static FilterTag createFilterTag(const char *text) {
	GtkWidget *label = gtk_label_new(text);
	gtk_widget_add_css_class(label, "chip-text");

	GtkWidget *close = gtk_label_new("✕");
	gtk_widget_add_css_class(close, "chip-x");
	GtkWidget *closeButton = gtk_button_new();
	gtk_widget_add_css_class(closeButton, "chip-button");
	gtk_widget_set_parent(close, closeButton);

	GtkWidget *box = gtk_box_new(0, 4);
	gtk_widget_add_css_class(box, "chip");
	gtk_box_append(GTK_BOX(box), label);
	gtk_box_append(GTK_BOX(box), closeButton);

	GtkBox *parent = GTK_BOX(gtk_builder_get_object(gameContext.builder, "box:filter-tags"));
	gtk_box_append(parent, box);

	return (FilterTag){.box = box, .label = label, .closeButton = closeButton};
}

void ui_createFilterTag(const char *text, GtkEntryBuffer *buffer) {
	const FilterTag tag = createFilterTag(text);
	g_signal_connect(tag.closeButton, "clicked", G_CALLBACK(onFilterTagClick), buffer);
}

void ui_createClubFilterTag(const char *text, GtkEditable *buffer) {
	const FilterTag tag = createFilterTag(text);
	g_signal_connect(tag.closeButton, "clicked", G_CALLBACK(onClubFilterTagClick), buffer);
	gtk_widget_set_name(tag.label, "tag:club-name");
}

void ui_clearFilterTags(void) {
	GtkBox *filterTags = GTK_BOX(gtk_builder_get_object(gameContext.builder, "box:filter-tags"));
	GtkWidget *child;
	while ((child = gtk_widget_get_first_child(GTK_WIDGET(filterTags))) != NULL) {
		gtk_box_remove(filterTags, GTK_WIDGET(child));
	}
}

static void onTagClick(GtkWidget *self) {
	GtkWidget *box = gtk_widget_get_parent(self);
	GtkWidget *parent = gtk_widget_get_parent(box);
	GtkWidget *closeLabel = gtk_widget_get_first_child(self);
	if (closeLabel != NULL) {
		gtk_widget_unparent(closeLabel);
	}

	gtk_box_remove(GTK_BOX(parent), box);

	searchHandler_cacheFilters();
	if (gameContext.filterOptions.filterMask) {
		searchHandler_doSearch(false);
	} else {
		playerTable_populate(NULL);
	}
}

void onClubFilterTagClick(GtkWidget *self, GtkEditable *buffer) {
	gtk_editable_set_text(buffer, "");
	onTagClick(self);
}

void onFilterTagClick(GtkWidget *self, GtkEntryBuffer *buffer) {
	gtk_entry_buffer_set_text(buffer, "", 1);
	onTagClick(self);
}


static void loadStylesheet(const char *fileName) {
	char pathToStylesheet[256] = {0};
	GtkCssProvider *provider = gtk_css_provider_new();

	snprintf(pathToStylesheet, sizeof(pathToStylesheet), "%s/%s", LAYOUTS_DIR, fileName);
	gtk_css_provider_load_from_path(provider, pathToStylesheet);
	gtk_style_context_add_provider_for_display(
		gdk_display_get_default(),
		GTK_STYLE_PROVIDER(provider),
		GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
	);
	g_object_unref(provider);
}
