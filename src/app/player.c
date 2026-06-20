// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "player.h"
#include "app/constants.h"
#include "app/maths.h"
#include "platform/platform.h"


uint32_t getPersonUniqueId(const ProcessContext *processContext) {
	uint8_t bytes[4];
	void *handle = processContext->handle;
	readFromMemory(handle, processContext->moduleBaseAddress + POINTER_TO_CURRENT_SCREEN_PERSON_ID, 4, bytes);
	readFromMemory(handle, hexBytesToInt(bytes, 4) + POINTER_TO_CURRENT_SCREEN_PERSON_ID_OFFSET_1, 4, bytes);
	readFromMemory(handle, hexBytesToInt(bytes, 4) + POINTER_TO_CURRENT_SCREEN_PERSON_ID_OFFSET_2, 4, bytes);
	return (uint32_t)hexBytesToInt(bytes, 4);
}
