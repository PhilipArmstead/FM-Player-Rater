// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "player-table.h"
#include "app/ui.h"
#include "app/helpers/formatter.h"
#include "app/helpers/vector.h"

#include <glib.h>

extern GameContext gameContext;

G_DEFINE_TYPE(SearchPlayerRow, search_player_row, G_TYPE_OBJECT)

static void search_player_row_finalize(GObject *object) {
	G_OBJECT_CLASS(search_player_row_parent_class)->finalize(object);
}

enum {
	PROP_0,
	PROP_PLAYER,
	N_PROPS
};

typedef enum {
	COLUMN_NATIONALITIES,
	COLUMN_NAME,
	COLUMN_AGE,
	COLUMN_POSITIONS,
	COLUMN_CA,
	COLUMN_PA,
	COLUMN_CA_PA_DELTA,
	COLUMN_STATUS,
	COLUMN_RATING,
	COLUMN_VALUE,
	COLUMN_CLUB,
	COLUMN_REPUTATION_HOME,
	COLUMN_REPUTATION_CURRENT,
	COLUMN_REPUTATION_WORLD,
} Columns;

static GParamSpec *properties[N_PROPS];

static void search_player_row_set_property(GObject *object, guint propertyId, const GValue *value, GParamSpec *pspec) {
	if (propertyId == PROP_PLAYER) {
		SearchPlayerRow *self = SEARCH_PLAYER_ROW(object);
		self->player = (Player*)g_value_get_pointer(value);
	} else {
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propertyId, pspec);
	}
}

static void search_player_row_get_property(GObject *object, guint propertyId, GValue *value, GParamSpec *pspec) {
	if (propertyId == PROP_PLAYER) {
		SearchPlayerRow *self = SEARCH_PLAYER_ROW(object);
		g_value_set_pointer(value, self->player);
	} else {
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propertyId, pspec);
	}
}

static void search_player_row_init(SearchPlayerRow *self) {
	// Don't do anything here — let properties handle initialization
	(void)self;
}

static void search_player_row_class_init(SearchPlayerRowClass *klass) {
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->set_property = search_player_row_set_property;
	object_class->get_property = search_player_row_get_property;
	object_class->finalize = search_player_row_finalize;

	properties[PROP_PLAYER] = g_param_spec_pointer(
		"player",
		"Player",
		"Player data",
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY
	);

	g_object_class_install_properties(object_class, N_PROPS, properties);
}

static void onRowClicked(GtkGestureClick *gesture, uint8_t clickCount, double x, double y, gpointer data) {
	(void)gesture;
	(void)x;
	(void)y;

	GtkListItem *listItem = GTK_LIST_ITEM(data);
	const SearchPlayerRow *row = gtk_list_item_get_item(listItem);

	if (row != NULL && row->player != NULL && clickCount == 2) {
		const WindowContext context = createPlayerInfoWindow();
		renderPlayerInfoWindow(context, row->player);
	}
}

static void bindClickHandler(GtkWidget *widget, GtkListItem *item) {
	GtkGestureClick *gesture = GTK_GESTURE_CLICK(gtk_gesture_click_new());
	gtk_widget_add_controller(widget, GTK_EVENT_CONTROLLER(gesture));
	g_signal_connect(gesture, "pressed", G_CALLBACK(onRowClicked), item);
}

static void setupTextLabel(GtkSignalListItemFactory *factory, GtkListItem *item, gpointer data) {
	(void)factory;
	(void)data;

	GtkWidget *label = gtk_label_new("");
	gtk_list_item_set_child(item, label);

	bindClickHandler(label, item);
}

static void setupBox(GtkSignalListItemFactory *factory, GtkListItem *item, gpointer data) {
	(void)factory;
	(void)data;

	GtkWidget *box = gtk_box_new(0, 2);
	gtk_list_item_set_child(item, box);

	bindClickHandler(box, item);
}

static void printNumeric(GtkWidget *label, int64_t value) {
	gchar buffer[32];
	g_snprintf(buffer, sizeof(buffer), "%llu", value);
	if (value > 999) {
		formatter_printNumber(buffer);
	}
	gtk_label_set_text(GTK_LABEL(label), buffer);
}

static void printPercentage(GtkWidget *label, float value) {
	char buffer[8];
	// getScaledHSL(getBestRatingForFilteredPositions(player))
	snprintf(buffer, sizeof(buffer), "%.2f%%", value);
	gtk_label_set_text(GTK_LABEL(label), buffer);
}

static void printCurrency(GtkWidget *label, uint64_t value) {
	gchar buffer[32];
	g_snprintf(buffer, sizeof(buffer), "%llu", value);
	formatter_printCurrency(buffer);
	gtk_label_set_xalign(GTK_LABEL(label), 0);
	gtk_label_set_text(GTK_LABEL(label), buffer);
}

