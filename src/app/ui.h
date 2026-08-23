// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <gtk/gtk.h>

#include "app/types.h"


typedef struct {
	GtkBuilder *builder;
	GtkWidget *window;
} WindowContext;

void connectToProcess(void);
gboolean update(gpointer userData);
void updateUi(void);
void updateGameStatus(void);
void updateInGameDate(void);
void updateShowCurrentPlayerButton(void);
WindowContext openWindow(const char *layoutName, const char *windowName);

WindowContext createPlayerInfoWindow(const Player *player);
void renderPlayerInfoWindow(WindowContext context, const Player *player);
