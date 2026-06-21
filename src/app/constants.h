// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#define MINIMUM_POSITIONAL_PROFICIENCY 15
#define ATTRIBUTE_COUNT 54

// Relative to base module
#define POINTER_TO_CURRENT_SCREEN_PERSON_ID 0x06374410
#define POINTER_TO_CURRENT_SCREEN_PERSON_ID_OFFSET_1 0xE0
#define POINTER_TO_CURRENT_SCREEN_PERSON_ID_OFFSET_2 0x10

// Day is represented with 1 byte and a 1 bit in the following byte to represent values > 255
// Time is represented as 1 byte (even bits only) determining the number of 15 minute periods since 6AM
// (plus 1 because 0 is midnight)
// Year is 2 bytes

// e.g. Sunday 12th of May 2024 at 0800 is 85 12 E8 07
// Relative to base module
#define POINTER_TO_CURRENT_DATETIME 0x631d5bc
