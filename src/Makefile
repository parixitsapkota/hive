C_SOURCES := $(wildcard *.c)
H_HEADERS := $(wildcard *.h)
SRCFILES  := $(C_SOURCES) $(H_HEADERS)

OBJECTS := $(patsubst %.c, $(BUILD)/%.o, $(C_SOURCES))

SHI_SRC   := shi_arena.h shi_hs.h shi_flags.h shi_file.h
SHI_FILES := $(patsubst %.h, dep/%.h, $(SHI_SRC))

.DELETE_ON_ERROR:

all: dependency $(OBJECTS)
	@echo -e "$(COLOR_YELLOW)[#] Linking $(OUTPUT) $(COLOR_BLUE)$(MODE)$(COLOR_YELLOW) mode...$(COLOR_RESET)"
	@$(CC) $(CFLAGS) $(OBJECTS) -o ../$(OUTPUT)

dependency: $(SHI_FILES) dep/keywords.h

$(SHI_FILES):

dep/keywords.h: ../res/keywords.gperf
	@mkdir -p $(dir $@)
	@printf "$(COLOR_MAGENTA)[+] Creating $@...$(COLOR_RESET)\n"
	@gperf -N get_keyword_kind -t $< > $@

# rule to download missing SHI headers
dep/%.h:
	@mkdir -p $(dir $@)
	@printf "$(COLOR_MAGENTA)[+] Downloading $@...$(COLOR_RESET)\n"
	@wget -q https://raw.githubusercontent.com/parixitsapkota/SHI/refs/heads/main/$(notdir $@) -O $@ || (rm -f $@ && exit 1)

$(OBJECTS):

# Compile sourcefile
$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo -e "$(COLOR_GREEN)[+] Compiling $<...$(COLOR_RESET)"
	@$(CC) $(CFLAGS) -c $< -o $@

# Format sourcefile
format:
	@echo -e "$(COLOR_BLUE)[-] Formatting source files...$(COLOR_RESET)"
	@clang-format -i $(SRCFILES)

.PHONY: all format
