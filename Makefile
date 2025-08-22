# Makefile to compile bootloader-attiny
#
# Copyright 2017 Matthijs Kooijman <matthijs@stdin.nl>
#
# Permission is hereby granted, free of charge, to anyone obtaining a
# copy of this document to do whatever they want with them without any
# restriction, including, but not limited to, copying, modification and
# redistribution.
#
# NO WARRANTY OF ANY KIND IS PROVIDED.
#
# To compile, just make sure that avr-gcc and friends are in your path
# and type "make".

# Protocol version major version must fit
# Major 0xff00, minor 0x00ff
PROTOCOL_VERSION = 0x0302

CPPSRC         = $(wildcard *.cpp)
CPPSRC        += $(ARCH)/SelfProgram.cpp $(ARCH)/uart.cpp $(ARCH)/Reset.cpp $(ARCH)/Clock.cpp
CPPSRC        += $(ARCH)/$(BUS).cpp
CPPSRC        += $(ARCH)/power_panic.cpp $(ARCH)/fan.cpp $(ARCH)/iwdg.cpp $(ARCH)/otp.cpp
HALSRC         =
ifeq ($(ARCH),stm32-h5hal)
HALSRC        += stm32h5xx_hal_driver/Src/stm32h5xx_hal_cortex.c
HALSRC        += stm32h5xx_hal_driver/Src/stm32h5xx_hal_flash.c
HALSRC        += stm32h5xx_hal_driver/Src/stm32h5xx_hal_flash_ex.c
HALSRC        += stm32h5xx_hal_driver/Src/stm32h5xx_hal.c
HALSRC        += stm32h5xx_hal_driver/Src/stm32h5xx_ll_rcc.c
HALSRC        += stm32h5xx_hal_driver/Src/stm32h5xx_ll_usart.c
HALSRC        += stm32h5xx_hal_driver/Src/stm32h5xx_ll_gpio.c
HALSRC        += stm32h5xx_hal_driver/Src/stm32h5xx_ll_utils.c
HALSRC        += stm32-h5hal/system_stm32h5xx.c
HALSRC        += stm32-h5hal/startup_stm32h503cbux.s
endif

ifeq ($(ARCH),stm32-f4hal)
HALSRC		  += stm32f4xx_hal_driver/Src/stm32f4xx_hal_gpio.c
HALSRC        += stm32f4xx_hal_driver/Src/stm32f4xx_hal_cortex.c
HALSRC        += stm32f4xx_hal_driver/Src/stm32f4xx_hal_rcc.c
HALSRC        += stm32f4xx_hal_driver/Src/stm32f4xx_hal_uart.c
HALSRC        += stm32f4xx_hal_driver/Src/stm32f4xx_hal_flash.c
HALSRC        += stm32f4xx_hal_driver/Src/stm32f4xx_hal_flash_ex.c
HALSRC        += stm32f4xx_hal_driver/Src/stm32f4xx_hal.c
HALSRC        += stm32f4xx_hal_driver/Src/stm32f4xx_ll_rcc.c
HALSRC        += stm32f4xx_hal_driver/Src/stm32f4xx_ll_usart.c
HALSRC        += stm32f4xx_hal_driver/Src/stm32f4xx_ll_gpio.c
HALSRC        += stm32f4xx_hal_driver/Src/stm32f4xx_ll_utils.c
HALSRC        += stm32-f4hal/system_stm32f4xx.c
HALSRC        += stm32-f4hal/startup_stm32f427xx.s
HALSRC		  += stm32-f4hal/setup_stdout.c
HALSRC		  += stm32-f4hal/Gpio.c
endif

OBJ            = $(CPPSRC:.cpp=.o)
EXTRA_OBJ_1    = $(HALSRC:.c=.o)
EXTRA_OBJ      = $(EXTRA_OBJ_1:.s=.o)
ifeq ($(ARCH),attiny)
	LDSCRIPT       = $(ARCH)/linker-script.x
	MCU            = attiny841
	FLASH_WRITE_SIZE    = SPM_PAGESIZE # Defined by avr-libc
	FLASH_ERASE_SIZE    = 64
	FLASH_SIZE          = 8192
	# Size of the bootloader area. Must be a multiple of the erase size
	BL_SIZE        = 2048
	FLASH_APP_OFFSET    = 0
	BL_OFFSET           = $(shell expr $(FLASH_SIZE) - $(BL_SIZE))
