// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../helpers/vector-shared-pointer.h"
#include "app/ui.h"
#include "app/helpers/formatter.h"
#include "app/helpers/vector.h"


extern GameContext gameContext;

WindowContext ui_createBestElevenWindow(void) {
	const WindowContext context = openWindow("best-xi", "window:best-xi");
	gtk_window_set_default_size(GTK_WINDOW(context.window), 420, 900);

	SharedPointer *snapshot = gameContext.searchResults;
	sharedPointer_ref(snapshot);
	g_object_set_data_full(
		G_OBJECT(context.window),
		"best-xi:search-results",
		snapshot,
		(GDestroyNotify) sharedPointer_unref
	);

	return context;
}

void ui_renderBestElevenWindow(WindowContext context) {
	const SharedPointer *snapshot = g_object_get_data(G_OBJECT(context.window), "best-xi:search-results");

	GtkListBox *listBox = GTK_LIST_BOX(gtk_builder_get_object(context.builder, "list-box:best-xi"));
	gtk_list_box_remove_all(listBox);

	const uint32_t *playerIds = snapshot ? snapshot->data : NULL;
	const size_t playerCount = playerIds ? vector_length(playerIds) : 0;

	const uint16_t o = PLAYER_OFFSET_POSITION_GK;
	const BestElevenFormation defaultFormation = (BestElevenFormation){
		.name = "4-4-2",
		.positions = {
			PLAYER_OFFSET_POSITION_GK - o,
			PLAYER_OFFSET_POSITION_DL - o,
			PLAYER_OFFSET_POSITION_DC - o,
			PLAYER_OFFSET_POSITION_DC - o,
			PLAYER_OFFSET_POSITION_DR - o,
			PLAYER_OFFSET_POSITION_ML - o,
			PLAYER_OFFSET_POSITION_MC - o,
			PLAYER_OFFSET_POSITION_MC - o,
			PLAYER_OFFSET_POSITION_MR - o,
			PLAYER_OFFSET_POSITION_ST - o,
			PLAYER_OFFSET_POSITION_ST - o
		},
	};

	for (uint8_t i = 0; i < 11; ++i) {
		GtkWidget *widgetRow = gtk_list_box_row_new();
		GtkListBoxRow *row = GTK_LIST_BOX_ROW(widgetRow);
		gtk_list_box_row_set_activatable(row, false);
		gtk_list_box_row_set_selectable(row, false);
		gtk_list_box_append(listBox, widgetRow);

		GtkWidget *widgetGrid = gtk_grid_new();
		GtkGrid *grid = GTK_GRID(widgetGrid);
		gtk_grid_set_column_spacing(grid, 12);
		gtk_widget_set_margin_bottom(widgetGrid, 4);
		gtk_widget_set_margin_end(widgetGrid, 4);
		gtk_widget_set_margin_start(widgetGrid, 4);
		gtk_widget_set_margin_top(widgetGrid, 4);

		static const char *positionNames[15] = {
			"GK",
			"SW",
			"DL",
			"DC",
			"DR",
			"DM",
			"ML",
			"MC",
			"MR",
			"AML",
			"AMC",
			"AMR",
			"ST",
			"WBL",
			"WBR"
		};

		GtkWidget *widgetLabelPosition = gtk_label_new(positionNames[defaultFormation.positions[i]]);
		gtk_widget_add_css_class(widgetLabelPosition, "monospace");
		// TODO: replace with result of best-XI algorithm
		GtkWidget *widgetLabelPlayer = gtk_label_new(gameContext.players[0].commonName);
		gtk_widget_set_hexpand(widgetLabelPlayer, true);
		gtk_label_set_xalign(GTK_LABEL(widgetLabelPlayer), 0);
		// TODO: replace with rating of relevant position
		char ratingBuffer[8] = {0};
		snprintf(ratingBuffer, sizeof(ratingBuffer), "%.2f%%", gameContext.players[0].ratings[0].value);
		GtkWidget *widgetLabelRating = gtk_label_new(ratingBuffer);
		gtk_label_set_xalign(GTK_LABEL(widgetLabelRating), 1.f);
		gtk_widget_add_css_class(widgetLabelRating, "success");
		GtkWidget *widgetButton = gtk_button_new();
		GtkWidget *widgetCrossImage = gtk_image_new_from_icon_name("window-close-symbolic");
		gtk_button_set_child(GTK_BUTTON(widgetButton), widgetCrossImage);
		// TODO: connect to callback to remove player from best-XI

		gtk_grid_attach(grid, widgetLabelPosition, 1, i, 1, 1);
		gtk_grid_attach(grid, widgetLabelPlayer, 2, i, 1, 1);
		gtk_grid_attach(grid, widgetLabelRating, 3, i, 1, 1);
		gtk_grid_attach(grid, widgetButton, 4, i, 1, 1);
		gtk_list_box_append(listBox, widgetGrid);

		// TODO: show age/condition, if we're going to filter on them
	}
}
