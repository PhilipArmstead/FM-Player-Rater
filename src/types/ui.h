// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

typedef struct {
	GtkEntryBuffer *minAge;
	GtkEntryBuffer *maxAge;
	GtkEntryBuffer *minCA;
	GtkEntryBuffer *maxCA;
	GtkEntryBuffer *minPA;
	GtkEntryBuffer *maxPA;
	GtkEntryBuffer *minRating;
	GtkEntryBuffer *maxRating;
	GtkEntryBuffer *clubSearch;
} FilterBuffer;

typedef struct {
	GtkSearchEntry *entry;
	GtkPopover *popover;
	GtkListBox *listBox;
} SearchDatalist;
