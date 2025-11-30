SRC_DIR ?= ./
OBJ_DIR ?= ./
SOURCES ?= $(shell find $(SRC_DIR) -name '*.c' -or -name '*.S')
OBJECTS ?= $(addsuffix .o, $(basename $(notdir $(SOURCES))))
LINKER ?= $(SRC_DIR)/dtekv-script.lds

TOOLCHAIN ?= riscv32-unknown-elf-
CFLAGS ?= -Wall -nostdlib -O3 -mabi=ilp32 -march=rv32imzicsr -fno-builtin

# benchmarking
CFLAGS_O0 = -Wall -nostdlib -O0 -mabi=ilp32 -march=rv32imzicsr -fno-builtin -DPERF
CFLAGS_O3 = -Wall -nostdlib -O3 -mabi=ilp32 -march=rv32imzicsr -fno-builtin -DPERF

build: clean main.bin

main.elf: 
	$(TOOLCHAIN)gcc -c $(CFLAGS) $(SOURCES)
	$(TOOLCHAIN)ld -o $@ -T $(LINKER) $(filter-out boot.o, $(OBJECTS)) softfloat.a

main.bin: main.elf
	$(TOOLCHAIN)objcopy --output-target binary $< $@
	$(TOOLCHAIN)objdump -D $< > $<.txt

#benchmarking
bench_O0.elf:
	$(TOOLCHAIN)gcc -c $(CFLAGS_O0) $(SOURCES)
	$(TOOLCHAIN)ld -o $@ -T $(LINKER) $(filter-out boot.o, $(OBJECTS)) softfloat.a

bench_O3.elf:
	$(TOOLCHAIN)gcc -c $(CFLAGS_O3) $(SOURCES)
	$(TOOLCHAIN)ld -o $@ -T $(LINKER) $(filter-out boot.o, $(OBJECTS)) softfloat.a

bench_O0.bin: bench_O0.elf
	$(TOOLCHAIN)objcopy --output-target binary $< $@
	$(TOOLCHAIN)objdump -D $< > $<.txt

bench_O3.bin: bench_O3.elf
	$(TOOLCHAIN)objcopy --output-target binary $< $@
	$(TOOLCHAIN)objdump -D $< > $<.txt

TOOL_DIR ?= ./tools
run: main.bin
	make -C $(TOOL_DIR) "FILE_TO_RUN=$(CURDIR)/$<"

#benchmarking
run_O0: bench_O0.bin
	make -C $(TOOL_DIR) "FILE_TO_RUN=$(CURDIR)/$<"
run_O3: bench_O3.bin
	make -C $(TOOL_DIR) "FILE_TO_RUN=$(CURDIR)/$<"

clean:
	rm -f *.o *.elf *.bin *.txt
