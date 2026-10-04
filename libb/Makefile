OUT_DIR := $(BUILD)/../libb/

all:
	@mkdir -p $(OUT_DIR)

	@../$(OUTPUT) -i "brt.b" -o "$(OUT_DIR)/brt.asm"
	@nasm -f elf64 "$(OUT_DIR)/brt.asm" -o "$(OUT_DIR)/brt1.o"
	@nasm -f elf64 "brt.asm" -o "$(OUT_DIR)/brt0.o"
	@ld -r "$(OUT_DIR)/brt0.o" "$(OUT_DIR)/brt1.o" -o "$(OUT_DIR)/../brt.o"

	@../$(OUTPUT) -i "libb.b" -o "$(OUT_DIR)/libb.asm"
	@nasm -f elf64 "$(OUT_DIR)/libb.asm" -o "$(OUT_DIR)/libb1.o"
	@nasm -f elf64 "libb.asm" -o "$(OUT_DIR)/libb0.o"
	@ld -r "$(OUT_DIR)/libb0.o" "$(OUT_DIR)/libb1.o" -o "$(OUT_DIR)/../libb.o"
