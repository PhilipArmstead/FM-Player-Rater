# SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
# SPDX-License-Identifier: GPL-3.0-or-later

# Makefile for FM Player Rater (C99 + GTK4)
# Supports build modes:
#   MODE=release (default)
#   MODE=debug
#   MODE=relwithdebinfo
#
# Works on macOS/Linux directly via pkg-config.
# On Windows, use an environment that provides:
#   - gcc/clang (or compatible cc)
#   - pkg-config
#   - gtk4 pkg-config package metadata
# (e.g., MSYS2/MinGW64 shell)

# =========================
# Project Configuration
# =========================
SRC_DIR := ./src
SRCS := \
	$(SRC_DIR)/main.c

OUT_DIR := ./build
OBJ_DIR := $(OUT_DIR)/obj

# =========================
# Toolchain
# =========================
CC ?= clang
RM := rm -rf
MKDIR := mkdir -p

CSTD := -std=c99
WARNINGS := -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion \
            -Wstrict-prototypes -Wmissing-prototypes -Wno-pointer-sign \
            -Wno-missing-field-initializers -Wno-c11-extensions -Wno-gnu-empty-struct

# =========================
# Build Type Configuration
# =========================
MODE ?= release

ifeq ($(MODE),release)
	CFLAGS_MODE := -O3 -DNDEBUG
else ifeq ($(MODE),debug)
	CFLAGS_MODE := -O0 -g3 -DDEBUG -fno-omit-frame-pointer
else ifeq ($(MODE),relwithdebinfo)
	CFLAGS_MODE := -O2 -g -DNDEBUG
else
$(error Invalid MODE='$(MODE)'. Use MODE=release|debug|relwithdebinfo)
endif

TARGET := $(OUT_DIR)/$(MODE)
OBJ_DIR := $(OBJ_DIR)/$(MODE)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

# ==============================
# Platform Detection
# ==============================
# Enforce 64-bit builds where applicable
ARCH_FLAGS :=
ifeq ($(OS),Windows_NT)
	ARCH_FLAGS += -m64
	ARCH_DEFINES := -DARCH_WIN
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		ARCH_FLAGS += -m64
		ARCH_DEFINES := -DARCH_LINUX
	else ifeq ($(UNAME_S),Darwin)
	# macOS generally targets host arch by default; do not force -m64 universally.
		ARCH_DEFINES := -DARCH_MACOS
	endif
endif

GTK_CFLAGS = $(shell pkg-config --cflags gtk4)
GTK_LIBS = $(shell pkg-config --libs gtk4)
CFLAGS += $(CSTD) $(WARNINGS) $(ARCH_DEFINES) $(CFLAGS_MODE)
LDFLAGS := $(ARCH_FLAGS)
LDLIBS :=

# Generate dependency files (.d) alongside object compilation
CFLAGS += -MMD -MP

.PHONY: all release debug relwithdebinfo clean run info

all: $(TARGET)

$(OUT_DIR):
	$(MKDIR) $(OUT_DIR)

# Ensure obj dir exists (and create OUT_DIR first)
$(OBJ_DIR): | $(OUT_DIR)
	$(MKDIR) $@

# Link: link from mode-specific object files
$(TARGET): LDLIBS += $(GTK_LIBS)
$(TARGET): $(OBJS) | $(OUT_DIR)
	$(CC) $(LDFLAGS) $(LDFLAGS_MODE) -o $@ $(OBJS) $(LDLIBS)

# Pattern rule: compile .c -> .o and generate .d alongside it in OBJ_DIR
$(OBJ_DIR)/%.o: CFLAGS += $(GTK_CFLAGS)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Include generated dependency files if present
-include $(DEPS)

release:
	$(MAKE) MODE=release

debug:
	$(MAKE) MODE=debug

relwithdebinfo:
	$(MAKE) MODE=relwithdebinfo

run: all
	./$(TARGET)

info:
	@echo "MODE       = $(MODE)"
	@echo "CC         = $(CC)"
	@echo "CFLAGS     = $(CFLAGS)"
	@echo "LDFLAGS    = $(LDFLAGS)"
	@echo "LDLIBS     = $(LDLIBS)"
	@echo "TARGET     = $(TARGET)"
	@echo "SRCS       = $(SRCS)"
	@echo "OBJS       = $(OBJS)"
	@echo "DEPS       = $(DEPS)"

clean:
	$(RM) $(OUT_DIR)
