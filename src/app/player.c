// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "player.h"
#include "app/constants.h"
#include "app/game-status.h"
#include "app/maths.h"
#include "app/mocks.h"
#include "core/logger.h"
#include "platform/platform.h"

#include <stdio.h>


extern GameContext gameContext;

static bool isPlayerAlsoStaff(void *handle, uint32_t playerAddress);
static bool isPersonAlsoStaff(void *handle, uint32_t playerAddress);
static bool isPlayerValid(void *handle, uint32_t personAddress);
static bool isPersonValid(void *handle, uint32_t personAddress);
static uint32_t getPlayerAddressFromPersonAddress(void *handle, uint32_t personAddress);
static PartialPlayer getPartialPlayer(void *handle, uint32_t personAddress);
static inline void getPersonName(void *handle, uint8_t pointer[4], char str[32]);
static inline void getPersonForename(void *handle, uint64_t attributeBase, char str[32]);
static inline void getPersonSurname(void *handle, uint64_t attributeBase, char str[32]);
static inline void getPersonCommonName(void *handle, uint64_t attributeBase, char str[64]);
static uint8_t getAge(void *handle, uint64_t address);
static int32_t getClubIndexFromPerson(void *handle, uint32_t personAddress);
static uint32_t getPersonAddressFromUid(const ProcessContext *processContext, uint32_t uid);
static void getSortedPositionRatings(Player *player);
static float getRatingPerPosition(const Player *player, PositionGrouped position);

// Assumes valid ProcessContext
uint32_t getCurrentPersonUniqueId(const ProcessContext *processContext) {
	#ifndef PLAYER_BY_ID
	uint8_t bytes[4];
	void *handle = processContext->handle;
	readFromMemory(handle, processContext->moduleBaseAddress + CURRENT_SCREEN_PLAYER_ID_PTR_BASE, 4, bytes);
	readFromMemory(handle, hexBytesToInt(bytes, 4) + CURRENT_SCREEN_PLAYER_ID_PTR_BASE_OFFSET_1, 4, bytes);
	readFromMemory(handle, hexBytesToInt(bytes, 4) + CURRENT_SCREEN_PLAYER_ID_PTR_BASE_OFFSET_2, 4, bytes);
	readFromMemory(handle, hexBytesToInt(bytes, 4) + CURRENT_SCREEN_PLAYER_ID_PTR_BASE_OFFSET_3, 4, bytes);
	readFromMemory(handle, hexBytesToInt(bytes, 4) + CURRENT_SCREEN_PLAYER_ID_PTR_BASE_OFFSET_4, 4, bytes);
	readFromMemory(handle, hexBytesToInt(bytes, 4) + CURRENT_SCREEN_PLAYER_ID_PTR_BASE_OFFSET_5, 4, bytes);
	readFromMemory(handle, hexBytesToInt(bytes, 4) + CURRENT_SCREEN_PLAYER_ID_PTR_BASE_OFFSET_6, 4, bytes);
	readFromMemory(handle, hexBytesToInt(bytes, 4) + CURRENT_SCREEN_PLAYER_ID_PTR_BASE_OFFSET_7, 4, bytes);
	readFromMemory(handle, hexBytesToInt(bytes, 4) + CURRENT_SCREEN_PLAYER_ID_PTR_BASE_OFFSET_8, 4, bytes);
	return (uint32_t)hexBytesToInt(bytes, 4);
	#else
	const Player player = PLAYER_BY_ID;
	return player.uid;
	#endif
}

// Assumes valid ProcessContext
Player getPlayerById(const ProcessContext *processContext, uint32_t uniqueId) {
	const uint32_t personAddress = getPersonAddressFromUid(processContext, uniqueId);
	if (personAddress && isPlayerValid(processContext->handle, personAddress)) {
		return getPlayer(processContext->handle, false, personAddress, 0);
	}

	LOG_ERROR("Could not find player by ID %d", uniqueId);

	return (Player){0};
}

// Assumes valid ProcessContext
PartialPlayer getPlayerByIdPartial(const ProcessContext *processContext, uint32_t uniqueId) {
	const uint32_t personAddress = getPersonAddressFromUid(processContext, uniqueId);
	if (personAddress && isPlayerValid(processContext->handle, personAddress)) {
		return getPartialPlayer(processContext->handle, personAddress);
	}

	LOG_ERROR("Could not find player by ID %d", uniqueId);

	return (PartialPlayer){0};
}

