# SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.20)
project(FMPlayerRater VERSION 0.0.1 LANGUAGES C)

# Default build type for single-config generators only.
if (NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif ()

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)

add_executable(fm_player_rater
	src/main.c
	src/app/player.c
	src/core/logger.c
	src/platform/linux.c
	src/platform/macos.c
	src/platform/windows.c
)

# Allow including headers using absolute paths relative to the project `src/` directory,
# e.g. #include "platform/platform.h"
target_include_directories(fm_player_rater PRIVATE
	${PROJECT_SOURCE_DIR}/src
)

# Keep binary name with hyphens, but use a CMake-safe target name.
set_target_properties(fm_player_rater PROPERTIES
	OUTPUT_NAME "FM-Player-Rater"
)

target_compile_features(fm_player_rater PRIVATE c_std_99)

target_compile_options(fm_player_rater PRIVATE
	$<$<C_COMPILER_ID:GNU,Clang,AppleClang>:
	-Wall
	-Wextra
	-Wpedantic
	-Wshadow
	-Wconversion
	-Wsign-conversion
	-Wstrict-prototypes
	-Wmissing-prototypes
	-Wno-missing-field-initializers
	>
	$<$<C_COMPILER_ID:MSVC>:
	/W4
	>
)

target_compile_definitions(fm_player_rater PRIVATE
	$<$<PLATFORM_ID:Windows>:ARCH_WIN>
	$<$<PLATFORM_ID:Darwin>:ARCH_MACOS>
	$<$<PLATFORM_ID:Linux>:ARCH_LINUX>
	$<$<CONFIG:Debug>:DEBUG>
)

target_link_libraries(fm_player_rater PRIVATE PkgConfig::GTK4)

add_custom_target(info
	COMMAND ${CMAKE_COMMAND} -E echo "Generator: ${CMAKE_GENERATOR}"
	COMMAND ${CMAKE_COMMAND} -E echo "Config: $<CONFIG>"
	COMMAND ${CMAKE_COMMAND} -E echo "Compiler: ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}"
	COMMAND ${CMAKE_COMMAND} -E echo "Target: $<TARGET_FILE:fm_player_rater>"
)

add_custom_target(run
	COMMAND $<TARGET_FILE:fm_player_rater>
	DEPENDS fm_player_rater
	WORKING_DIRECTORY $<TARGET_FILE_DIR:fm_player_rater>
	USES_TERMINAL
)