/*
 * Copyright (C) 2017 3devo (http://www.3devo.eu)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <libopencm3/stm32/desig.h>
#include <libopencm3/stm32/iwdg.h>
#include <libopencm3/stm32/flash.h>
#include <stdio.h>

#include "Config.h"
#include "Bus.h"
#include "BaseProtocol.h"
#include "SelfProgram.h"
#include "bootloader.h"
#include "led.hpp"
#include "crash_dump_shared.hpp"
#include "Gpio.h"


struct Commands {
	// See also ProtocolCommands in BaseProtocol.h
	static const uint8_t GET_HARDWARE_INFO     = 0x03;
	static const uint8_t START_APPLICATION     = 0x05;
	static const uint8_t WRITE_FLASH           = 0x06;
	static const uint8_t FINALIZE_FLASH        = 0x07;
	static const uint8_t READ_FLASH            = 0x08;
	static const uint8_t GET_FINGERPRINT       = 0x0e;
	static const uint8_t COMPUTE_FINGERPRINT   = 0x0f;
	static const uint8_t READ_OTP              = 0x10;

	// These were removed and should not be used
	static const uint8_t RESERVED_02 = 0x02; ///< POWER_UP_DISPLAY     
	static const uint8_t RESERVED_04 = 0x04; ///< GET_SERIAL_NUMBER
	static const uint8_t RESERVED_09 = 0x09; ///< GET_HARDWARE_REVISION
	static const uint8_t RESERVED_0a = 0x0a; ///< GET_NUM_CHILDREN     
	static const uint8_t RESERVED_0b = 0x0b; ///< SET_CHILD_SELECT     
	static const uint8_t RESERVED_0d = 0x0d; ///< GET_EXTRA_INFO       
	static const uint8_t RESERVED_46 = 0x46; ///< RESET
	static const uint8_t RESERVED_44 = 0x44; ///< RESET_ADDRESS
};

struct VersionInfoInFlash {
    uint8_t hw_compatible_version; uint8_t hw_revision; uint8_t hw_type; uint32_t bl_version;
};

// Put version info into flash, so applications can read this to
// determine the hardware version. The linker puts this at a fixed
// position at the end of flash.
constexpr const struct VersionInfoInFlash version_info __attribute__((__section__(".version"), __used__)) = {
	HARDWARE_COMPATIBLE_REVISION,
	HARDWARE_REVISION,
	INFO_HW_TYPE,
	BL_VERSION,
};

struct __attribute__((packed)) ApplicationStartupArguments {
    uint8_t modbus_address;
};

struct ApplicationStartupArguments application_startup_arguments __attribute__((__section__(".app_args"), __used__)) = {
    .modbus_address = 0xFF,
};

// OTP area
static constexpr uint32_t OTP_START_ADDR = 0x1FFF7000UL;
static constexpr uint32_t OTP_SIZE = 1024;

// Check that the version info size used by the linker (which must be
// hardcoded...) is correct.
static_assert(sizeof(version_info) == VERSION_SIZE, "Version section has wrong size?");

// Exit the bootloader
// Either check the internal unsalted fingerprint
// or get salt and fingerprint from buddy
volatile bool bootloaderExit = false;	///< Exit with check of the internal fingerprint
volatile bool bootloaderFingerprintMatch = false;	///< True if fingerprint was checked by buddy

// Note that we must buffer a full erase page size (not smaller), since
// we must know at the start of an erase page whether any byte in the
// entire page is changed to decide whether or not to erase.
static uint8_t writeBuffer[FLASH_ERASE_SIZE];
static uint32_t nextWriteAddress = 0;

// Helper function that is declared but not defined, to allow
// semi-static assertions (where input to a check is not really const,
// but can be derived by the optimizer, so if the check passes, the call
// to this function is optimized away, and if not, produces a linker
// error).
void compiletime_check_failed();

// Disable compile-time check (doesn't work on gcc 7 without LTO)
void compiletime_check_failed() {}

static bool equalToFlash(uint32_t address, uint16_t len) {
	uint16_t offset = 0;
	while (len > 0) {
		if (writeBuffer[offset] != SelfProgram::readByte(address + offset))
			return false;
		--len;
		++offset;
	}
	return true;
}


static uint8_t commitToFlash(uint32_t address, uint16_t len) {
	// If nothing needs to be changed, then don't
	if (equalToFlash(address, len))
		return 0;

	uint16_t offset = 0;
	while (len > 0) {
		uint16_t pageLen = len < FLASH_WRITE_SIZE ? len : FLASH_WRITE_SIZE;
		uint8_t err = SelfProgram::writePage(address + offset, &writeBuffer[offset], pageLen);
		if (err)
			return err;
		len -= pageLen;
		offset += pageLen;
	}
	return 0;
}

static cmd_result handleWriteFlash(uint32_t address, uint8_t *data, uint16_t len, uint8_t *dataout) {
	if (address == 0)
		nextWriteAddress = 0;

	// Only consecutive writes are supported
	if (address != nextWriteAddress)
		return cmd_result(Status::INVALID_ARGUMENTS);

	nextWriteAddress += len;
	while (address < nextWriteAddress) {
		writeBuffer[address % sizeof(writeBuffer)] = *data;
		++data;
		++address;

		if (address % sizeof(writeBuffer) == 0) {
			uint8_t err = commitToFlash(address - sizeof(writeBuffer), sizeof(writeBuffer));
			if (err) {
				dataout[0] = err;
				return cmd_result(Status::COMMAND_FAILED, 1);
			}
		}
	}

	return cmd_ok();
}

/**
 * @brief Get revision from datamatrix from OTP.
 * @return revision number (only VV field of datamatrix, no factorify ID)
 */
