# --- Colors ---
COLOR_RESET   := \033[0m
COLOR_RED     := \033[1;31m
COLOR_GREEN   := \033[1;32m
COLOR_YELLOW  := \033[1;33m
COLOR_BLUE    := \033[1;34m
COLOR_MAGENTA := \033[1;35m

export COLOR_RESET COLOR_RED COLOR_GREEN COLOR_YELLOW COLOR_BLUE

# --- paths ---
ROOT_DIR  := $(shell pwd)
PREFIX    := /usr/local
MANPREFIX := ${PREFIX}/share/man

# --- Configuration ---
PROJECT := hive
CC      = clang
MAKEFLAGS += --no-print-directory
DEBUG   := -fsanitize=address -g -O0
RELEASE := -O3
CFLAGS  := -Wall -Wextra -Werror
LDFLAGS :=

export PROJECT CC DEBUG RELEASE CFLAGS LDFLAGS

# --- build mode ---
MODE    ?= debug
BUILD_PATH   ?=

ifeq ($(MODE),release)
  CFLAGS += $(RELEASE)
  BUILD_PATH  := build/release
else
  CFLAGS += $(DEBUG)
  BUILD_PATH  := build/debug
endif

BUILD := $(ROOT_DIR)/$(BUILD_PATH)

export MODE BUILD

# --- Information ---
GIT_TAG       := $(shell git describe --tags --abbrev=0 2>/dev/null || echo "0.0.1")
GIT_HASH      := $(shell git rev-parse --short HEAD 2>/dev/null || echo "unknown")
VERSION       := $(GIT_TAG) $(MODE)-$(shell date "+%d%m%Y")-$(GIT_HASH)
TIME_INFO     := $(shell date "+%Y/%m/%d %H:%M:%S:%p")
COMPILER_INFO := $(shell $(CC) --version | head -n 1 | cut -d' ' -f1-3)

CFLAGS += -DVERSION_INFO="\"$(VERSION)\"" \
          -DTIME_INFO="\"$(TIME_INFO)\"" \
          -DCC_INFO="\"$(COMPILER_INFO)\""

# --- Platform ---
PLATFORM ?= linux
OUTPUT   ?=

ifeq ($(PLATFORM),linux)
  CFLAGS += -D_TUX
	OUTPUT = $(PROJECT)

else ifeq ($(PLATFORM),macos)
  CC     := clang
  CFLAGS += -D_XOS
  OUTPUT := $(PROJECT)

else ifeq ($(PLATFORM),freebsd)
  CC     := clang
  CFLAGS += -D_BSD
  OUTPUT := $(PROJECT)

else ifeq ($(PLATFORM),windows)
	CC = x86_64-w64-mingw32-gcc
  CFLAGS += -D_WIN32 -mwindows
	OUTPUT = $(PROJECT).exe
endif

export OUTPUT

all:
	@$(MAKE) -C src
	@$(MAKE) -C libb

# Clean build artifact
clean:
	@echo -e "$(COLOR_BLUE)[-] Cleaning build artifacts...$(COLOR_RESET)"
	@rm -rf build/ $(PROJECT) $(PROJECT).exe

CLEAN: clean
	@echo -e "$(COLOR_BLUE)[-] Cleaning examples artifacts...$(COLOR_RESET)"
	@rm -rf examples/*.o examples/*.asm examples/*.bin

# Install
install: clean all
	@echo "Installing $(OUTPUT)..."
	@mkdir -p $(PREFIX)/bin
	@cp -f $(OUTPUT) $(PREFIX)/bin
	@chmod 755 $(PREFIX)/bin/$(OUTPUT)
	@mkdir -p $(MANPREFIX)/man1
	@sed "s/VERSION/$(VERSION)/g" < res/$(PROJECT).1 > $(MANPREFIX)/man1/$(PROJECT).1
	@chmod 644 $(MANPREFIX)/man1/$(PROJECT).1
	@echo "Installed $(OUTPUT) to $(PREFIX)/bin/.."

# Uninstall
uninstall:
	@rm -f $(MANPREFIX)/man1/$(OUTPUT).1
	@rm -f $(PREFIX)/bin/$(OUTPUT)

.PHONY: all clean CLEAN