else ifeq ($(ARCH),stm32-ocm3)
	LDSCRIPT            = stm32-ocm3/stm32g070rbt6.ld
	OPENCM3_DIR         = libopencm3
	DEVICE              = STM32G070RBT6
	FLASH_WRITE_SIZE    = 256
	FLASH_ERASE_SIZE    = 2048
	# Size of the bootloader area. Must be a multiple of the erase size
	BL_SIZE             = 8192
	BL_OFFSET           = 0

	# Bootloader is at the start of flash, so write app after it
	FLASH_APP_OFFSET    = $(BL_SIZE)
	# actual flash size is 128kb, but we are currently limited to application size of 64kb due offset being uint16
	APPLICATION_SIZE    = (128*1024-FLASH_APP_OFFSET)
	# there is 128 bytes of FW descriptor at the end of app space
	FW_DESCRIPTOR_SIZE = 128

else ifeq ($(ARCH),stm32-h5hal)
	LDSCRIPT            = stm32-h5hal/stm32h503cbux.ld
	FLASH_WRITE_SIZE    = 8192
	FLASH_ERASE_SIZE    = 8192

	BL_SIZE             = 8192
	BL_OFFSET           = 0

	# Bootloader is at the start of flash, so write app after it
	FLASH_APP_OFFSET    = $(BL_SIZE)
	# actual flash size is 128kb, but we are currently limited to application size of 64kb due offset being uint16
	APPLICATION_SIZE    = (128*1024-FLASH_APP_OFFSET)
	# there is 128 bytes of FW descriptor at the end of app space
	FW_DESCRIPTOR_SIZE = 128
else ifeq ($(ARCH),stm32-f4hal)
	LDSCRIPT            = stm32-f4hal/STM32F427ZITx_FLASH.ld
	FW_DESCRIPTOR_SIZE  = 128
	# TODO: It seem that F427 has different sizes of flash sectors
	# - update / replace this later

	# This chip has different sizes of erase&write sectors,
	# -> this does not make sense. FIXME: replace with sector table
	FLASH_WRITE_SIZE    = 16384
	FLASH_ERASE_SIZE    = 16384

	BL_SIZE             = 16384 # Size of the first flash sector
	BL_OFFSET           = 0
	FLASH_APP_OFFSET    = BL_SIZE
	APPLICATION_SIZE    = 2048*1024-FLASH_APP_OFFSET
endif

VERSION_SIZE   = 7

ifdef DEBUG
#Debug version passes check in puppy
	BL_VERSION     ?= 2147483647
	BL_VERSION_PREFIX ?=
else
	BL_VERSION     ?= 1
	BL_VERSION_PREFIX ?=
endif

CXXFLAGS       =
CXXFLAGS      += -Wall -Wextra
CXXFLAGS      += -fpack-struct -fshort-enums

CXXFLAGS  += -ggdb3 -Os

# Comment this out to enable watchdog, or put it into DEBUG to enable it only in release
CXXFLAGS  += -DDISABLE_WATCHDOG

#CXXFLAGS      += -flto -ffat-lto-objects
# I would think these are not required with -flto, but adding these
# removes a lot of unused functions that lto apparently leaves...
CXXFLAGS      += -ffunction-sections -fdata-sections -Wl,--gc-sections
CXXFLAGS      += -fno-exceptions

CXXFLAGS      += -I$(ARCH) -I.
ifeq ($(ARCH),stm32-h5hal)
CXXFLAGS      += -Istm32h5xx_hal_driver/Inc
CXXFLAGS	  += -Istm32-h5hal/CMSIS/Device/ST/STM32H5xx/Include
CXXFLAGS	  += -Istm32-h5hal/CMSIS/Include

CXXFLAGS      += -mcpu=cortex-m33 -mfloat-abi=hard -mfpu=fpv5-sp-d16

