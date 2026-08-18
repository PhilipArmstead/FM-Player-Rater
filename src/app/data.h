// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/types.h"


void clearCaches(GameContext *gameContext);
void cacheClubs(ProcessContext processContext, GameContext *gameContext);
void cacheNations(ProcessContext processContext, GameContext *gameContext);