uint8_t get_revision() {
	uint8_t otp_v = *(uint8_t*)(OTP_START_ADDR);
	if (otp_v != 5)	{ // This understands only OTP v5
		return 0;
	}

	char *datamatrix = (char*)(OTP_START_ADDR + 8);
	if ((datamatrix[4] != '-')                            // Separator between factorify product ID and revision
		|| (datamatrix[5] < '0') || (datamatrix[5] > '9') // Revision is two decimal digits
		|| (datamatrix[6] < '0') || (datamatrix[6] > '9')) {
		return 0;
	}

	return (datamatrix[5] - '0') * 10 + (datamatrix[6] - '0');
}

/**
 * @brief Read FLASH or read OTP.
 * @param cmd either Commands::READ_FLASH or Commands::READ_OTP
 * @param datain input data
 * @param len number of bytes in datain
 * @param dataout response output data
 * @param maxLen max number of bytes in dataout
 * @return command result, possibly with number of used bytes in dataout
 */
cmd_result readMemory(uint8_t cmd, uint8_t *datain, uint8_t len, uint8_t *dataout, uint8_t maxLen) {
	if (len != 4+1)
		return cmd_result(Status::INVALID_ARGUMENTS);

	uint32_t address = datain[0] << 24 | datain[1] << 16 | datain[2] << 8 | datain[3];
	uint8_t readlen = datain[4];
	uint32_t memOffset;
	uint32_t memSize;

	if (cmd == Commands::READ_FLASH) {
		memSize = APPLICATION_SIZE;
		memOffset = FLASH_BASE + FLASH_APP_OFFSET;
	} else {
		memSize = OTP_SIZE;
		memOffset = OTP_START_ADDR;
	}

	if ((readlen > maxLen) || ((address + readlen) > memSize)) {
		return cmd_result(Status::INVALID_ARGUMENTS);
	}

	memcpy(dataout, (uint8_t*)(memOffset + address), readlen);
	return cmd_ok(readlen);
}

