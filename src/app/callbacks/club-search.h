// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <gtk/gtk.h>

#include "app/types.h"


G_MODULE_EXPORT void callbacks_OnClubNameChange(GtkEditable *editable, SearchDatalist *dataList);
G_MODULE_EXPORT void callbacks_onClubNameSelected(
	GtkListBox *box,
	GtkListBoxRow *row,
	const SearchDatalist *dataList
);
