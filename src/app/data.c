// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "data.h"
#include "app/callbacks.h"
#include "app/config.h"
#include "app/maths.h"
#include "app/mocks.h"
#include "app/player.h"
#include "core/logger.h"
#include "platform/platform.h"

#include <string.h>
#include <stdlib.h>


extern ProcessContext processContext;
extern GameContext gameContext;

static thread_t threads[THREAD_COUNT];

#ifdef ARCH_WIN
static DWORD WINAPI threadFunction(const LPVOID arg) {
	const uint8_t func_num = (uint8_t)(intptr_t)arg;

	switch (func_num) {
		case 1: cacheClubs();
			break;
		case 2: cacheNations();
			break;
		case 3: cachePlayers(0);
			break;
		default: cachePlayers(1);
			break;
	}

	// Workers must not join each other; runMultiThreadedCache() owns the join.
	return 0;
}
#else
#include <pthread.h>


static void *threadFunction(void *arg) {
	const int func_num = (int)(intptr_t)arg;

	switch (func_num) {
		case 1: cacheClubs();
			break;
		case 2: cacheNations();
			break;
		case 3: cachePlayers(0);
			break;
		default: cachePlayers(1);
			break;
	}

	// Workers must not join each other; runMultiThreadedCache() owns the join.
	return NULL;
}
#endif

void clearCaches(void) {
	if (gameContext.clubs != NULL) {
		free(gameContext.clubs);
		gameContext.clubs = NULL;
		gameContext.clubCount = 0;
	}
	if (gameContext.nations != NULL) {
		free(gameContext.nations);
		gameContext.nations = NULL;
		gameContext.nationCount = 0;
	}
	if (gameContext.players != NULL) {
		free(gameContext.players);
		gameContext.players = NULL;
		gameContext.playerCount = 0;
	}
}


void cacheNations(void) {
	const int64_t timeStart = platform_getMicroseconds();

	#ifndef MOCKS_MODE
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
	gameContext.nationCount = nationCount;
	gameContext.nations = malloc(nationCount * sizeof(Nation));
	for (uint64_t i = 0; i < nationCount; i++) {
		uint8_t nationBuffer[4];
		readFromMemory(processContext.handle, nationStart + i * NATION_LIST_STRIDE, 4, nationBuffer);
		readFromMemory(processContext.handle, hexBytesToInt(nationBuffer, 4) + NATION_OFFSET_NAME, 4, bytes);
		readFromMemory(
			processContext.handle,
			hexBytesToInt(bytes, 4) + STRING_OFFSET_VALUE,
			MAX_NATION_STRING_LENGTH,
			(uint8_t*)gameContext.nations[i].name
		);
		readFromMemory(processContext.handle, hexBytesToInt(nationBuffer, 4) + NATION_OFFSET_NAME_CODE, 4, bytes);
		readFromMemory(
			processContext.handle,
			hexBytesToInt(bytes, 4) + STRING_OFFSET_VALUE,
			4,
			(uint8_t*)gameContext.nations[i].code
		);
	}
	#else
	const uint8_t nationCount = 251;
	gameContext.nationCount = nationCount;
	gameContext.nations = malloc(nationCount * sizeof(Nation));
	gameContext.nations[189] = PLAYER_BY_ID_NATION_1;
	gameContext.nations[170] = PLAYER_BY_ID_NATION_2;
	#endif

	const int64_t timeEnd = platform_getMicroseconds();
	LOG_INFO("Cached %d nations in %zu microseconds", nationCount, timeEnd - timeStart);
}

void cacheClubs(void) {
	const int64_t timeStart = platform_getMicroseconds();

	#ifndef MOCKS_MODE
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
	gameContext.clubCount = clubCount;
	gameContext.clubs = malloc(clubCount * sizeof(Club));
	uint64_t missed = 0;
	for (uint64_t i = 0; i < clubCount; i++) {
		uint8_t clubBuffer[4];
		readFromMemory(processContext.handle, clubStart + i * CLUB_LIST_STRIDE, 4, clubBuffer);
		readFromMemory(processContext.handle, hexBytesToInt(clubBuffer, 4) + CLUB_OFFSET_NAME, 4, bytes);
		uint32_t namePointer = (uint32_t)hexBytesToInt(bytes, 4);
		if (!namePointer || !readFromMemory(
			processContext.handle,
			hexBytesToInt(bytes, 4) + STRING_OFFSET_VALUE,
			CLUB_LONG_NAME_LENGTH,
			(uint8_t*)gameContext.clubs[i - missed].name
		)) {
			missed++;
			continue;
		}

		readFromMemory(processContext.handle, hexBytesToInt(clubBuffer, 4) + CLUB_OFFSET_NAME_SHORT, 4, bytes);
		namePointer = (uint32_t)hexBytesToInt(bytes, 4);
		readFromMemory(
			processContext.handle,
			hexBytesToInt(bytes, 4) + STRING_OFFSET_VALUE,
			CLUB_SHORT_NAME_LENGTH,
			(uint8_t*)gameContext.clubs[i - missed].shortName
		);
	}

	gameContext.clubCount -= missed;
	#else
	const uint32_t clubCount = 36289;
	gameContext.clubCount = clubCount;
	gameContext.clubs = malloc(clubCount * sizeof(Club));
	gameContext.clubs[1125] = PLAYER_BY_ID_CLUB;
	#endif

	const int64_t timeEnd = platform_getMicroseconds();
	LOG_INFO("Cached %d clubs in %zu microseconds", gameContext.clubCount, timeEnd - timeStart);
}

