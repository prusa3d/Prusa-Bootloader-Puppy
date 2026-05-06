PREFIX = arm-none-eabi-
CXX = $(PREFIX)g++
OBJCOPY = $(PREFIX)objcopy

CMSIS_DIR = ../stm32-c0hal/CMSIS
LINKER_SCRIPT = preboot.ld
OBJ = preboot.o preboot_vector_table.o preboot_jump.o

CXXFLAGS  = -Werror -Wall -Wextra -pedantic -std=c++23 -Os
CXXFLAGS += -ffunction-sections -fdata-sections -fno-exceptions
CXXFLAGS += -mcpu=cortex-m0plus -DSTM32C092xx -I$(CMSIS_DIR)/Include -I$(CMSIS_DIR)/Device/ST/STM32C0xx/Include

LDFLAGS  = -nostartfiles
LDFLAGS += -Wl,-T,$(LINKER_SCRIPT),--print-memory-usage,--gc-sections
LDFLAGS += -Wl,--defsym=PREBOOT_SIZE=$(PREBOOT_SIZE)
LDFLAGS += -Wl,--defsym=BOOTLOADER_SIZE=$(BOOTLOADER_SIZE)

.PHONY: all clean

all: preboot.bin

clean:
	rm -f *.bin *.elf $(OBJ)

preboot.bin: preboot.elf Makefile
	$(OBJCOPY) -O binary --pad-to=$$((0x08000000 + $(PREBOOT_SIZE))) --gap-fill=0xff $< $@

preboot.elf: $(OBJ) $(LINKER_SCRIPT) Makefile
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(OBJ)

%.o: %.cpp Makefile
	$(CXX) $(CXXFLAGS) -o $@ -c $<

%.o: %.s Makefile
	$(CXX) $(CXXFLAGS) -o $@ -c $<
