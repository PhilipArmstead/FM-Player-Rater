// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "player.h"
#include "app/constants.h"
#include "app/maths.h"
#include "platform/platform.h"


// Assumes valid ProcessContext
uint32_t getPersonUniqueId(const ProcessContext *processContext) {
	uint8_t bytes[4];
	void *handle = processContext->handle;
	readFromMemory(handle, processContext->moduleBaseAddress + CURRENT_SCREEN_PERSON_ID_PTR_BASE, 4, bytes);
	readFromMemory(handle, hexBytesToInt(bytes, 4) + CURRENT_SCREEN_PERSON_ID_PTR_BASE_OFFSET_1, 4, bytes);
	readFromMemory(handle, hexBytesToInt(bytes, 4) + CURRENT_SCREEN_PERSON_ID_PTR_BASE_OFFSET_2, 4, bytes);
	return (uint32_t)hexBytesToInt(bytes, 4);
}