static PartialPlayer getPartialPlayer(void *handle, uint32_t personAddress) {
	#ifndef PLAYER_BY_ID
	const uint32_t playerAddress = personAddress + PLAYER_OFFSET_FROM_PERSON;

	uint8_t ability[3];
	readFromMemory(handle, playerAddress + PLAYER_OFFSET_ABILITY, 3, ability);

	uint8_t bytes[6];
	readFromMemory(handle, personAddress + PERSON_OFFSET_UNIQUE_ID, 4, bytes);
	const uint32_t uid = (uint32_t)hexBytesToInt(bytes, 4);

	PartialPlayer player = {
		.personAddress = personAddress,
		.uid = uid,
	};
	getPersonForename(handle, personAddress, player.forename);
	getPersonSurname(handle, personAddress, player.surname);
	getPersonCommonName(handle, personAddress, player.commonName);
	#else
	const Player p = PLAYER_BY_ID;
	PartialPlayer player = {
		.personAddress = personAddress,
		.uid = p.uid,
	};
	strncpy(player.forename, p.forename, sizeof(player.forename));
	strncpy(player.surname, p.surname, sizeof(player.surname));
	strncpy(player.commonName, p.commonName, sizeof(player.commonName));
	#endif

	if (player.commonName[0] == '\0') {
		snprintf(player.commonName, sizeof(player.commonName), "%s %s", player.forename, player.surname);
	}

	return player;
}

static uint32_t getPlayerAddressFromPersonAddress(void *handle, uint32_t personAddress) {
	const int32_t offset = isPersonAlsoStaff(handle, personAddress)
		? STAFF_OFFSET_FROM_PERSON
		: PLAYER_OFFSET_FROM_PERSON;

	return personAddress + offset;
}

uint32_t getPersonAddressFromPlayerAddress(void *handle, uint32_t playerAddress) {
	const int32_t offset = isPlayerAlsoStaff(handle, playerAddress)
		? STAFF_OFFSET_FROM_PERSON
		: PLAYER_OFFSET_FROM_PERSON;

	return playerAddress - offset;
}

Player getPlayer(void *handle, bool skipValidCheck, uint32_t personAddress, uint32_t playerAddress) {
	#ifndef PLAYER_BY_ID
	if (!skipValidCheck && !isPlayerValid(handle, personAddress)) {
		return (Player){0};
	}

	if (playerAddress == 0) {
		playerAddress = getPlayerAddressFromPersonAddress(handle, personAddress);
	}

	uint8_t ability[3];
	readFromMemory(handle, playerAddress + PLAYER_OFFSET_ABILITY, 3, ability);

	uint8_t bytes[6];
	readFromMemory(handle, personAddress + PERSON_OFFSET_ROW_ID, 4, bytes);
	const uint32_t rowId = (uint32_t)hexBytesToInt(bytes, 4);
	readFromMemory(handle, personAddress + PERSON_OFFSET_UNIQUE_ID, 4, bytes);
	const uint32_t uid = (uint32_t)hexBytesToInt(bytes, 4);
	readFromMemory(handle, personAddress + PERSON_OFFSET_RANDOM_ID, 4, bytes);
	const uint32_t rid = (uint32_t)hexBytesToInt(bytes, 4);

	Player player = {
		.personAddress = personAddress,
		.playerAddress = playerAddress,
		.age = getAge(handle, personAddress),
		.ca = ability[ABILITY_CA],
		.pa = ability[ABILITY_PA],
		.rid = rid,
		.uid = uid,
		.rowId = rowId,
	};
	getPersonForename(handle, personAddress, player.forename);
	getPersonSurname(handle, personAddress, player.surname);
	getPersonCommonName(handle, personAddress, player.commonName);

	if (player.commonName[0] == '\0') {
		snprintf(player.commonName, sizeof(player.commonName), "%s %s", player.forename, player.surname);
	}

	readFromMemory(handle, personAddress + PERSON_OFFSET_PERSONALITY, 8, player.personality);
	readFromMemory(handle, playerAddress + PLAYER_OFFSET_ATTRIBUTES, ATTRIBUTE_COUNT, player.attributes);
	readFromMemory(handle, playerAddress + PLAYER_OFFSET_POSITIONS, 15, player.positions);
	player.clubIndex = getClubIndexFromPerson(handle, personAddress);
	getSortedPositionRatings(&player);

	// Sharpness, fatigue, condition
	readFromMemory(handle, playerAddress + PLAYER_OFFSET_SHARPNESS, 6, bytes);
	player.sharpness = (uint16_t)hexBytesToInt(bytes, 2);
	player.fatigue = (int16_t)hexBytesToInt(bytes + 2, 2);
	player.condition = (uint16_t)hexBytesToInt(bytes + 4, 2);

	// Nationality
	readFromMemory(handle, personAddress + PERSON_OFFSET_NATIONALITY, 4, bytes);
	const uint32_t country = (uint32_t)hexBytesToInt(bytes, 4);
	readFromMemory(handle, country + NATION_OFFSET_ROW_ID, 4, bytes);
	player.nationIndex = (uint8_t)hexBytesToInt(bytes, 4);

	// Reputation
	readFromMemory(handle, playerAddress + PLAYER_OFFSET_HOME_REPUTATION, 6, bytes);
	player.homeReputation = (uint16_t)hexBytesToInt(bytes, 2);
	player.currentReputation = (uint16_t)hexBytesToInt(bytes + 2, 2);
	player.worldReputation = (uint16_t)hexBytesToInt(bytes + 4, 2);

	// Value
	readFromMemory(handle, playerAddress + PLAYER_OFFSET_GUIDE_VALUE, 4, bytes);
	player.guideValue = (uint32_t)hexBytesToInt(bytes, 4);

	// Extra
	player.canDevelopQuickly = player.age <= 23 &&
		player.attributes[ATTR_INJ] < 70 &&
		player.personality[PERSONALITY_AMBITION] > 10 &&
		player.personality[PERSONALITY_PROFESSIONALISM] > 10 &&
		player.attributes[ATTR_DET] > 50;
	if (player.age >= 15 && player.age <= 19) {
		player.isHotProspect = player.ca >= 80 + 5 * (player.age - 15);
	} else if (player.age <= 23) {
		player.isHotProspect = player.ca >= 140;
	} else {
		player.isHotProspect = false;
	}
	#else
	const Player player = PLAYER_BY_ID;
	#endif

	return player;
}

