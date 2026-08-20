// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdint.h>

#include "app/constants.h"


#define bool _Bool
#define false 0
#define true 1

// Logger
typedef enum LogLevel {
	LogLevelFatal,
	LogLevelError,
	LogLevelWarn,
	LogLevelInfo,
	LogLevelCount
} LogLevel;

// Process
typedef struct {
	void *handle; // Platform-specific process handle
	uintptr_t moduleBaseAddress;
	uint32_t pid;
} ProcessContext;

// Application
typedef enum {
	POSITION_GROUPED_GK = 0,
	POSITION_GROUPED_FB,
	POSITION_GROUPED_CB,
	POSITION_GROUPED_WB,
	POSITION_GROUPED_DM,
	POSITION_GROUPED_MC,
	POSITION_GROUPED_W,
	POSITION_GROUPED_AM,
	POSITION_GROUPED_ST,
	POSITION_GROUPED_COUNT,
} PositionGrouped;

// Longest club name: Club de Fútbol Lobos de la Benemérita Universidad Autónoma de Puebla, 71 chars
// Longest club short name: Persatuan Sepakbola Indonesia Karawang, 38 chars
#define CLUB_NAME_LENGTH 40

typedef struct {
	char name[CLUB_NAME_LENGTH];
	uint64_t address;
	uint8_t teamType;
} Club;

// "Saint Vincent and the Grenadines" is the longest at 32 bytes
#define MAX_NATION_STRING_LENGTH 32

typedef struct {
	char name[MAX_NATION_STRING_LENGTH];
} Nation;

typedef struct {
	float value;
	char position[2];
} Rating;

// TODO: note I only tested players, not all people
// Longest person forename: Kenji Syed Rusydi Al-Asyraf, 27 chars
// Longest person surname: Conceição Benevenuto Malaquias, 32 chars
// Longest person common name: Yamanalage Lakshitha Jayathunga, 31 chars
#define PERSON_FORENAME_LENGTH 32
#define PERSON_SURNAME_LENGTH 32
#define PERSON_COMMON_NAME_LENGTH 32

typedef struct {
	uint8_t attributes[ATTRIBUTE_COUNT];
	Rating ratings[POSITION_GROUPED_COUNT];
	char forename[PERSON_FORENAME_LENGTH];
	char surname[PERSON_SURNAME_LENGTH];
	char commonName[PERSON_COMMON_NAME_LENGTH];
	uint32_t rowId;
	uint32_t rid;
	uint32_t uid;
	uint32_t personAddress;
	uint32_t playerAddress;
	uint32_t guideValue;
	uint32_t annualWage;
	int32_t clubIndex; // -1 means unemployed
	uint16_t sharpness;
	uint16_t fatigue;
	uint16_t condition;
	uint16_t homeReputation;
	uint16_t currentReputation;
	uint16_t worldReputation;
	uint8_t positions[15];
	uint8_t age;
	uint8_t ca;
	uint8_t pa;
	uint8_t personality[8];
	uint8_t nationIndex;
	bool canDevelopQuickly;
	bool isHotProspect;
} Player;

typedef struct {
	char forename[PERSON_FORENAME_LENGTH];
	char surname[PERSON_SURNAME_LENGTH];
	char commonName[PERSON_COMMON_NAME_LENGTH];
	uint32_t uid;
	uint32_t personAddress;
} PartialPlayer;

typedef struct {
	uint16_t days;
	uint16_t year;
} Date;

#define MONTH_NAME_LENGTH 12

typedef struct {
	char month[MONTH_NAME_LENGTH];
	uint16_t year;
	uint16_t day;
} DayMonthYear;

#define GAME_STATUS_STRING_BUFFER_SIZE 32

typedef struct {
	PartialPlayer currentlyViewedPlayer;
	char gameVersion[GAME_STATUS_STRING_BUFFER_SIZE];
	DayMonthYear currentDate;
	Club *clubs;
	Nation *nations;
	Player *players;
	uint64_t clubCount;
	uint64_t nationCount;
	uint64_t playerCount;
} GameContext;
