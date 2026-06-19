// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtk/gtk.h>

#include "core/logger.h"


G_MODULE_EXPORT void print_hello(void);

static void activate(GtkApplication *app) {
	// TODO: do something about this path
	GtkBuilder *builder = gtk_builder_new_from_file("../layouts/hello-world.ui");

	GtkWidget *window = GTK_WIDGET(gtk_builder_get_object (builder, "window"));
	gtk_window_set_application(GTK_WINDOW(window), GTK_APPLICATION(app));
	g_object_unref(builder);
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

G_MODULE_EXPORT void print_hello(void) {
	LOG_INFO("Hello World!", "");
	LOG_WARN("Hello World :\\", "");
	LOG_ERROR("Hello World :(", "");
	LOG_FATAL("Hello World :'(", "");
}