static bool isPlayerAlsoStaff(void *handle, uint32_t playerAddress) {
	uint8_t bytes[8];
	readFromMemory(handle, playerAddress - PLAYER_OFFSET_FROM_PERSON + 0x0C, 8, bytes);
	const uint64_t ids = hexBytesToInt(bytes, 8);
	return ids >> 32 != (ids & 0xFFFFFFFF);
}

static bool isPersonAlsoStaff(void *handle, uint32_t personAddress) {
	uint8_t bytes[4];
	readFromMemory(handle, personAddress + PLAYER_OFFSET_FROM_PERSON + 0x08, 4, bytes);
	return hexBytesToInt(bytes, 4) == 0;
}

static bool isPlayerValid(void *handle, uint32_t personAddress) {
	#ifndef MOCKS_MODE
	const uint32_t playerAddress = personAddress + PLAYER_OFFSET_FROM_PERSON;
	for (uint8_t i = 0; i < 5; ++i) {
		const uint8_t attribute = readByte(handle, playerAddress + PLAYER_OFFSET_HIDDEN_ATTRIBUTES + i);
		if (!attribute || attribute > 100) {
			return false;
		}
	}

	return isPersonValid(handle, personAddress);
	#else
	return true;
	#endif
}

static bool isPersonValid(void *handle, uint32_t personAddress) {
	#ifndef MOCKS_MODE
	for (uint8_t i = 0; i < 8; ++i) {
		const uint8_t attribute = readByte(handle, personAddress + PERSON_OFFSET_PERSONALITY + i);
		if (!attribute || attribute > 20) {
			return false;
		}
	}
	#endif

	return true;
}

static inline void getPersonName(void *handle, uint8_t pointer[4], char str[32]) {
	uint32_t a = (uint32_t)hexBytesToInt(pointer, 4);
	if (!a) {
		str[0] = '\0';
		return;
	}
	readFromMemory(handle, a, 4, pointer);
	a = (uint32_t)hexBytesToInt(pointer, 4);
	readFromMemory(handle, a + 4, 32, (uint8_t*)str);
}

static inline void getPersonForename(void *handle, uint64_t attributeBase, char str[32]) {
	uint8_t pointer[4];
	readFromMemory(handle, attributeBase + PERSON_OFFSET_FORENAME, 4, pointer);
	getPersonName(handle, pointer, str);
}

