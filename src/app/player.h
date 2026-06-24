// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/types.h"


uint32_t getCurrentPersonUniqueId(const ProcessContext *processContext);
Player getPlayerById(const ProcessContext *processContext, uint32_t uniqueId);