static GString *formatPlayerPositions(const uint8_t *positions) {
	GString *str = g_string_new(NULL);
	bool hasPrevious = false;
	bool sameRow = false;

	// Goalkeeper
	if (positions[PLAYER_OFFSET_POSITION_GK - PLAYER_OFFSET_POSITION_GK] >= MINIMUM_POSITIONAL_PROFICIENCY) {
		g_string_append(str, "GK");
		hasPrevious = true;
	}

	// Fullback
	if (positions[PLAYER_OFFSET_POSITION_DL - PLAYER_OFFSET_POSITION_GK] >= MINIMUM_POSITIONAL_PROFICIENCY) {
		if (hasPrevious) {
			g_string_append(str, ", ");
		}
		hasPrevious = true;
		sameRow = true;
		g_string_append(str, "DL");
	}

	if (positions[PLAYER_OFFSET_POSITION_DR - PLAYER_OFFSET_POSITION_GK] >= MINIMUM_POSITIONAL_PROFICIENCY) {
		if (sameRow) {
			sameRow = false;
			g_string_append(str, "/R");
		} else if (hasPrevious) {
			g_string_append(str, ", ");
		} else {
			g_string_append(str, "DR");
		}
		hasPrevious = true;
	}

	// Centre back
	if (positions[PLAYER_OFFSET_POSITION_DC - PLAYER_OFFSET_POSITION_GK] >= MINIMUM_POSITIONAL_PROFICIENCY) {
		if (hasPrevious) {
			g_string_append(str, ", ");
		}
		g_string_append(str, "DC");
		hasPrevious = true;
	}


	// Wingback
	if (positions[PLAYER_OFFSET_POSITION_WBL - PLAYER_OFFSET_POSITION_GK] >= MINIMUM_POSITIONAL_PROFICIENCY) {
		if (hasPrevious) {
			g_string_append(str, ", ");
		}
		hasPrevious = true;
		sameRow = true;
		g_string_append(str, "WBL");
	}

	if (positions[PLAYER_OFFSET_POSITION_WBR - PLAYER_OFFSET_POSITION_GK] >= MINIMUM_POSITIONAL_PROFICIENCY) {
		if (sameRow) {
			sameRow = false;
			g_string_append(str, "/R");
		} else {
			if (hasPrevious) {
				g_string_append(str, ", ");
			}

			g_string_append(str, "WBR");
		}
		hasPrevious = true;
	}

	// Defensive midfielder
	if (positions[PLAYER_OFFSET_POSITION_DM - PLAYER_OFFSET_POSITION_GK] >= MINIMUM_POSITIONAL_PROFICIENCY) {
		if (hasPrevious) {
			g_string_append(str, ", ");
		}
		hasPrevious = true;
		g_string_append(str, "DM");
	}

	sameRow = false;

	// Winger
	if (positions[PLAYER_OFFSET_POSITION_ML - PLAYER_OFFSET_POSITION_GK] >= MINIMUM_POSITIONAL_PROFICIENCY) {
		if (hasPrevious) {
			g_string_append(str, ", ");
		}
		hasPrevious = true;
		sameRow = true;
		g_string_append(str, "ML");
	}

	if (positions[PLAYER_OFFSET_POSITION_MR - PLAYER_OFFSET_POSITION_GK] >= MINIMUM_POSITIONAL_PROFICIENCY) {
		if (sameRow) {
			g_string_append(str, "/R");
		} else {
			if (hasPrevious) {
				g_string_append(str, ", ");
			}

			g_string_append(str, "MR");
		}
		hasPrevious = true;
	}

	sameRow = false;

	// Central midfielder
	if (positions[PLAYER_OFFSET_POSITION_MC - PLAYER_OFFSET_POSITION_GK] >= MINIMUM_POSITIONAL_PROFICIENCY) {
		if (hasPrevious) {
			g_string_append(str, ", ");
		}
		hasPrevious = true;
		g_string_append(str, "MC");
	}

	// Advanced winger
	if (positions[PLAYER_OFFSET_POSITION_AML - PLAYER_OFFSET_POSITION_GK] >= MINIMUM_POSITIONAL_PROFICIENCY) {
		if (hasPrevious) {
			g_string_append(str, ", ");
		}
		hasPrevious = true;
		sameRow = true;
		g_string_append(str, "AML");
	}

	if (positions[PLAYER_OFFSET_POSITION_AMR - PLAYER_OFFSET_POSITION_GK] >= MINIMUM_POSITIONAL_PROFICIENCY) {
		if (sameRow) {
			g_string_append(str, "/R");
		} else {
			if (hasPrevious) {
				g_string_append(str, ", ");
			}

			g_string_append(str, "AMR");
		}
		hasPrevious = true;
	}

	sameRow = false;

	// Attacking midfielder
	if (positions[PLAYER_OFFSET_POSITION_AMC - PLAYER_OFFSET_POSITION_GK] >= MINIMUM_POSITIONAL_PROFICIENCY) {
		if (hasPrevious) {
			g_string_append(str, ", ");
		}
		hasPrevious = true;
		g_string_append(str, "AMC");
	}

	// Striker
	if (positions[PLAYER_OFFSET_POSITION_ST - PLAYER_OFFSET_POSITION_GK] >= MINIMUM_POSITIONAL_PROFICIENCY) {
		if (hasPrevious) {
			g_string_append(str, ", ");
		}
		hasPrevious = true;
		g_string_append(str, "ST");
	}

	return str;
}

