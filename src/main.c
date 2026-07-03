// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtk/gtk.h>

#include "app/callbacks.h"
#include "core/logger.h"


ProcessContext processContext = {0};
GameContext gameContext = {0};
GtkBuilder *builder;


static void activate(GtkApplication *app) {
	char pathToAppLayout[256] = {0};
	snprintf(pathToAppLayout, sizeof(pathToAppLayout), "%s/app.ui", REPO_ROOT_DIR);
	builder = gtk_builder_new_from_file(pathToAppLayout);

	GtkWidget *window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
	gtk_window_set_application(GTK_WINDOW(window), GTK_APPLICATION(app));

	// Periodic callbacks
	g_timeout_add(1000, update, NULL);
	update(NULL);

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

// TODO: print player name + age in UI
// TODO: print game date + version in UI
// TODO: print player attributes, ratings, positions, condition
// TODO: pop out player details in to new window? even for current player?
