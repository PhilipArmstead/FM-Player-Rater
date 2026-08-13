// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/types.h"


Player getPlayer(void *handle, bool skipValidCheck, uint32_t personAddress, uint32_t playerAddress);
uint32_t getPersonAddressFromPlayerAddress(void *handle, uint32_t playerAddress);
uint32_t getCurrentPersonUniqueId(const ProcessContext *processContext);
Player getPlayer(void *handle, uint32_t personAddress);
Player getPlayerById(const ProcessContext *processContext, uint32_t uniqueId);
PartialPlayer getPlayerByIdPartial(const ProcessContext *processContext, uint32_t uniqueId);
void getWeightsForPosition(
	PositionGrouped position,
	float out_attr[ATTRIBUTE_COUNT],
	float out_personality[8],
	float *out_weight
);