static inline void getPersonSurname(void *handle, uint64_t attributeBase, char str[32]) {
	uint8_t pointer[4];
	readFromMemory(handle, attributeBase + PERSON_OFFSET_SURNAME, 4, pointer);
	getPersonName(handle, pointer, str);
}

static inline void getPersonCommonName(void *handle, uint64_t attributeBase, char str[64]) {
	uint8_t pointer[4];
	readFromMemory(handle, attributeBase + PERSON_OFFSET_COMMON_NAME, 4, pointer);
	getPersonName(handle, pointer, str);
}

static uint8_t getAge(void *handle, uint64_t address) {
	uint8_t bytes[4];
	readFromMemory(handle, address + PERSON_OFFSET_DOB, 4, bytes);
	const uint8_t yearBytes[2] = {bytes[2], bytes[3]};
	const uint16_t yearOfBirth = (uint16_t)hexBytesToInt(yearBytes, 2);
	const uint16_t dayOfBirth = (uint16_t)hexBytesToInt(bytes, 2);

	uint8_t age = (uint8_t)(gameContext.currentDate.year - yearOfBirth);
	if (gameContext.currentDate.day < dayOfBirth) {
		--age;
	}

	return age;
}

static int32_t getClubIndexFromPerson(void *handle, uint32_t personAddress) {
	uint8_t pointer[4];
	readFromMemory(handle, personAddress + PERSON_OFFSET_CONTRACTS, 4, pointer);
	const uint32_t contractAddress = (uint32_t)hexBytesToInt(pointer, 4);
	if (!contractAddress) {
		return -1;
	}

	readFromMemory(handle, contractAddress + CONTRACTS_OFFSET_TEAM, 4, pointer);
	const uint32_t teamAddress = (uint32_t)hexBytesToInt(pointer, 4);
	readFromMemory(handle, teamAddress + TEAM_OFFSET_CLUB, 4, pointer);
	const uint32_t clubAddress = (uint32_t)hexBytesToInt(pointer, 4);
	readFromMemory(handle, clubAddress + CLUB_OFFSET_ROW_ID, 4, pointer);
	return (int32_t)hexBytesToInt(pointer, 4);

	// readFromMemory(handle, teamsAddress + TEAM_OFFSET_COMPETITION, 4, pointer);
	// const uint32_t competitionAddress = (uint32_t)hexBytesToInt(pointer, 4);
	// readFromMemory(handle, competitionAddress + COMPETITION_OFFSET_LONG_NAME_ADDRESS, 4, pointer);
	// readFromMemory(
	// 	handle,
	// 	hexBytesToInt(pointer, 4) + GENERAL_OFFSET_NAME,
	// 	CLUB_NAME_LENGTH,
	// 	(uint8_t*)club.divisionName
	// );
}

/* Helper structure to represent sparse attribute weights */
typedef struct {
	int idx;
	float weight;
} AttrPair;

/* Personality weights are fixed length 8 */

