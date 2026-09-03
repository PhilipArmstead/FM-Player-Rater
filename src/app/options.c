// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "options.h"
#include "app/types.h"
#include "core/logger.h"
#include "platform/platform.h"

#include <stdio.h>
#include <string.h>


#define OPTIONS_FILE_NAME "options.yaml"
#define OPTIONS_PATH_BUFFER_SIZE 4096
#define OPTIONS_LINE_BUFFER_SIZE 512

extern GameContext gameContext;

// Accumulates a single formation while parsing its block of lines.
typedef struct {
	Formation current;
	bool haveCurrent;
	bool positionsSeen;
	bool positionsValid;
	uint8_t positionCount;
} FormationParser;


static char *trimLeading(char *text) {
	while (*text == ' ' || *text == '\t') {
		++text;
	}
	return text;
}

static void trimTrailing(char *text) {
	size_t length = strlen(text);
	while (length > 0) {
		const char c = text[length - 1];
		if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
			break;
		}
		text[--length] = '\0';
	}
}

static bool equalsIgnoreCase(const char *a, const char *b) {
	while (*a && *b) {
		int ca = (unsigned char)*a;
		int cb = (unsigned char)*b;
		if (ca >= 'A' && ca <= 'Z') {
			ca += 'a' - 'A';
		}
		if (cb >= 'A' && cb <= 'Z') {
			cb += 'a' - 'A';
		}
		if (ca != cb) {
			return false;
		}
		++a;
		++b;
	}
	return *a == *b;
}

static bool parseBool(const char *value, bool *out) {
	if (
		equalsIgnoreCase(value, "true") ||
		equalsIgnoreCase(value, "yes") ||
		equalsIgnoreCase(value, "1")
	) {
		*out = true;
		return true;
	}

	if (
		equalsIgnoreCase(value, "false") ||
		equalsIgnoreCase(value, "no") ||
		equalsIgnoreCase(value, "0")
	) {
		*out = false;
		return true;
	}

	return false;
}

// Position codes are matched case-insensitively ("GK" and "gk" both work).
static bool positionCodeFromString(const char *token, PositionCode *out) {
	for (int i = 0; i < POSITION_CODE_COUNT; ++i) {
		if (equalsIgnoreCase(token, positionCodeNames[i])) {
			*out = (PositionCode)i;
			return true;
		}
	}
	return false;
}

// Copies a formation name, stripping a single pair of surrounding double quotes if present.
static void copyFormationName(char *destination, const char *value) {
	const size_t length = strlen(value);
	if (length >= 2 && value[0] == '"' && value[length - 1] == '"') {
		snprintf(destination, FORMATION_NAME_LENGTH, "%.*s", (int)(length - 2), value + 1);
	} else {
		snprintf(destination, FORMATION_NAME_LENGTH, "%s", value);
	}
}

// Validates the accumulated formation and, if valid, appends it to the options.
static void formationParser_finalize(FormationParser *parser) {
	if (!parser->haveCurrent) {
		return;
	}

	Options *options = &gameContext.options;
	if (parser->current.name[0] == '\0') {
		LOG_WARN("Skipping formation with no name");
	} else if (!parser->positionsSeen || !parser->positionsValid ||
		parser->positionCount != FORMATION_POSITION_COUNT) {
		LOG_WARN(
			"Skipping formation '%s': expected %d valid positions",
			parser->current.name,
			FORMATION_POSITION_COUNT
		);
	} else if (options->formationCount >= OPTIONS_MAX_FORMATIONS) {
		LOG_WARN(
			"Ignoring formation '%s': exceeded maximum of %d",
			parser->current.name,
			OPTIONS_MAX_FORMATIONS
		);
	} else {
		options->formations[options->formationCount++] = parser->current;
	}

	*parser = (FormationParser){0};
}

// Begins a new formation list item, finalising any previous one.
static void formationParser_begin(FormationParser *parser) {
	formationParser_finalize(parser);
	*parser = (FormationParser){0};
	parser->haveCurrent = true;
}

static void formationParser_positions(FormationParser *parser, char *value) {
	parser->positionsSeen = true;
	parser->positionsValid = true;
	parser->positionCount = 0;

	char *token = strtok(value, " \t");
	while (token) {
		PositionCode code = POSITION_CODE_GK;
		if (!positionCodeFromString(token, &code)) {
			LOG_WARN("Unknown position code '%s' in formation '%s'", token, parser->current.name);
			parser->positionsValid = false;
		} else if (parser->positionCount < FORMATION_POSITION_COUNT) {
			parser->current.positions[parser->positionCount] = code;
		}

		if (parser->positionCount < UINT8_MAX) {
			++parser->positionCount;
		}
		token = strtok(NULL, " \t");
	}
}

static void formationParser_keyValue(FormationParser *parser, const char *key, char *value) {
	if (!parser->haveCurrent) {
		LOG_WARN("Ignoring formation field '%s' outside a list item", key);
		return;
	}

	if (!strcmp(key, "name")) {
		copyFormationName(parser->current.name, value);
		return;
	}

	if (!strcmp(key, "positions")) {
		formationParser_positions(parser, value);
		return;
	}

	LOG_WARN("Unrecognised formation field '%s'", key);
}

