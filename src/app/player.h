// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/types.h"


Player getPlayer(void *handle, bool skipIsValidCheck, uint32_t personAddress, uint32_t playerAddress);
uint32_t getPersonAddressFromPlayerAddress(void *handle, uint32_t playerAddress);
uint32_t getCurrentPersonUniqueId(const ProcessContext *processContext);
Player getPlayerById(const ProcessContext *processContext, uint32_t uniqueId);
void getWeightsForPosition(
	PositionGrouped position,
	float outAttr[ATTRIBUTE_COUNT],
	float outPersonality[8],
	float *outWeight
);