cmd_result processCommand(uint8_t cmd, uint8_t *datain, uint8_t len, uint8_t *dataout, uint8_t maxLen) {
	if (maxLen < 5)
		compiletime_check_failed();

	switch (cmd) {
		case Commands::GET_HARDWARE_INFO: {
			if (len != 0)
				return cmd_result(Status::INVALID_ARGUMENTS);

			const size_t hw_info_size = 11;
			if (maxLen < hw_info_size)
				compiletime_check_failed();

			// Type 42 dwarf or type 43 modular bed
			static_assert(sizeof(INFO_HW_TYPE) == sizeof(uint8_t), "INFO_HW_TYPE won't fit to 1 byte!");
			dataout[0] = INFO_HW_TYPE;

			// Hardware revision
			uint16_t hardware_revision = get_revision();
			dataout[1] = hardware_revision >> 8;
			dataout[2] = hardware_revision;

			// Bootloader version as a commit counter, MSB
			uint32_t bl_version = BL_VERSION;
			dataout[3] = bl_version >> 24;
			dataout[4] = bl_version >> 16;
			dataout[5] = bl_version >> 8;
			dataout[6] = bl_version;

			// Available flash size is up to startApplication, MSB
			uint32_t size = SelfProgram::applicationSize;
			dataout[7] = size >> 24;
			dataout[8] = size >> 16;
			dataout[9] = size >> 8;
			dataout[10] = size;

			return cmd_ok(hw_info_size);
		}

		case Commands::START_APPLICATION:
			if (len == 0) { // No fingerprint, need to check internal fingerprint
				bootloaderFingerprintMatch = false;
				bootloaderExit = true;
			} else if (len == (sizeof(SelfProgram::appFwFingerprintSalt) + sizeof(SelfProgram::appFwFingerprint))) { // Check with fingerprint that was already calculated
				if (((static_cast<uint32_t>(datain[0]) << 24 | datain[1] << 16 | datain[2] << 8 | datain[3]) == SelfProgram::appFwFingerprintSalt)
					&& (memcmp(SelfProgram::appFwFingerprint, &datain[4], sizeof(SelfProgram::appFwFingerprint)) == 0)) {
					bootloaderFingerprintMatch = true;
				}

				bootloaderExit = true;
			} else {
				return cmd_result(Status::INVALID_ARGUMENTS);
			}

			dataout[0] = bootloaderFingerprintMatch;  // Report if fingerprint calculation is skipped
			return cmd_ok(1);

		case Commands::WRITE_FLASH:
		{
			if (len < 4)
				return cmd_result(Status::INVALID_ARGUMENTS);

			uint32_t address = datain[0] << 24 | datain[1] << 16 | datain[2] << 8 | datain[3];
			return handleWriteFlash(address, datain + 4, len - 4, dataout);
		}
		case Commands::FINALIZE_FLASH:
		{
			if (len != 0)
				return cmd_result(Status::INVALID_ARGUMENTS);

			uint32_t pageAddress = nextWriteAddress & ~(sizeof(writeBuffer) - 1);
			uint8_t err = commitToFlash(pageAddress, nextWriteAddress - pageAddress);
			if (err) {
				dataout[0] = err;
				return cmd_result(Status::COMMAND_FAILED, 1);
			} else {
				dataout[0] = SelfProgram::eraseCount;
				SelfProgram::eraseCount = 0;
				return cmd_ok(1);
			}
		}
		case Commands::READ_FLASH:
		case Commands::READ_OTP:
			return readMemory(cmd, datain, len, dataout, maxLen);

		case Commands::GET_FINGERPRINT: {
			uint8_t offset = 0;
			uint8_t size = sizeof(SelfProgram::appFwFingerprint);
			if (len == 2) {	// Buddy wants only a chunk of the fingerprint
				offset = datain[0];
				size = datain[1];
				if ((offset + size) > sizeof(SelfProgram::appFwFingerprint)) {
					return cmd_result(Status::INVALID_ARGUMENTS);
				}
			} else if (len != 0) {
				return cmd_result(Status::INVALID_ARGUMENTS);
			}

			if (!SelfProgram::appFwFingerprintValid)
				return cmd_result(Status::COMMAND_FAILED);

			memcpy(dataout, &SelfProgram::appFwFingerprint[offset], size);

			return cmd_ok(size);
		}
		case Commands::COMPUTE_FINGERPRINT: {
			if (len != 4) {
				return cmd_result(Status::INVALID_ARGUMENTS);
			}

			SelfProgram::appFwFingerprintSalt = datain[0] << 24 | datain[1] << 16 | datain[2] << 8 | datain[3];
			
			SelfProgram::calculateSaltedFingerprint(SelfProgram::appFwFingerprintSalt);
			return cmd_ok();
		}

		default:
			return cmd_result(Status::COMMAND_NOT_SUPPORTED);
	}
}


#ifndef DISABLE_WATCHDOG /*Watchdog can be disabled by define in makefile*/

/**
 * @brief Start IWDG and configure watchdog time.
 */
