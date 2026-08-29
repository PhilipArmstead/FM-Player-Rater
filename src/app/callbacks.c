// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "callbacks.h"

#include "app/player-table.h"
#include "app/callbacks/club-search.h"
#include "app/callbacks/filters.h"
#include "core/logger.h"


extern GameContext gameContext;

void callbacks_init(void) {
	// Capture keypresses within sidebar to trigger search
	GtkEventControllerKey *controllerKey = GTK_EVENT_CONTROLLER_KEY(gtk_event_controller_key_new());
	g_signal_connect(controllerKey, "key-released", G_CALLBACK(callbacks_onFiltersKeypress), NULL);
	gtk_event_controller_set_propagation_phase(GTK_EVENT_CONTROLLER(controllerKey), GTK_PHASE_BUBBLE);

	GtkWidget *sidebar = GTK_WIDGET(gtk_builder_get_object(gameContext.builder, "sidebar"));
	gtk_widget_add_controller(sidebar, GTK_EVENT_CONTROLLER(controllerKey));

	// Attach datalist callbacks
	SearchDatalist *dataList = gameContext.dataList;
	g_signal_connect(dataList->entry, "changed", G_CALLBACK(callbacks_OnClubNameChange), dataList);
	g_signal_connect(dataList->listBox, "row-activated", G_CALLBACK(callbacks_onClubNameSelected), dataList);
}
