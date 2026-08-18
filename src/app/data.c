// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "data.h"
#include "maths.h"
#include "core/logger.h"
#include "platform/platform.h"


void clearCaches(GameContext *gameContext) {
	if (gameContext->nations != NULL) {
		free(gameContext->nations);
		gameContext->nations = NULL;
		gameContext->nationCount = 0;
	}
	if (gameContext->clubs != NULL) {
		free(gameContext->clubs);
		gameContext->clubs = NULL;
		gameContext->clubCount = 0;
	}
}


void cacheClubs(const ProcessContext processContext, GameContext *gameContext) {
	const int64_t timeStart = platform_getMicroseconds();

	uint8_t bytes[4];
	readFromMemory(processContext.handle, processContext.moduleBaseAddress + CLUB_LIST_PTR_BASE, 4, bytes);
	readFromMemory(processContext.handle, hexBytesToInt(bytes, 4) + CLUB_LIST_PTR_BASE_OFFSET, 4, bytes);

	uint8_t clubStartBuffer[4];
	uint8_t clubEndBuffer[4];
	readFromMemory(processContext.handle, hexBytesToInt(bytes, 4) + CLUB_LIST_START, 4, clubStartBuffer);
	readFromMemory(processContext.handle, hexBytesToInt(bytes, 4) + CLUB_LIST_END, 4, clubEndBuffer);
	const uint64_t clubStart = hexBytesToInt(clubStartBuffer, 4);
	const uint64_t clubEnd = hexBytesToInt(clubEndBuffer, 4);
	const uint64_t clubCount = (clubEnd - clubStart) / CLUB_LIST_STRIDE;
	gameContext->clubCount = clubCount;
	gameContext->clubs = malloc(clubCount * sizeof(Club));
	uint64_t missed = 0;
	for (uint64_t i = 0; i < clubCount; i++) {
		uint8_t clubBuffer[4];
		readFromMemory(processContext.handle, clubStart + i * CLUB_LIST_STRIDE, 4, clubBuffer);
		readFromMemory(processContext.handle, hexBytesToInt(clubBuffer, 4) + CLUB_OFFSET_NAME, 4, bytes);
		if (!readFromMemory(
			processContext.handle,
			hexBytesToInt(bytes, 4) + STRING_OFFSET_VALUE,
			64,
			(uint8_t*)gameContext->clubs[i - missed].name
		)) {
			missed++;
		}
	}

	gameContext->clubCount -= missed;

	const int64_t timeEnd = platform_getMicroseconds();
	LOG_INFO("Cached %d clubs in %llu microseconds", gameContext->clubCount, timeEnd - timeStart);
}


void cacheNations(const ProcessContext processContext, GameContext *gameContext) {
	const int64_t timeStart = platform_getMicroseconds();

	uint8_t bytes[4];
	readFromMemory(processContext.handle, processContext.moduleBaseAddress + NATION_LIST_PTR_BASE, 4, bytes);
	readFromMemory(processContext.handle, hexBytesToInt(bytes, 4) + NATION_LIST_PTR_BASE_OFFSET, 4, bytes);

	uint8_t nationStartBuffer[4];
	uint8_t nationEndBuffer[4];
	readFromMemory(processContext.handle, hexBytesToInt(bytes, 4) + NATION_LIST_START, 4, nationStartBuffer);
	readFromMemory(processContext.handle, hexBytesToInt(bytes, 4) + NATION_LIST_END, 4, nationEndBuffer);
	const uint64_t nationStart = hexBytesToInt(nationStartBuffer, 4);
	const uint64_t nationEnd = hexBytesToInt(nationEndBuffer, 4);
	const uint64_t nationCount = (nationEnd - nationStart) / NATION_LIST_STRIDE;
	gameContext->nationCount = nationCount;
	gameContext->nations = malloc(nationCount * sizeof(Nation));
	for (uint64_t i = 0; i < nationCount; i++) {
		uint8_t nationBuffer[4];
		readFromMemory(processContext.handle, nationStart + i * NATION_LIST_STRIDE, 4, nationBuffer);
		readFromMemory(processContext.handle, hexBytesToInt(nationBuffer, 4) + NATION_OFFSET_NAME, 4, bytes);
		readFromMemory(
			processContext.handle,
			hexBytesToInt(bytes, 4) + STRING_OFFSET_VALUE,
			MAX_NATION_STRING_LENGTH,
			(uint8_t*)gameContext->nations[i].name
		);
	}

	const int64_t timeEnd = platform_getMicroseconds();
	LOG_INFO("Cached %d nations in %llu microseconds", nationCount, timeEnd - timeStart);
}
