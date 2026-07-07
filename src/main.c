// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtk/gtk.h>

#include "app/callbacks.h"
#include "app/ui.h"
#include "core/logger.h"


ProcessContext processContext = {0};
GameContext gameContext = {0};
GtkBuilder *builder;


static void activate(GtkApplication *app) {
	char pathToStylesheet[256] = {0};
	GtkCssProvider *css_provider = gtk_css_provider_new();
	snprintf(pathToStylesheet, sizeof(pathToStylesheet), "%s/app.css", REPO_ROOT_DIR);
	gtk_css_provider_load_from_path(css_provider, pathToStylesheet);
	gtk_style_context_add_provider_for_display(
		gdk_display_get_default(),
		GTK_STYLE_PROVIDER(css_provider),
		GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
	);
	g_object_unref(css_provider);

	// Periodic callbacks
	g_timeout_add(1000, update, NULL);
	update(NULL);

	GtkWidget *window = openWindow("app", "window");
	gtk_window_set_application(GTK_WINDOW(window), GTK_APPLICATION(app));
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
// TODO: print player attributes, ratings, positions, condition
// TODO: pop out player details in to new window? even for current player?
