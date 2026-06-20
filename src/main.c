// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtk/gtk.h>

#include "app/player.h"
#include "core/logger.h"
#include "platform/platform.h"


static ProcessContext processContext = {0};

G_MODULE_EXPORT void callbackConnect(void);
G_MODULE_EXPORT void callbackShowCurrentPlayer(void);

static void activate(GtkApplication *app) {
	// TODO: do something about this path
	GtkBuilder *builder = gtk_builder_new_from_file("../layouts/hello-world.ui");

	GtkWidget *window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
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

G_MODULE_EXPORT void callbackConnect(void) {
	platform_openProcess(&processContext);
	if (processContext.handle == NULL) {
		LOG_ERROR("Failed to open process");
		return;
	}

	LOG_INFO(
		"Process opened with PID: %u (base module address of %p)",
		processContext.pid,
		(void*)processContext.moduleBaseAddress
	);
}

G_MODULE_EXPORT void callbackShowCurrentPlayer(void) {
	if (processContext.handle == NULL) {
		LOG_ERROR("Process handle is NULL, cannot read current player");
		return;
	}

	const uint32_t uniqueId = getPersonUniqueId(&processContext);
	LOG_INFO("Current Player Unique ID: %u", uniqueId);
}