void cachePlayers(uint8_t half) {
	const int64_t timeStart = platform_getMicroseconds();
	uint64_t cached = 0;

	#ifndef MOCKS_MODE
	const uint64_t halfCount = (gameContext.playerCount - 1) / 2;
	const uint64_t start = half ? halfCount + 1 : 0;
	const uint64_t end = half ? gameContext.playerCount : halfCount + 1;

	uint8_t bytes[4];
	readFromMemory(processContext.handle, processContext.moduleBaseAddress + PLAYER_LIST_PTR_BASE, 4, bytes);
	const uint64_t playerStart = hexBytesToInt(bytes, 4);
	for (uint64_t i = start; i < end; i++) {
		readFromMemory(processContext.handle, playerStart + i * PLAYER_LIST_STRIDE, 4, bytes);
		const uint32_t playerAddress = (uint32_t)hexBytesToInt(bytes, 4);
		const uint32_t personAddress = getPersonAddressFromPlayerAddress(processContext.handle, playerAddress);
		const Player player = getPlayer(processContext.handle, true, personAddress, playerAddress);
		// Each worker owns a disjoint index range [start, end) and writes in
		// place. Invalid players are left zeroed (uid == 0) for compactPlayers()
		// to strip out afterwards, so the worker never touches shared counters
		// or another thread's slots.
		if (!player.uid) {
			continue;
		}
		gameContext.players[i] = player;
		++cached;
	}
	#else
	// Only one worker seeds the mock data so the two halves never race.
	if (half == 0) {
		gameContext.players[0] = PLAYER_VINI;
		gameContext.players[1] = PLAYER_JEFF;
		cached = 2;
	}
	#endif

	const int64_t timeEnd = platform_getMicroseconds();
	LOG_INFO(
		"Cached %llu players in %llu microseconds",
		(unsigned long long)cached,
		(unsigned long long)(timeEnd - timeStart)
	);
}

// Removes the invalid (zeroed) players the workers skipped, closing the gaps so
// gameContext.players is a dense [0, playerCount) range again. Must run on a
// single thread after every worker has been joined.
static void compactPlayers(void) {
	uint64_t write = 0;
	for (uint64_t read = 0; read < gameContext.playerCount; ++read) {
		if (gameContext.players[read].uid == 0) {
			continue;
		}
		if (write != read) {
			gameContext.players[write] = gameContext.players[read];
		}
		++write;
	}
	gameContext.playerCount = write;
}

void runMultiThreadedCache(void) {
	// Prepare player array for multithreaded writing
	#ifndef MOCKS_MODE
	uint8_t bytes[4];
	readFromMemory(processContext.handle, processContext.moduleBaseAddress + PLAYER_COUNT_PTR_BASE, 4, bytes);
	const uint64_t playerCount = hexBytesToInt(bytes, 4);
	#else
	const uint64_t playerCount = 900;
	#endif

	gameContext.playerCount = playerCount;
	// calloc so that slots the workers skip stay zeroed (uid == 0) and are
	// recognisable to compactPlayers().
	gameContext.players = calloc(playerCount, sizeof(Player));
	if (gameContext.players == NULL) {
		LOG_ERROR("Failed to allocate memory for %llu players", (unsigned long long)playerCount);
		gameContext.playerCount = 0;
		return;
	}

	#ifdef ARCH_WIN
	for (uint8_t i = 0; i < THREAD_COUNT; i++) {
		threads[i] = CreateThread(
			NULL,
			0,
			threadFunction,
			(LPVOID)(intptr_t)(i + 1),
			0,
			NULL
		);

		if (threads[i] == NULL) {
			LOG_ERROR("Failed to create thread %d", i + 1);
		}
	}

	// Wait for every worker to finish before reading/mutating shared caches.
	for (uint8_t i = 0; i < THREAD_COUNT; i++) {
		if (threads[i] != NULL) {
			WaitForSingleObject(threads[i], INFINITE);
			CloseHandle(threads[i]);
			threads[i] = NULL;
		}
	}
	#else
	bool created[THREAD_COUNT] = {false};
	for (uint8_t i = 0; i < THREAD_COUNT; i++) {
		if (pthread_create(
			&threads[i],
			NULL,
			threadFunction,
			(void*)(intptr_t)(i + 1)
		) == 0) {
			created[i] = true;
		} else {
			LOG_ERROR("Failed to create thread %d", i + 1);
		}
	}

	// Wait for every worker to finish before reading/mutating shared caches.
	for (uint8_t i = 0; i < THREAD_COUNT; i++) {
		if (created[i]) {
			pthread_join(threads[i], NULL);
		}
	}
	#endif

	// Safe now that all workers have been joined: single-threaded compaction of
	// the players written by the two player workers.
	compactPlayers();
}