static void fill_weights_for_position(
	PositionGrouped p,
	float out_attr[ATTRIBUTE_COUNT],
	float out_personality[8],
	float *out_weight
) {
	/* zero defaults */
	for (int i = 0; i < ATTRIBUTE_COUNT; ++i) out_attr[i] = 0.0f;
	for (int i = 0; i < 8; ++i) out_personality[i] = 0.0f;
	*out_weight = 1.0f;

	/* Define sparse attribute lists and personality for each position */
	switch (p) {
		case POSITION_GROUPED_GK: {
			static const AttrPair attrs[] = {
				{ATTR_PAS, 14},
				{ATTR_VIS, 5},
				{ATTR_HAN, 6},
				{ATTR_AER, 27},
				{ATTR_CMD, 36},
				{ATTR_COM, 13},
				{ATTR_KIC, -6},
				{ATTR_THR, 10},
				{ATTR_ANT, 4},
				{ATTR_POS, 5},
				{ATTR_REF, 100},
				{ATTR_FIR, 11},
				{ATTR_FLA, -2},
				{ATTR_TEA, 6},
				{ATTR_WOR, 9},
				{ATTR_ECC, 6},
				{ATTR_TRO, 5},
				{ATTR_TTP, 2},
				{ATTR_ACC, 37},
				{ATTR_STR, 7},
				{ATTR_STA, 9},
				{ATTR_PAC, 27},
				{ATTR_JUM, 8},
				{ATTR_BRA, 7},
				{ATTR_CON, 20},
				{ATTR_AGG, -6},
				{ATTR_AGI, 63},
				{ATTR_IMP, 17},
				{ATTR_CMP, 1},
				{ATTR_CNT, 14}
			};
			const float pers[8] = {0, 0, 0, 32, 0, 0, 0, 0};
			const float weight = 4.4638f;
			for (size_t i = 0; i < sizeof(attrs) / sizeof(attrs[0]); ++i) out_attr[attrs[i].idx] = attrs[i].weight;
			for (int i = 0; i < 8; ++i) out_personality[i] = pers[i];
			*out_weight = weight;
			break;
		}
		case POSITION_GROUPED_CB: {
			static const AttrPair attrs[] = {
				{ATTR_DRI, 49},
				{ATTR_FIN, 14},
				{ATTR_HEA, 7},
				{ATTR_LON, 16},
				{ATTR_OTB, 9},
				{ATTR_PAS, 10},
				{ATTR_ANT, 24},
				{ATTR_POS, 2},
				{ATTR_FIR, 13},
				{ATTR_WOR, 31},
				{ATTR_ACC, 98},
				{ATTR_STR, 29},
				{ATTR_STA, 20},
				{ATTR_PAC, 100},
				{ATTR_JUM, 50},
				{ATTR_DIR, -15},
				{ATTR_BAL, 31},
				{ATTR_CON, 20},
				{ATTR_AGG, 15},
				{ATTR_AGI, 9},
				{ATTR_IMP, 16},
				{ATTR_INJ, -27},
				{ATTR_VER, 3},
				{ATTR_NAT, 21},
				{ATTR_DET, 18},
				{ATTR_CMP, 12},
				{ATTR_CNT, 28}
			};
			const float pers[8] = {4, 10, 9, 34, 21, 0, 10, -1};
			const float weight = 5.84f;
			for (size_t i = 0; i < sizeof(attrs) / sizeof(attrs[0]); ++i) out_attr[attrs[i].idx] = attrs[i].weight;
			for (int i = 0; i < 8; ++i) out_personality[i] = pers[i];
			*out_weight = weight;
			break;
		}
		case POSITION_GROUPED_FB: {
			static const AttrPair attrs[] = {
				{ATTR_DRI, 44},
				{ATTR_FIN, 15},
				{ATTR_HEA, 7},
				{ATTR_LON, 13},
				{ATTR_OTB, 5},
				{ATTR_PAS, 6},
				{ATTR_ANT, 23},
				{ATTR_POS, 1},
				{ATTR_FIR, 9},
				{ATTR_WOR, 31},
				{ATTR_ACC, 98},
				{ATTR_STR, 26},
				{ATTR_STA, 17},
				{ATTR_PAC, 100},
				{ATTR_JUM, 31},
				{ATTR_DIR, -11},
				{ATTR_BAL, 30},
				{ATTR_CON, 17},
				{ATTR_AGG, 11},
				{ATTR_AGI, 8},
				{ATTR_IMP, 12},
				{ATTR_INJ, -28},
				{ATTR_VER, 4},
				{ATTR_NAT, 17},
				{ATTR_DET, 16},
				{ATTR_CMP, 9},
				{ATTR_CNT, 25}
			};
			const float pers[8] = {3, 6, 5, 24, 17, 0, 6, -1};
			const float weight = 5.17f;
			for (size_t i = 0; i < sizeof(attrs) / sizeof(attrs[0]); ++i) out_attr[attrs[i].idx] = attrs[i].weight;
			for (int i = 0; i < 8; ++i) out_personality[i] = pers[i];
			*out_weight = weight;
			break;
		}
		case POSITION_GROUPED_WB: {
			static const AttrPair attrs[] = {
				{ATTR_DRI, 44},
				{ATTR_FIN, 15},
				{ATTR_HEA, 7},
				{ATTR_LON, 13},
				{ATTR_OTB, 5},
				{ATTR_PAS, 6},
				{ATTR_ANT, 23},
				{ATTR_POS, 1},
				{ATTR_FIR, 9},
				{ATTR_WOR, 31},
				{ATTR_ACC, 98},
				{ATTR_STR, 26},
				{ATTR_STA, 16},
				{ATTR_PAC, 100},
				{ATTR_JUM, 31},
				{ATTR_DIR, -11},
				{ATTR_BAL, 30},
				{ATTR_CON, 17},
				{ATTR_AGG, 11},
				{ATTR_AGI, 8},
				{ATTR_IMP, 12},
				{ATTR_INJ, -28},
				{ATTR_NAT, 17},
				{ATTR_DET, 16},
				{ATTR_CMP, 9},
				{ATTR_CNT, 25}
			};
			const float pers[8] = {3, 6, 5, 24, 17, 0, 6, -1};
			const float weight = 5.17f;
			for (size_t i = 0; i < sizeof(attrs) / sizeof(attrs[0]); ++i) out_attr[attrs[i].idx] = attrs[i].weight;
			for (int i = 0; i < 8; ++i) out_personality[i] = pers[i];
			*out_weight = weight;
			break;
		}
		case POSITION_GROUPED_DM: {
			static const AttrPair attrs[] = {
				{ATTR_DRI, 25},
				{ATTR_FIN, 13},
				{ATTR_HEA, 4},
				{ATTR_LON, 10},
				{ATTR_OTB, 4},
				{ATTR_PAS, 5},
				{ATTR_ANT, 21},
				{ATTR_POS, 6},
				{ATTR_FIR, 7},
				{ATTR_WOR, 32},
				{ATTR_ACC, 100},
				{ATTR_STR, 20},
				{ATTR_STA, 17},
				{ATTR_PAC, 100},
				{ATTR_JUM, 32},
				{ATTR_DIR, -9},
				{ATTR_BAL, 29},
				{ATTR_CON, 14},
				{ATTR_AGG, 9},
				{ATTR_AGI, 7},
				{ATTR_IMP, 10},
				{ATTR_INJ, -26},
				{ATTR_VER, 3},
				{ATTR_NAT, 15},
				{ATTR_DET, 16},
				{ATTR_CMP, 7},
				{ATTR_CNT, 14}
			};
			const float pers[8] = {2, 3, 3, 22, 15, 0, 5, -1};
			const float weight = 5.17f;
			for (size_t i = 0; i < sizeof(attrs) / sizeof(attrs[0]); ++i) out_attr[attrs[i].idx] = attrs[i].weight;
			for (int i = 0; i < 8; ++i) out_personality[i] = pers[i];
			*out_weight = weight;
			break;
		}
		case POSITION_GROUPED_MC: {
			static const AttrPair attrs[] = {
				{ATTR_DRI, 33},
				{ATTR_FIN, 13},
				{ATTR_HEA, 4},
				{ATTR_LON, 10},
				{ATTR_OTB, 3},
				{ATTR_PAS, 4},
				{ATTR_ANT, 22},
				{ATTR_POS, 6},
				{ATTR_FIR, 7},
				{ATTR_WOR, 33},
				{ATTR_ACC, 100},
				{ATTR_STR, 26},
				{ATTR_STA, 16},
				{ATTR_PAC, 100},
				{ATTR_JUM, 32},
				{ATTR_DIR, -9},
				{ATTR_BAL, 28},
				{ATTR_CON, 14},
				{ATTR_AGG, 9},
				{ATTR_AGI, 6},
				{ATTR_IMP, 10},
				{ATTR_INJ, -26},
				{ATTR_VER, 3},
				{ATTR_NAT, 15},
				{ATTR_DET, 16},
				{ATTR_CMP, 7},
				{ATTR_CNT, 14}
			};
			const float pers[8] = {2, 4, 3, 22, 15, 0, 4, -1};
			const float weight = 5.17f;
			for (size_t i = 0; i < sizeof(attrs) / sizeof(attrs[0]); ++i) out_attr[attrs[i].idx] = attrs[i].weight;
			for (int i = 0; i < 8; ++i) out_personality[i] = pers[i];
			*out_weight = weight;
			break;
		}
		case POSITION_GROUPED_AM: {
			static const AttrPair attrs[] = {
				{ATTR_DRI, 32},
				{ATTR_FIN, 12},
				{ATTR_HEA, 4},
				{ATTR_LON, 10},
				{ATTR_OTB, 4},
				{ATTR_PAS, 6},
				{ATTR_ANT, 22},
				{ATTR_POS, 8},
				{ATTR_FIR, 8},
				{ATTR_WOR, 33},
				{ATTR_ACC, 100},
				{ATTR_STR, 27},
				{ATTR_STA, 16},
				{ATTR_PAC, 100},
				{ATTR_JUM, 32},
				{ATTR_DIR, -8},
				{ATTR_BAL, 29},
				{ATTR_CON, 14},
				{ATTR_AGG, 8},
				{ATTR_AGI, 7},
				{ATTR_IMP, 10},
				{ATTR_INJ, -26},
				{ATTR_VER, 3},
				{ATTR_NAT, 15},
				{ATTR_DET, 16},
				{ATTR_CMP, 7},
				{ATTR_CNT, 13}
			};
			const float pers[8] = {2, 4, 3, 22, 15, 0, 5, -1};
			const float weight = 5.17f;
			for (size_t i = 0; i < sizeof(attrs) / sizeof(attrs[0]); ++i) out_attr[attrs[i].idx] = attrs[i].weight;
			for (int i = 0; i < 8; ++i) out_personality[i] = pers[i];
			*out_weight = weight;
			break;
		}
		case POSITION_GROUPED_W: {
			static const AttrPair attrs[] = {
				{ATTR_DRI, 36},
				{ATTR_FIN, 20},
				{ATTR_HEA, 13},
				{ATTR_LON, 17},
				{ATTR_OTB, 10},
				{ATTR_PAS, 11},
				{ATTR_ANT, 28},
				{ATTR_POS, 5},
				{ATTR_FIR, 11},
				{ATTR_WOR, 30},
				{ATTR_ACC, 100},
				{ATTR_STR, 32},
				{ATTR_STA, 18},
				{ATTR_PAC, 99},
				{ATTR_JUM, 38},
				{ATTR_DIR, -17},
				{ATTR_BAL, 33},
				{ATTR_CON, 22},
				{ATTR_AGG, 17},
				{ATTR_AGI, 9},
				{ATTR_IMP, 18},
				{ATTR_INJ, -28},
				{ATTR_VER, 4},
				{ATTR_NAT, 23},
				{ATTR_DET, 15},
				{ATTR_CMP, 12},
				{ATTR_CNT, 19}
			};
			const float pers[8] = {5, 12, 11, 30, 23, 0, 13, -1};
			const float weight = 6.6f;
			for (size_t i = 0; i < sizeof(attrs) / sizeof(attrs[0]); ++i) out_attr[attrs[i].idx] = attrs[i].weight;
			for (int i = 0; i < 8; ++i) out_personality[i] = pers[i];
			*out_weight = weight;
			break;
		}
		default: {
			static const AttrPair attrs[] = {
				{ATTR_DRI, 26},
				{ATTR_FIN, 9},
				{ATTR_HEA, 6},
				{ATTR_LON, 13},
				{ATTR_OTB, 3},
				{ATTR_PAS, 8},
				{ATTR_ANT, 23},
				{ATTR_POS, 3},
				{ATTR_FIR, 8},
				{ATTR_WOR, 23},
				{ATTR_ACC, 97},
				{ATTR_STR, 20},
				{ATTR_STA, 14},
				{ATTR_PAC, 100},
				{ATTR_JUM, 27},
				{ATTR_DIR, -13},
				{ATTR_BAL, 31},
				{ATTR_CON, 18},
				{ATTR_AGG, 13},
				{ATTR_AGI, 6},
				{ATTR_IMP, 14},
				{ATTR_INJ, -27},
				{ATTR_VER, 3},
				{ATTR_NAT, 19},
				{ATTR_DET, 15},
				{ATTR_CMP, 8},
				{ATTR_CNT, 17}
			};
			const float pers[8] = {3, 8, 7, 26, 19, 0, 9, -1};
			const float weight = 5.79f;
			for (size_t i = 0; i < sizeof(attrs) / sizeof(attrs[0]); ++i) out_attr[attrs[i].idx] = attrs[i].weight;
			for (int i = 0; i < 8; ++i) out_personality[i] = pers[i];
			*out_weight = weight;
			break;
		}
	}
}

