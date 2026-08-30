// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/types.h"

#include <gtk/gtk.h>


#define SEARCH_TYPE_PLAYER_ROW (search_player_row_get_type())
G_DECLARE_FINAL_TYPE(SearchPlayerRow, search_player_row, SEARCH, PLAYER_ROW, GObject)

struct _SearchPlayerRow {
	GObject parent_instance;
	Player *player;
	uint8_t age;
	uint8_t ca;
	uint8_t pa;
	uint8_t diff;
	float rating;
};

void playerTable_init(void);
void playerTable_clear(const GtkColumnView *table);
void playerTable_populate(GtkColumnView *table, const uint32_t *playerIds);
