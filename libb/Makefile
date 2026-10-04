OUT_DIR := $(BUILD)/../libb/

all:
	@mkdir -p $(OUT_DIR) ../lib/

	@../$(OUTPUT) -i "brt.b" -c
	@mv "brt.o"  "$(OUT_DIR)/brt1.o"
	@nasm -f elf64 "brt.asm" -o "$(OUT_DIR)/brt0.o"
	@ld -r "$(OUT_DIR)/brt0.o" "$(OUT_DIR)/brt1.o" -o "../lib/brt.o"

	@../$(OUTPUT) -i "libb.b" -c
	@mv "libb.o" "$(OUT_DIR)/libb1.o"
	@nasm -f elf64 "libb.asm" -o "$(OUT_DIR)/libb0.o"
	@ld -r "$(OUT_DIR)/libb0.o" "$(OUT_DIR)/libb1.o" -o "../lib/libb.o"