static void bindCellValue(GtkSignalListItemFactory *factory, GtkListItem *item, uint8_t columnId) {
	(void)factory;

	GtkWidget *label = gtk_list_item_get_child(GTK_LIST_ITEM(item));
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	const SearchPlayerRow *row = gtk_list_item_get_item(GTK_LIST_ITEM(item));

	if (row == NULL || row->player == NULL) {
		return;
	}

	const Player *player = row->player;

	switch (columnId) {
		case COLUMN_NAME:
			if (player->commonName[0] == '\0') {
				char buffer[PERSON_FORENAME_LENGTH + PERSON_SURNAME_LENGTH + 2];
				snprintf(buffer, sizeof(buffer), "%s %s", player->forename, player->surname);
				gtk_label_set_text(GTK_LABEL(label), buffer);
			} else {
				gtk_label_set_text(GTK_LABEL(label), player->commonName);
			}
			gtk_label_set_xalign(GTK_LABEL(label), 0);
			break;
		case COLUMN_AGE:
			printNumeric(label, player->age);
			break;
		case COLUMN_POSITIONS:
			GString *str = formatPlayerPositions(row->player->positions);
			gtk_label_set_text(GTK_LABEL(label), str->str);
			gtk_label_set_xalign(GTK_LABEL(label), 0);
			g_string_free(str, TRUE);
			break;
		case COLUMN_CA:
			printNumeric(label, player->ca);
			break;
		case COLUMN_PA:
			printNumeric(label, player->pa);
			break;
		case COLUMN_CA_PA_DELTA:
			printNumeric(label, player->pa - player->ca);
			break;
		case COLUMN_RATING:
			printPercentage(label, player->ratings[0].value);
			break;
		case COLUMN_VALUE:
			printCurrency(label, player->guideValue);
			break;
		case COLUMN_CLUB:
			gtk_label_set_text(GTK_LABEL(label), gameContext.clubs[row->player->clubIndex].shortName);
			gtk_label_set_xalign(GTK_LABEL(label), 0);
			break;
		case COLUMN_REPUTATION_HOME:
			printNumeric(label, player->homeReputation);
			break;
		case COLUMN_REPUTATION_CURRENT:
			printNumeric(label, player->currentReputation);
			break;
		case COLUMN_REPUTATION_WORLD:
			printNumeric(label, player->worldReputation);
			break;
		default:
			break;
	}
}

static void bindStatusValue(GtkSignalListItemFactory *factory, GtkListItem *item, gpointer data) {
	(void)factory;
	(void)data;

	GtkWidget *box = gtk_list_item_get_child(GTK_LIST_ITEM(item));
	const SearchPlayerRow *row = gtk_list_item_get_item(GTK_LIST_ITEM(item));

	GtkWidget *child;
	while ((child = gtk_widget_get_first_child(GTK_WIDGET(box))) != NULL) {
		gtk_box_remove(GTK_BOX(box), child);
	}

	if (row == NULL || row->player == NULL) {
		return;
	}

	if (row->player->canDevelopQuickly) {
		GtkWidget *label = gtk_label_new("🧠");
		gtk_box_append(GTK_BOX(box), label);
		gtk_widget_set_tooltip_text(label, "Can develop quickly");
	}
	if (row->player->isHotProspect) {
		GtkWidget *label = gtk_label_new("🔥");
		gtk_box_append(GTK_BOX(box), label);
		gtk_widget_set_tooltip_text(label, "A hot prospect");
	}
}

static void bindNationalitiesValue(GtkSignalListItemFactory *factory, GtkListItem *item, gpointer data) {
	(void)factory;
	(void)data;

	GtkWidget *box = gtk_list_item_get_child(GTK_LIST_ITEM(item));
	const SearchPlayerRow *row = gtk_list_item_get_item(GTK_LIST_ITEM(item));

	GtkWidget *child;
	while ((child = gtk_widget_get_first_child(GTK_WIDGET(box))) != NULL) {
		gtk_box_remove(GTK_BOX(box), child);
	}

	if (row == NULL || row->player == NULL) {
		return;
	}

	uint8_t nationalityIndex = 0;
	while (nationalityIndex < 4 && row->player->nationality[nationalityIndex] != 0xFF) {
		char pathToFlag[256] = {0};
		const Nation nation = gameContext.nations[row->player->nationality[nationalityIndex]];
		snprintf(
			pathToFlag,
			sizeof(pathToFlag),
			"%s/assets/flags/%s.png",
			REPO_ROOT_DIR,
			nation.code
		);
		GtkWidget *flagImage = gtk_image_new_from_file(pathToFlag);
		gtk_box_append(GTK_BOX(box), flagImage);
		gtk_widget_set_tooltip_text(flagImage, nation.name);
		nationalityIndex++;
	}
}

