# --- Colors ---
COLOR_RESET   := \033[0m
COLOR_RED     := \033[1;31m
COLOR_GREEN   := \033[1;32m
COLOR_YELLOW  := \033[1;33m
COLOR_BLUE    := \033[1;34m
COLOR_MAGENTA := \033[1;35m

# --- Configuration ---
PROJECT := trident
CC      = clang

DEBUG   := -fsanitize=address -g -O0
RELEASE := -O3
CFLAGS  := -Isrc -Wall -Wextra -Werror
LDFLAGS :=

# --- paths ---
PREFIX    := /usr/local
MANPREFIX := ${PREFIX}/share/man

MODE    ?= debug
BUILD   ?=

ifeq ($(MODE),release)
  CFLAGS += $(RELEASE)
  BUILD  := build/release
else
  CFLAGS += $(DEBUG)
  BUILD  := build/debug
endif

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

C_SOURCES := $(wildcard src/*.c)
H_HEADERS := $(wildcard src/*.h)
SRCFILES  := $(C_SOURCES) $(H_HEADERS)

OBJECTS := $(patsubst src/%.c, $(BUILD)/%.o, $(C_SOURCES))

SHI_SRC   := shi_arena.h shi_hs.h shi_flags.h shi_file.h
SHI_FILES := $(patsubst %.h, src/dep/%.h, $(SHI_SRC))

.PHONY: all clean dependency format

.DELETE_ON_ERROR:

all: format dependency $(OUTPUT)

dependency: $(SHI_FILES) src/dep/keywords.h

$(SHI_FILES):

src/dep/keywords.h: res/keywords.gperf
	@mkdir -p $(dir $@)
	@printf "$(COLOR_MAGENTA)[+] Creating $@...$(COLOR_RESET)\n"
	@gperf -N get_keyword_kind -t $< > $@

# rule to download missing SHI headers
src/dep/%.h:
	@mkdir -p $(dir $@)
	@printf "$(COLOR_MAGENTA)[+] Downloading $@...$(COLOR_RESET)\n"
	@wget -q https://raw.githubusercontent.com/parixitsapkota/SHI/refs/heads/main/$(notdir $@) -O $@ || (rm -f $@ && exit 1)

# Link the main exe
$(OUTPUT): $(OBJECTS)
	@echo -e "$(COLOR_YELLOW)[#] Linking $(OUTPUT) $(COLOR_BLUE)$(MODE)$(COLOR_YELLOW) mode...$(COLOR_RESET)"
	@$(CC) $(CFLAGS) $(OBJECTS) -o $(OUTPUT)

$(OBJECTS):

# Compile sourcefile
$(BUILD)/%.o: src/%.c
	@mkdir -p $(dir $@)
	@echo -e "$(COLOR_GREEN)[+] Compiling $<...$(COLOR_RESET)"
	@$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifact
clean: clean_build

clean_all: clean_build clean_examples clean_deps

clean_build:
	@echo -e "$(COLOR_BLUE)[-] Cleaning build artifacts...$(COLOR_RESET)"
	@rm -rf build/ $(PROJECT) $(PROJECT).exe

clean_examples:
	@echo -e "$(COLOR_BLUE)[-] Cleaning examples artifacts...$(COLOR_RESET)"
	@rm -rf examples/*.o examples/*.asm examples/*.bin

clean_deps:
	@echo -e "$(COLOR_BLUE)[-] Cleaning dependencies...$(COLOR_RESET)"
	@rm -rf src/dep/

# Format sourcefile
format:
	@echo -e "$(COLOR_BLUE)[-] Formatting source files...$(COLOR_RESET)"
	@clang-format -i $(SRCFILES)

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

info:
	@echo $(PROJECT) $(VERSION)
	@echo "Compiler : $(COMPILER_INFO)"
	@echo "Built    : $(TIME_INFO)"

EXAMPLE ?= $(wildcard examples/*.b)

run:
	@printf "\n\n"
	@./$(OUTPUT) -v
	@printf "\n\n"
	@mkdir -p lib/ examples/

	@./$(OUTPUT) -i "res/brt.b" -o "lib/brt.asm"
	@nasm -f elf64 "lib/brt.asm" -o "lib/brt1.o"
	@nasm -f elf64 "res/brt.asm" -o "lib/brt0.o"
	@ld -r "lib/brt0.o" "lib/brt1.o" -o "lib/brt.o"

	@./$(OUTPUT) -i "res/libb.b" -o "lib/libb.asm"
	@nasm -f elf64 "lib/libb.asm" -o "lib/libb1.o"
	@nasm -f elf64 "res/libb.asm" -o "lib/libb0.o"
	@ld -r "lib/libb0.o" "lib/libb1.o" -o "lib/libb.o"

	@for file in $(EXAMPLE); do \
		name=$$(basename "$$file" .b); \
		printf "$(COLOR_MAGENTA)[+] Compiling $$file...$(COLOR_RESET)\n"; \
		./$(OUTPUT) -i "$$file" -o "examples/$$name.asm" || exit 1; \
		printf "$(COLOR_GREEN)[+] Assembling examples/$$name.asm...$(COLOR_RESET)\n"; \
		nasm -f elf64 "examples/$$name.asm" -o "examples/$$name.o" || exit 1; \
		printf "$(COLOR_YELLOW)[#] Linking examples/$$name.o...$(COLOR_RESET)\n"; \
		ld -o "examples/$$name.bin" "examples/$$name.o" "lib/libb.o" "lib/brt.o" || exit 1; \
		./examples/$$name.bin; status=$$?; \
		printf "$(COLOR_BLUE)[+] $$name.b : Exit-code : $(COLOR_RED)%d$(COLOR_RESET)\n\n" $$status; \
	done
