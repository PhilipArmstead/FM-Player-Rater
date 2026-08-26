// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <gtk/gtk.h>

#include "app/types.h"


void callbacks_init();
G_MODULE_EXPORT void callbackOnClubNameChange(GtkEditable *editable, const SearchDatalist *dataList);
G_MODULE_EXPORT void callbackOnClubNameSelected(
	GtkListBox *box,
	GtkListBoxRow *row,
	const SearchDatalist *dataList
);
G_MODULE_EXPORT void callbackShowCurrentPlayer(void);
G_MODULE_EXPORT void callbackClearFilters(void);
G_MODULE_EXPORT gboolean callbackFilterKeypress(
	GtkEventControllerKey *controller,
	guint keyval,
	guint keycode,
	GdkModifierType state,
	gpointer data
);