// Handles one indented line inside a `formations:` block.
static void formationParser_line(FormationParser *parser, char *cursor) {
	if (cursor[0] == '-') {
		formationParser_begin(parser);
		char *rest = trimLeading(cursor + 1);
		if (*rest == '\0') {
			return;
		}

		char *separator = strchr(rest, ':');
		if (!separator) {
			LOG_WARN("Ignoring malformed formation entry '%s'", rest);
			return;
		}

		*separator = '\0';
		char *key = rest;
		char *value = trimLeading(separator + 1);
		trimTrailing(key);
		trimTrailing(value);
		formationParser_keyValue(parser, key, value);
		return;
	}

	char *separator = strchr(cursor, ':');
	if (!separator) {
		LOG_WARN("Ignoring malformed formation line '%s'", cursor);
		return;
	}

	*separator = '\0';
	char *key = cursor;
	char *value = trimLeading(separator + 1);
	trimTrailing(key);
	trimTrailing(value);
	formationParser_keyValue(parser, key, value);
}

static void applyOption(const char *key, const char *value) {
	if (!strcmp(key, "dark-mode")) {
		bool darkMode = false;
		if (parseBool(value, &darkMode)) {
			gameContext.options.darkMode = darkMode;
		} else {
			LOG_WARN("Invalid boolean value '%s' for option '%s'", value, key);
		}
		return;
	}

	LOG_WARN("Unrecognised option key '%s'", key);
}

static void setDefaults(void) {
	Options *options = &gameContext.options;
	memset(options, 0, sizeof(*options));
	options->darkMode = true;

	// TODO: use a vector for this, lose the formationCount property and the limit of 32
	static const struct {
		const char *name;
		PositionCode positions[FORMATION_POSITION_COUNT];
	} formations[] = {
		{
			"4-4-2",
			{
				POSITION_CODE_GK,
				POSITION_CODE_DL,
				POSITION_CODE_DC,
				POSITION_CODE_DC,
				POSITION_CODE_DR,
				POSITION_CODE_ML,
				POSITION_CODE_MC,
				POSITION_CODE_MC,
				POSITION_CODE_MR,
				POSITION_CODE_ST,
				POSITION_CODE_ST,
			},
		},
		{
			"4-3-3",
			{
				POSITION_CODE_GK,
				POSITION_CODE_DL,
				POSITION_CODE_DC,
				POSITION_CODE_DC,
				POSITION_CODE_DR,
				POSITION_CODE_MC,
				POSITION_CODE_MC,
				POSITION_CODE_MC,
				POSITION_CODE_AML,
				POSITION_CODE_AMR,
				POSITION_CODE_ST,
			},
		},
		{
			"4-2-3-1",
			{
				POSITION_CODE_GK,
				POSITION_CODE_DL,
				POSITION_CODE_DC,
				POSITION_CODE_DC,
				POSITION_CODE_DR,
				POSITION_CODE_DM,
				POSITION_CODE_DM,
				POSITION_CODE_AML,
				POSITION_CODE_AMC,
				POSITION_CODE_AMR,
				POSITION_CODE_ST,
			},
		},
		{
			"4-2-4 IF",
			{
				POSITION_CODE_GK,
				POSITION_CODE_DL,
				POSITION_CODE_DC,
				POSITION_CODE_DC,
				POSITION_CODE_DR,
				POSITION_CODE_DM,
				POSITION_CODE_DM,
				POSITION_CODE_AML,
				POSITION_CODE_AMR,
				POSITION_CODE_ST,
				POSITION_CODE_ST,
			},
		},
	};

	options->formationCount = (uint8_t)(sizeof(formations) / sizeof(formations[0]));
	for (uint8_t i = 0; i < options->formationCount; ++i) {
		snprintf(options->formations[i].name, FORMATION_NAME_LENGTH, "%s", formations[i].name);
		memcpy(options->formations[i].positions, formations[i].positions, sizeof(formations[i].positions));
	}

	// TODO: use a vector for this
	static PositionWeights weights[] = {
		{.role = POSITION_GROUPED_GK, .weights = {0}},
		{.role = POSITION_GROUPED_COUNT, .weights = {0}},
	};
	// Ref: https://fm-arena.com/find-comment/53835/
	weights[0].weights[ATTR_DET] = 20;
	weights[0].weights[ATTR_CON] = 18.53f;
	weights[0].weights[ATTR_REF] = 19.35f;
	weights[0].weights[ATTR_AER] = 6.45f;
	weights[0].weights[ATTR_AGI] = 7.35f;
	weights[0].weights[ATTR_PAC] = 6.37f;
	weights[0].weights[ATTR_ACC] = 3.38f;
	weights[0].weights[ATTR_INJ] = -11.03f;
	weights[0].weights[ATTR_DIR] = -3.68f;
	weights[0].weights[ATTR_FIR] = 3.83f;
	weights[0].weights[ATTR_WOR] = 8.7f;
	weights[0].weights[ATTR_JUM] = 3.22f;
	weights[0].weights[ATTR_STA] = 7.35f;
	weights[0].weights[ATTR_TEC] = 3.53f;
	weights[0].weights[ATTR_FLA] = 11.03f;
	weights[0].weights[ATTR_COM] = 3.53f;
	weights[0].weights[ATTR_PRE] = 3.38f;
	weights[0].weights[ATTR_PRO] = 2.2f;
	weights[0].weights[ATTR_NAT] = 2.35f;
	weights[0].scale = 1;

	weights[1].weights[ATTR_PAC] = 20;
	weights[1].weights[ATTR_ACC] = 19.26f;
	weights[1].weights[ATTR_JUM] = 9.15f;
	weights[1].weights[ATTR_DRI] = 3.04f;
	weights[1].weights[ATTR_BAL] = 3.03f;
	weights[1].weights[ATTR_CON] = 9.57f;
	weights[1].weights[ATTR_ANT] = 8.51f;
	weights[1].weights[ATTR_DET] = 9.25f;
	weights[1].weights[ATTR_AGI] = 3.40f;
	weights[1].weights[ATTR_STA] = 10.64f;
	weights[1].weights[ATTR_DIR] = 6.8f;
	weights[1].weights[ATTR_CMP] = 5.53f;
	weights[1].weights[ATTR_WOR] = 14.15f;
	weights[1].weights[ATTR_PRE] = 3.62f;
	weights[1].weights[ATTR_INJ] = -4.8f;
	weights[1].scale = 1.05f;
	memcpy(options->weights, weights, sizeof(weights));
}