static GtkColumnViewColumn *createColumn(const char *title, uint8_t columnType) {
	GtkListItemFactory *factory = gtk_signal_list_item_factory_new();
	if (columnType == COLUMN_NATIONALITIES) {
		g_signal_connect(factory, "setup", G_CALLBACK(setupBox), NULL);
		g_signal_connect(factory, "bind", G_CALLBACK(bindNationalitiesValue), NULL);
	} else if (columnType == COLUMN_STATUS) {
		g_signal_connect(factory, "setup", G_CALLBACK(setupBox), NULL);
		g_signal_connect(factory, "bind", G_CALLBACK(bindStatusValue), NULL);
	} else {
		g_signal_connect(factory, "setup", G_CALLBACK(setupTextLabel), NULL);
		g_signal_connect(factory, "bind", G_CALLBACK(bindCellValue), (void*)(uint64_t)columnType);
	}

	GtkColumnViewColumn *column = gtk_column_view_column_new(title, factory);
	return column;
}

void playerTable_init(void) {
	GtkColumnView *table = gameContext.table;
	gtk_column_view_append_column(table, createColumn("", COLUMN_NATIONALITIES));
	gtk_column_view_append_column(table, createColumn("Name", COLUMN_NAME));
	gtk_column_view_append_column(table, createColumn("Age", COLUMN_AGE));
	gtk_column_view_append_column(table, createColumn("Positions", COLUMN_POSITIONS));
	gtk_column_view_append_column(table, createColumn("CA", COLUMN_CA));
	gtk_column_view_append_column(table, createColumn("PA", COLUMN_PA));
	gtk_column_view_append_column(table, createColumn("Diff", COLUMN_CA_PA_DELTA));
	gtk_column_view_append_column(table, createColumn("Status", COLUMN_STATUS));
	gtk_column_view_append_column(table, createColumn("Rating", COLUMN_RATING));
	gtk_column_view_append_column(table, createColumn("Value", COLUMN_VALUE));
	gtk_column_view_append_column(table, createColumn("Club", COLUMN_CLUB));
	gtk_column_view_append_column(table, createColumn("Home rep", COLUMN_REPUTATION_HOME));
	gtk_column_view_append_column(table, createColumn("Current rep", COLUMN_REPUTATION_CURRENT));
	gtk_column_view_append_column(table, createColumn("World rep", COLUMN_REPUTATION_WORLD));

	playerTable_clear(table);
	playerTable_populate(table, NULL);
}

GListStore *store = NULL;

void playerTable_clear(const GtkColumnView *table) {
	(void)table;

	if (store != NULL) {
		g_list_store_remove_all(store);
	}

	store = g_list_store_new(SEARCH_TYPE_PLAYER_ROW);
}

void playerTable_populate(GtkColumnView *table, const uint32_t *playerIds) {
	playerTable_clear(table);

	const size_t playerCount = vector_length(playerIds);

	GtkWidget *clearAll = GTK_WIDGET(gtk_builder_get_object(gameContext.builder, "button:clear-all"));
	gtk_widget_set_visible(clearAll, playerCount > 0);

	for (size_t i = 0; i < playerCount; ++i) {
		const Player *player = &gameContext.players[playerIds[i]];
		SearchPlayerRow *row = g_object_new(
			SEARCH_TYPE_PLAYER_ROW,
			"player",
			player,
			NULL
		);
		g_list_store_append(store, row);
		g_object_unref(row);
	}

	GtkSingleSelection *selection = gtk_single_selection_new(G_LIST_MODEL(store));
	gtk_column_view_set_model(table, GTK_SELECTION_MODEL(selection));
	g_object_unref(selection);

	GtkLabel *resultsCountLabel = GTK_LABEL(
		GTK_WIDGET(gtk_builder_get_object(gameContext.builder, "label:results-count"))
	);
	char resultCountString[32] = {0};
	char formattedResults[12] = {0};
	snprintf(formattedResults, 12, "%llu", playerCount);
	formatter_printNumber(formattedResults);
	snprintf(resultCountString, 32, "%s result%c", formattedResults, playerCount != 1 ? 's' : '\0');
	gtk_label_set_text(resultsCountLabel, resultCountString);
}