static inline int position_group_to_indices(PositionGrouped p, int out_indices[5]) {
	/* returns number of indices filled in out_indices (max 5) */
	int n = 0;
	switch (p) {
		case POSITION_GROUPED_GK: out_indices[n++] = 0;
			break;
		case POSITION_GROUPED_FB: out_indices[n++] = 2;
			out_indices[n++] = 4;
			break;
		case POSITION_GROUPED_CB: out_indices[n++] = 3;
			break;
		case POSITION_GROUPED_WB: out_indices[n++] = 13;
			out_indices[n++] = 14;
			break;
		case POSITION_GROUPED_DM: out_indices[n++] = 5;
			break;
		case POSITION_GROUPED_MC: out_indices[n++] = 7;
			break;
		case POSITION_GROUPED_W: out_indices[n++] = 6;
			out_indices[n++] = 8;
			out_indices[n++] = 9;
			out_indices[n++] = 11;
			break;
		case POSITION_GROUPED_AM: out_indices[n++] = 10;
			break;
		default: out_indices[n++] = 12;
			break;
	}
	return n;
}

static float getRatingPerPosition(const Player *player, PositionGrouped position) {
	float attr_weights[ATTRIBUTE_COUNT];
	float pers_weights[8];
	float totalWeight = 1.0f;
	fill_weights_for_position(position, attr_weights, pers_weights, &totalWeight);

	/* Check positional proficiency */
	int indices[5];
	const int count = position_group_to_indices(position, indices);
	bool canPlay = false;
	for (int i = 0; i < count; ++i) {
		const int idx = indices[i];
		if (player->positions[idx] >= MINIMUM_POSITIONAL_PROFICIENCY) {
			canPlay = true;
			break;
		}
	}
	if (!canPlay) {
		return 0.0f;
	}
	// LOG_INFO("%s can play	%d", player->forename, position);
	float rating = 0.0f;
	/* attribute contributions */
	for (int i = 0; i < ATTRIBUTE_COUNT; ++i) {
		if (attr_weights[i] != 0.f) {
			const float value = (float)player->attributes[i] / 100.f;
			rating += attr_weights[i] * value;
			// LOG_INFO("Attr %d: %f * %f = %f (new rating: %f)", i, value, attr_weights[i], attr_weights[i] * value, rating);
		}
	}
	/* personality contributions */
	for (int i = 0; i < 8; ++i) {
		const float value = (float)player->personality[i] / 20.f;
		rating += pers_weights[i] * value;
		// LOG_INFO("Pers %d: %f * %f = %f (new rating: %f)", i, value, pers_weights[i], pers_weights[i] * value, rating);
	}

	if (totalWeight != 0.f) {
		rating /= totalWeight;
	}
	// LOG_INFO("Final rating after dividing by %f: %f", totalWeight, rating);

	return rating;
}