static void watchdog_start() {
	// Freeze IWDG on debug
	#define DBGMCU_APB_FZ1               MMIO32(DBGMCU_BASE + 0x08) // Freeze register was not defined in opencm3
	#define DBGMCU_APB_FZ1_DBG_IWDG_STOP 0x00001000                 // IWDG freeze bit in DBGMCU_APB_FZ1 register
	rcc_periph_clock_enable(RCC_DBG);
	DBGMCU_APB_FZ1 |= DBGMCU_APB_FZ1_DBG_IWDG_STOP; // Stop IWDG when core is halted by debugger
	rcc_periph_clock_disable(RCC_DBG);

	// Start and unlock IWDG
	IWDG_KR = IWDG_KR_START;
	IWDG_KR = IWDG_KR_UNLOCK;

	// Configure IWDG
	IWDG_PR = 0x6;     // Slowest, approx 32 kHz / 256 = 125 Hz
	IWDG_RLR = 0xfff;  // Maximal, approx 2^12 / (~32 kHz / 256) = 32.768 s
	IWDG_WINR = 0xfff; // Window disabled

	// Cannot wait for IWDG update flags as buddy won't wait that long,
	// handle it in watchdog_reset() instead
}

/**
 * @brief Wait for the IWDG peripheral to finish configuration and reset IWDG.
 */
static void watchdog_reset() {
	static bool started = false;
	if (started) {
		IWDG_KR = IWDG_KR_RESET; // Reset watchdog
	} else if ((IWDG_SR & (IWDG_SR_WVU | IWDG_SR_RVU | IWDG_SR_PVU)) == 0x00u) {
		started = true;
	}
}

#else /*DISABLE_WATCHDOG*/
	// Dummy symbols when watchdog is disabled
	#define watchdog_start() do{}while(0)
	#define watchdog_reset() do{}while(0)
#endif /*DISABLE_WATCHDOG*/

// Active wait until power panic signal is disabled
void wait_for_end_of_power_panic() {
	// TODO: Suspend CPU wake up by interrupt.
#if defined(BOARD_TYPE_prusa_dwarf)
		Pin power_panic = {RCC_GPIOA, GPIOA, GPIO11};
#elif defined(BOARD_TYPE_prusa_modular_bed)
		Pin power_panic = {RCC_GPIOC, GPIOC, GPIO11};
#else
	#error "Unknown board"
#endif
	power_panic.hiz();
	// Power panic is active low, spin until it reads high
	while(!power_panic.read()) {
		watchdog_reset();
	}
}

// Used to read the FW_DESCRIPTOR section persistent data, used attribute is to make sure it's not optimized away
__attribute__((used)) const puppy_crash_dump::FWDescriptor * const fw_descriptor
	= reinterpret_cast<puppy_crash_dump::FWDescriptor *>(puppy_crash_dump::APP_DESCRIPTOR_OFFSET + FLASH_APP_OFFSET + 0x08000000 );

extern "C" {
	void runBootloader() {
		ClockInit();
		BusInit();

		// Configure watchdog
		watchdog_start();
		watchdog_reset();		

		// Avoid doing anything as long as power panic is active
		wait_for_end_of_power_panic();

		led::set_rgb(0, 0, 0x0f); // blue: bl is running

		// Turn on heatbreak fan to avoid heat spreading across heatbreak
#if defined(BOARD_TYPE_prusa_dwarf)
		rcc_periph_clock_enable(RCC_GPIOC);
		gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO6);
		gpio_set(GPIOC, GPIO6);
#endif

		bool busy = true;
		while (busy || !bootloaderExit) {
			#if !defined(BUS_USE_INTERRUPTS)
			busy = BusUpdate();
			#endif // defined(BUS_USE_INTERRUPTS)

			watchdog_reset();
		}

                //Check with unsalted fingerprint if necessary
		if (bootloaderFingerprintMatch == false) {
			bootloaderFingerprintMatch = SelfProgram::checkUnsaltedFingerprint(fw_descriptor->fingerprint); // Calculate and check match with fingerprint in descriptors
		}

		if (fw_descriptor->stored_type == puppy_crash_dump::FWDescriptor::StoredType::crash_dump
			|| !bootloaderFingerprintMatch
#if NEEDS_ADDRESS_CHANGE
			|| getConfiguredAddress() == INITIAL_ADDRESS
#endif
			){
			while(true) {
				led::set_rgb(0x0f, 0x08, 0x00); // orange: not safe to start
				watchdog_reset();
			}
		}

		led::set_rgb(0, 0x0f, 0x0f); // cyan: fw is about to start
		application_startup_arguments.modbus_address = getConfiguredAddress();
		BusDeinit();
		ClockDeinit();
	}
}
