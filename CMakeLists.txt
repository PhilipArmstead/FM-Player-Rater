# SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
# SPDX-License-Identifier: GPL-3.0-or-later

# CMakeLists.txt for FM Player Rater (C99 + GTK4)
# Build modes: Release (default), Debug, RelWithDebInfo
# Usage:
#   mkdir build && cd build
#   cmake -DCMAKE_BUILD_TYPE=Release ..
#   cmake --build .

cmake_minimum_required(VERSION 3.20)
project(FM-Player-Rater C)

# =========================
# Project Configuration
# =========================
set(PROJECT_VERSION 0.1.0)
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}")

# =========================
# C Standard and Compiler
# =========================
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Use default compiler (don't override with clang on Windows)
# Let the environment's CC or CMake defaults handle compiler selection
if (DEFINED ENV{CC})
	set(CMAKE_C_COMPILER $ENV{CC})
endif ()

# =========================
# Build Type Configuration
# =========================
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif ()

set(CMAKE_CONFIGURATION_TYPES "Release;Debug;RelWithDebInfo" CACHE STRING "")

# =========================
# Platform Detection & Flags
# =========================
# Compiler flags
add_compile_options(
	-Wall
	-Wextra
	-Wpedantic
	-Wshadow
	-Wconversion
	-Wsign-conversion
	-Wstrict-prototypes
	-Wmissing-prototypes
	-Wno-pointer-sign
	-Wno-missing-field-initializers
	-Wno-c11-extensions
	-Wno-gnu-empty-struct
)

# 64-bit architecture flags (skip on MinGW/Clang which default to 64-bit)
if (NOT MSVC AND NOT MINGW)
	add_compile_options(-m64)
endif ()

# Platform-specific definitions
if (WIN32)
	add_compile_definitions(ARCH_WIN)
elseif (APPLE)
	add_compile_definitions(ARCH_MACOS)
elseif (UNIX AND NOT APPLE)
	add_compile_definitions(ARCH_LINUX)
endif ()

# Build-type specific flags
if (CMAKE_BUILD_TYPE STREQUAL "Release")
	add_compile_options(-O3)
	add_compile_definitions(NDEBUG)
elseif (CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_compile_options(-O0 -g3 -fno-omit-frame-pointer)
	add_compile_definitions(DEBUG)
elseif (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
	add_compile_options(-O2 -g)
	add_compile_definitions(NDEBUG)
endif ()

# =========================
# Dependencies
# =========================
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)

# =========================
# Source Files
# =========================
set(SOURCES
	src/main.c
)

# =========================
# Executable
# =========================
add_executable(FM-Player-Rater ${SOURCES})

# Link GTK4
target_link_libraries(FM-Player-Rater PRIVATE ${GTK4_LIBRARIES})
# Apply all GTK4 compiler flags (includes -I, -D, -mfpmath, etc.)
target_compile_options(FM-Player-Rater PRIVATE ${GTK4_CFLAGS})
# Also add include directories explicitly
target_include_directories(FM-Player-Rater PRIVATE ${GTK4_INCLUDE_DIRS})

# Set output directory
set_target_properties(FM-Player-Rater PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${EXECUTABLE_OUTPUT_PATH}/${CMAKE_BUILD_TYPE}"
)

# =========================
# Info Target (optional)
# =========================
add_custom_target(info
	COMMAND ${CMAKE_COMMAND} -E echo "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}"
	COMMAND ${CMAKE_COMMAND} -E echo "CMAKE_C_COMPILER = ${CMAKE_C_COMPILER}"
	COMMAND ${CMAKE_COMMAND} -E echo "CMAKE_C_FLAGS = ${CMAKE_C_FLAGS}"
	COMMAND ${CMAKE_COMMAND} -E echo "GTK4_LIBRARIES = ${GTK4_LIBRARIES}"
)

# =========================
# Run Target (optional)
# =========================
add_custom_target(run
	COMMAND FM-Player-Rater
	DEPENDS FM-Player-Rater
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

