LIB_DIR := $(ROOT_DIR)/lib
TARGET  := $(ROOT_DIR)/$(OUTPUT)

SRCS := $(sort $(shell find $(LIB_DIR) -name '*.c'))
OBJS := $(patsubst $(LIB_DIR)/%.c,$(BUILD)/lib/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)
JSON := $(OBJS:.o=.json)

ALL_CFLAGS := $(CFLAGS) -std=c99 -I$(LIB_DIR) -MMD -MP

KEYWORDS := include/syntax/lexer/keywords

all:  format $(KEYWORDS).h $(TARGET)

$(OBJS): | $(KEYWORDS).h

$(KEYWORDS).h: $(KEYWORDS).gperf
	@printf "$(COLOR_MAGENTA)[gperf] %s$(COLOR_RESET)\n" $(notdir $@)
	@gperf -N get_keyword_kind -t $< > $@

$(TARGET): $(OBJS)
	@printf '$(COLOR_GREEN)[+] Linking %s ($(MODE))$(COLOR_RESET)\n' $(notdir $@)
	@$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.c:
	@:

$(BUILD)/lib/%.o: $(LIB_DIR)/%.c
	@mkdir -p $(dir $@)
	@printf '$(COLOR_YELLOW)[CC] %s$(COLOR_RESET)\n' $(patsubst $(LIB_DIR)/%,%,$<)
	@$(CC) $(ALL_CFLAGS) -MJ $(@:.o=.json) -c $< -o $@

compdb: $(OBJS)
	@{ echo '['; cat $(JSON) | sed '$$ s/,$$//'; echo ']'; } > $(ROOT_DIR)/compile_commands.json
	@printf '$(COLOR_MAGENTA)[=] compile_commands.json ($(words $(SRCS)) files)$(COLOR_RESET)\n'

format:
	@echo -e "$(COLOR_BLUE)[clang-format] Formatting source files...$(COLOR_RESET)"
	@clang-format -i $(SRCS)

-include $(DEPS)

$(DEPS): ;

.PHONY: all compdb