static void getSortedPositionRatings(Player *player) {
	const char *labels[POSITION_GROUPED_COUNT] = {"GK", "FB", "CB", "WB", "DM", "MC", "W", "AM", "ST"};
	for (int i = 0; i < POSITION_GROUPED_COUNT; ++i) {
		player->ratings[i].value = getRatingPerPosition(player, (PositionGrouped)i);
		player->ratings[i].position[0] = labels[i][0];
		player->ratings[i].position[1] = labels[i][1] ? labels[i][1] : '\0';
	}

	/* simple selection sort descending */
	for (int i = 0; i < POSITION_GROUPED_COUNT; ++i) {
		int best = i;
		for (int j = i + 1; j < POSITION_GROUPED_COUNT; ++j) {
			if (player->ratings[j].value > player->ratings[best].value) best = j;
		}
		if (best != i) {
			const Rating tmp = player->ratings[i];
			player->ratings[i] = player->ratings[best];
			player->ratings[best] = tmp;
		}
	}
}

static uint32_t getPersonAddressFromUid(const ProcessContext *processContext, uint32_t uid) {
	#ifndef PLAYER_BY_ID
	// Iterate over all players to find one with the matching UID
	uint8_t bytes[4];
	readFromMemory(processContext->handle, processContext->moduleBaseAddress + PLAYER_COUNT_PTR_BASE, 4, bytes);
	const uint32_t playerCount = (uint32_t)hexBytesToInt(bytes, 4);
	if (playerCount < 1 || playerCount > 500000) {
		LOG_ERROR("Unexpected number of players in database: %d", playerCount);
		return 0;
	}

	readFromMemory(
		processContext->handle,
		processContext->moduleBaseAddress + PLAYER_LIST_PTR_BASE,
		4,
		bytes
	);
	const uint32_t allPlayers = (uint32_t)hexBytesToInt(bytes, 4);

	for (uint32_t i = 0; i < playerCount; ++i) {
		readFromMemory(processContext->handle, allPlayers + PLAYER_LIST_STRIDE * i, 4, bytes);
		const uint32_t playerAddress = (uint32_t)hexBytesToInt(bytes, 4);
		const uint32_t personAddress = playerAddress - PLAYER_OFFSET_FROM_PERSON;

		readFromMemory(processContext->handle, personAddress + PERSON_OFFSET_UNIQUE_ID, 4, bytes);
		const uint32_t foundPlayerId = (uint32_t)hexBytesToInt(bytes, 4);
		if (foundPlayerId == uid) {
			return personAddress;
		}
	}

	return 0;
	#else
	return 0x5E18008;
	#endif
}
