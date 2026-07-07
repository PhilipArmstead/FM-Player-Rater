// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <gtk/gtk.h>


void updateUi(void);
void updateGameStatus(void);
void updateInGameDate(void);
void updateShowCurrentPlayerButton(void);
GtkWidget *openWindow(const char *layoutName, const char *windowName);