// Writes the current in-memory defaults to a new options file so the user has one to edit.
// The output is intentionally in the same form the parser reads back, so it round-trips.
static void writeDefaultOptions(const char *path) {
	FILE *file = fopen(path, "w");
	if (!file) {
		LOG_WARN("Could not create default options file '%s'; continuing with in-memory defaults", path);
		return;
	}

	const Options *options = &gameContext.options;

	fputs("# FM Player Rater options\n", file);
	fprintf(file, "dark-mode: %s\n", options->darkMode ? "true" : "false");

	fputs("formations:\n", file);
	for (uint8_t i = 0; i < options->formationCount; ++i) {
		fprintf(file, "  - name: \"%s\"\n", options->formations[i].name);
		fputs("    positions:", file);
		for (uint8_t j = 0; j < FORMATION_POSITION_COUNT; ++j) {
			fprintf(file, " %s", positionCodeNames[options->formations[i].positions[j]]);
		}
		fputc('\n', file);
	}

	fclose(file);

	LOG_INFO("Created default options file '%s'", path);
}

void options_init(void) {
	setDefaults();

	char directory[OPTIONS_PATH_BUFFER_SIZE];
	platform_getExecutableDirectory(directory, sizeof(directory));
	if (directory[0] == '\0') {
		LOG_WARN("Could not resolve executable directory; using default options");
		return;
	}

	char path[OPTIONS_PATH_BUFFER_SIZE];
	snprintf(path, sizeof(path), "%s%s", directory, OPTIONS_FILE_NAME);

	FILE *file = fopen(path, "r");
	if (!file) {
		LOG_INFO("Options file '%s' not found; writing defaults", path);
		writeDefaultOptions(path);
		return;
	}

	FormationParser parser = {0};
	bool inFormations = false;

	char line[OPTIONS_LINE_BUFFER_SIZE];
	while (fgets(line, sizeof(line), file)) {
		const bool indented = (line[0] == ' ' || line[0] == '\t');
		char *cursor = trimLeading(line);
		trimTrailing(cursor);

		// Skip blank lines and comments without ending an open formations block.
		if (*cursor == '\0' || *cursor == '#') {
			continue;
		}

		if (inFormations) {
			if (indented) {
				formationParser_line(&parser, cursor);
				continue;
			}

			// A non-indented line ends the block; fall through to handle it as a top-level key.
			formationParser_finalize(&parser);
			inFormations = false;
		}

		char *separator = strchr(cursor, ':');
		if (!separator) {
			LOG_WARN("Ignoring malformed options line '%s'", cursor);
			continue;
		}

		*separator = '\0';
		char *key = cursor;
		char *value = trimLeading(separator + 1);
		trimTrailing(key);
		trimTrailing(value);

		if (*key == '\0') {
			continue;
		}

		if (!strcmp(key, "formations")) {
			// The file is authoritative for formations when the key is present.
			gameContext.options.formationCount = 0;
			parser = (FormationParser){0};
			inFormations = true;
			if (*value != '\0') {
				LOG_WARN("Ignoring inline value for 'formations'");
			}
			continue;
		}

		applyOption(key, value);
	}

	// Finalise a formation still open at end of file.
	formationParser_finalize(&parser);

	fclose(file);

	LOG_DEBUG("Loaded options from '%s'", path);
}