CXXFLAGS	  += -DSTM32H503xx
CXXFLAGS	  += -DSTM32H5
CXXFLAGS	  += -DUSE_HAL_DRIVER
CXXFLAGS	  += -DUSE_FULL_LL_DRIVER
CXXFLAGS	  += -DFIXED_ADDRESS=17 # it's 8th puppy (1 MB + 6 DW) and bootloader addresses are starting from 10

CXXFLAGS      += -flto -ffat-lto-objects

endif

ifeq ($(ARCH),stm32-f4hal)
    CXXFLAGS          += -Istm32f4xx_hal_driver/Inc
    CXXFLAGS          += -Istm32-f4hal/CMSIS/Device/ST/STM32F4xx/Include
    CXXFLAGS          += -Istm32-f4hal/CMSIS/Include
    CXXFLAGS          += -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv5-sp-d16 -mthumb
    CXXFLAGS          += -DSTM32F427xx
    CXXFLAGS          += -DSTM32F4
    CXXFLAGS          += -DUSE_HAL_DRIVER
    CXXFLAGS          += -DUSE_FULL_LL_DRIVER
    ifeq ($(BOARD_TYPE), prusa_smartled01)
        CXXFLAGS      += -DFIXED_ADDRESS=3
    else ifeq ($(BOARD_TYPE), prusa_baseboard)
        CXXFLAGS      += -DFIXED_ADDRESS=2
	else
		$(error unknown f4 board")
    endif
    CXXFLAGS          += -DBL_SIZE=$(BL_SIZE)
    CXXFLAGS          += -flto -ffat-lto-objects
endif

CXXFLAGS      += -DVERSION_SIZE=$(VERSION_SIZE)
CXXFLAGS      += -DFLASH_ERASE_SIZE=$(FLASH_ERASE_SIZE)
CXXFLAGS      += -DFLASH_WRITE_SIZE=$(FLASH_WRITE_SIZE)
CXXFLAGS      += -DFLASH_APP_OFFSET=$(FLASH_APP_OFFSET)
CXXFLAGS      += -DPROTOCOL_VERSION=$(PROTOCOL_VERSION) -DBOARD_TYPE_$(BOARD_TYPE)
CXXFLAGS      += -DHARDWARE_REVISION=$(CURRENT_HW_REVISION) -DHARDWARE_COMPATIBLE_REVISION=$(COMPATIBLE_HW_REVISION)
CXXFLAGS      += -DBL_VERSION=$(BL_VERSION)
ifneq ($(EXTRA_INFO),)
CXXFLAGS      += -DEXTRA_INFO=$(EXTRA_INFO)
endif

ifeq ($(BUS),TwoWire)
CXXFLAGS      += -DUSE_I2C
else ifeq ($(BUS),Rs485)
CXXFLAGS      += -DUSE_RS485
endif

LDFLAGS        =
# Use a custom linker script
LDFLAGS       += -T $(LDSCRIPT)

ifeq ($(ARCH),attiny)
PREFIX         = avr-
SIZE_FORMAT    = avr

LDFLAGS       += -mmcu=$(MCU)

CXXFLAGS      += -mmcu=$(MCU) -DF_CPU=8000000UL

# Pass sizes to the script for positioning
LDFLAGS       += -Wl,--defsym=BL_SIZE=$(BL_SIZE)
LDFLAGS       += -Wl,--defsym=VERSION_SIZE=$(VERSION_SIZE)
# Pass ERASE_SIZE to the script to verify alignment
LDFLAGS       += -Wl,--defsym=FLASH_ERASE_SIZE=$(FLASH_ERASE_SIZE)
else ifeq ($(ARCH),stm32-ocm3)
PREFIX         = arm-none-eabi-
SIZE_FORMAT    = berkely

CXXFLAGS      += -DSTM32
CXXFLAGS      += -DAPPLICATION_SIZE="($(APPLICATION_SIZE))"
CXXFLAGS      += -DFW_DESCRIPTOR_SIZE="($(FW_DESCRIPTOR_SIZE))"
LDFLAGS       += -nostartfiles
LDFLAGS       += -specs=nano.specs
LDFLAGS       += -specs=nosys.specs
# TODO: Position VERSION constant
else ifeq ($(ARCH),stm32-h5hal)
PREFIX         = arm-none-eabi-
SIZE_FORMAT    = berkely

CXXFLAGS      += -DSTM32
CXXFLAGS      += -DAPPLICATION_SIZE="($(APPLICATION_SIZE))"
CXXFLAGS      += -DFW_DESCRIPTOR_SIZE="($(FW_DESCRIPTOR_SIZE))"
LDFLAGS       += -nostartfiles
LDFLAGS       += -specs=nano.specs
LDFLAGS       += -specs=nosys.specs

else ifeq ($(ARCH),stm32-f4hal)
PREFIX         = arm-none-eabi-
SIZE_FORMAT    = berkely

CXXFLAGS      += -DSTM32
CXXFLAGS      += -DAPPLICATION_SIZE="($(APPLICATION_SIZE))"
CXXFLAGS      += -DFW_DESCRIPTOR_SIZE="($(FW_DESCRIPTOR_SIZE))"
LDFLAGS       += -nostartfiles
LDFLAGS       += -specs=nano.specs
LDFLAGS       += -specs=nosys.specs
endif

CC             = $(PREFIX)gcc
OBJCOPY        = $(PREFIX)objcopy
OBJDUMP        = $(PREFIX)objdump
SIZE           = $(PREFIX)size

ifdef OPENCM3_DIR
include $(OPENCM3_DIR)/mk/genlink-config.mk
# override LDSCRIPT set by the library, as we don't want to auto-generate it
LDSCRIPT = stm32-ocm3/stm32g070rbt6.ld
ifeq ($(LIBNAME),)
$(error libopencm3 library not found, compile it first with "make -C libopencm3 lib/stm32/g0 CFLAGS='-flto -fno-fat-lto-objects'")
endif
# These are generated by genlink-config (along with LDSCRIPT)
CXXFLAGS      += $(CPPFLAGS)
CXXFLAGS      += $(ARCH_FLAGS)
endif

ifdef CURRENT_HW_REVISION
  CURRENT_HW_REVISION_MAJOR=$(shell echo $$(($(CURRENT_HW_REVISION) / 0x10)))
  CURRENT_HW_REVISION_MINOR=$(shell echo $$(($(CURRENT_HW_REVISION) % 0x10)))

ifdef DEBUG
  FILE_NAME=bootloader-debug-$(BOARD_TYPE)-$(CURRENT_HW_REVISION_MAJOR).$(CURRENT_HW_REVISION_MINOR)$(BOARD_VARIANT)
else
  FILE_NAME=bootloader-$(BL_VERSION_PREFIX)v$(BL_VERSION)-$(BOARD_TYPE)-$(CURRENT_HW_REVISION_MAJOR).$(CURRENT_HW_REVISION_MINOR)$(BOARD_VARIANT)
endif
endif

# Make sure that .o files are deleted after building, so we can build for multiple
# hw revisions without needing an explicit clean in between.
.INTERMEDIATE: $(OBJ) $(EXTRA_OBJ)

all: dwarf modularbed xbuddy_extension prusa_baseboard prusa_smartled01

dwarf:
	$(MAKE) firmware ARCH=stm32-ocm3 BUS=Rs485 BOARD_TYPE=prusa_dwarf CURRENT_HW_REVISION=0x10 COMPATIBLE_HW_REVISION=0x10

modularbed:
	$(MAKE) firmware ARCH=stm32-ocm3 BUS=Rs485 BOARD_TYPE=prusa_modular_bed CURRENT_HW_REVISION=0x10 COMPATIBLE_HW_REVISION=0x10

xbuddy_extension:
	$(MAKE) firmware ARCH=stm32-h5hal BUS=Rs485 BOARD_TYPE=prusa_xbuddy_extension CURRENT_HW_REVISION=0x10 COMPATIBLE_HW_REVISION=0x10

dwarf_debug:
	$(MAKE) firmware DEBUG=1 ARCH=stm32-ocm3 BUS=Rs485 BOARD_TYPE=prusa_dwarf CURRENT_HW_REVISION=0x10 COMPATIBLE_HW_REVISION=0x10

modularbed_debug:
	$(MAKE) firmware DEBUG=1 ARCH=stm32-ocm3 BUS=Rs485 BOARD_TYPE=prusa_modular_bed CURRENT_HW_REVISION=0x10 COMPATIBLE_HW_REVISION=0x10

xbuddy_extension_debug:
	$(MAKE) firmware DEBUG=1 ARCH=stm32-h5hal BUS=Rs485 BOARD_TYPE=prusa_xbuddy_extension CURRENT_HW_REVISION=0x10 COMPATIBLE_HW_REVISION=0x10

smartled01:
	$(MAKE) firmware ARCH=stm32-f4hal BUS=Rs485 BOARD_TYPE=prusa_smartled01 CURRENT_HW_REVISION=0x10 COMPATIBLE_HW_REVISION=0x10

smartled01_debug:
	$(MAKE) firmware DEBUG=1 ARCH=stm32-f4hal BUS=Rs485 BOARD_TYPE=prusa_smartled01 CURRENT_HW_REVISION=0x10 COMPATIBLE_HW_REVISION=0x10

baseboard:
	$(MAKE) firmware ARCH=stm32-f4hal BUS=Rs485 BOARD_TYPE=prusa_baseboard CURRENT_HW_REVISION=0x10 COMPATIBLE_HW_REVISION=0x10

baseboard_debug:
	$(MAKE) firmware DEBUG=1 ARCH=stm32-f4hal BUS=Rs485 BOARD_TYPE=prusa_baseboard CURRENT_HW_REVISION=0x10 COMPATIBLE_HW_REVISION=0x10

firmware: hex fuses size checksize

hex: $(FILE_NAME).hex

fuses:
ifdef ATTINY
	@if $(OBJDUMP) -s -j .fuse 2> /dev/null $(FILE_NAME).elf > /dev/null; then \
		$(OBJDUMP) -s -j .fuse $(FILE_NAME).elf; \
		echo "        ^^     Low"; \
		echo "          ^^   High"; \
		echo "            ^^ Extended"; \
	fi
endif

size:
	$(SIZE) --format=$(SIZE_FORMAT) $(FILE_NAME).elf

clean:
	$(MAKE) cleanarch ARCH=stm32-ocm3 BUS=Rs485
	$(MAKE) cleanarch ARCH=stm32-h5hal BUS=RS485

cleanarch:
	rm -rf $(OBJ) $(OBJ:.o=.d) $(EXTRA_OBJ) $(EXTRA_OBJ:.o=.d) *.elf *.hex *.lst *.map *.bin

$(FILE_NAME).elf: $(OBJ) $(EXTRA_OBJ) $(LDSCRIPT) $(LIBDEPS)
	$(CC) $(CXXFLAGS) $(LDFLAGS) -o $@ $(EXTRA_OBJ) $(OBJ) $(LDLIBS)

%.o: %.cpp Makefile
	$(CC) -std=gnu++11 $(CXXFLAGS) -MMD -MP -c -o $@ $<

%.o: %.c Makefile
	$(CC) -std=gnu99 $(CXXFLAGS) -MMD -MP -c -o $@ $<

%.o: %.s Makefile
	$(CC) $(CXXFLAGS) -MMD -MP -c -o $@ $<

%.lst: %.elf
	$(OBJDUMP) -h -S $< > $@

%.hex: %.elf
	$(OBJCOPY) -j .isr_vector -j .text -j '.text.*' -j .rodata -j .data -O ihex $< $@

%.bin: %.elf
	$(OBJCOPY) -j .isr_vector -j .text -j '.text.*' -j .rodata -j .data -O binary $< $@

# When the bootloader has an offset, objcopy pads the pin file at the
# start, so correct for that.
MAX_BIN_SIZE=$(shell expr $(BL_OFFSET) + $(BL_SIZE))
checksize: $(FILE_NAME).bin
	@echo $$(stat -c '%s' $<)
	@if [ $$(stat -c '%s' $<) -gt $(MAX_BIN_SIZE) ]; then \
		echo "Compiled size too big, maybe adjust BL_SIZE in Makefile?"; \
		false; \
	fi

.PHONY: all lst hex clean fuses size firmware

# pull in dependency info for *existing* .o files
-include $(OBJ:.o=.d)
-include $(EXTRA_OBJ:.o=.d)
