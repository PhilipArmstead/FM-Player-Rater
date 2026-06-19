// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtk/gtk.h>


G_MODULE_EXPORT void print_hello(void);

static void activate(GtkApplication *app) {
	// TODO: do something about this path
	GtkBuilder *builder = gtk_builder_new_from_file("../src/layouts/hello-world.ui");

	GtkWidget *window = GTK_WIDGET(gtk_builder_get_object (builder, "window"));
	gtk_window_set_application(GTK_WINDOW(window), GTK_APPLICATION(app));
	g_object_unref(builder);
	gtk_window_present(GTK_WINDOW(window));
}

int main(const int argc, char **argv) {
	GtkApplication *app = gtk_application_new("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	const int status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);

	return status;
}

G_MODULE_EXPORT void print_hello(void) {
	g_print("Hello World\n");
}
