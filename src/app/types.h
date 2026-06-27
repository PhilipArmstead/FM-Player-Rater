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

typedef struct {
	char name[64];
	uint64_t address;
	uint8_t teamType;
} Club;

typedef struct {
	char name[28];
	char code[4];
} Nationality;

typedef struct {
	float value;
	char position[2];
} Rating;

typedef struct {
	uint8_t attributes[ATTRIBUTE_COUNT];
	Rating ratings[POSITION_GROUPED_COUNT];
	Club club;
	char commonName[64];
	char forename[32];
	char surname[32];
	Nationality nationality;
	uint32_t rid;
	uint32_t uid;
	uint32_t personAddress;
	uint32_t playerAddress;
	uint32_t guideValue;
	uint32_t annualWage;
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
	bool canDevelopQuickly;
	bool isHotProspect;
} Player;

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
	char gameVersion[GAME_STATUS_STRING_BUFFER_SIZE];
	DayMonthYear currentDate;
} GameContext;
