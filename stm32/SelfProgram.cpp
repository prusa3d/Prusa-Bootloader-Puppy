/*
 * Copyright (C) 2015-2017 Erin Tomson <erin@rgba.net>
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

#include <cstring>
#include <libopencm3/stm32/flash.h>
#include "../SelfProgram.h"
#include "../sha256.h"

// Writing happens per row
#if FLASH_WRITE_SIZE != 256
#error "Incorrect FLASH_WRITE_SIZE"
#endif

#if FLASH_ERASE_SIZE % FLASH_WRITE_SIZE != 0
#error "Incorrect FLASH_ERASE_SIZE"
#endif

#if FLASH_APP_OFFSET % FLASH_ERASE_SIZE != 0
#error "Incorrect FLASH_APP_OFFSET"
#endif

uint8_t SelfProgram::eraseCount = 0;
bool SelfProgram::appFwFingerprintValid = false;

unsigned char SelfProgram::appFwFingerprint[32] = {0};
uint32_t SelfProgram::appFwFingerprintSalt = 0;

void SelfProgram::readFlash(uint32_t address, uint8_t *data, uint16_t len) {
	for (uint8_t i=0; i < len; i++) {
		data[i] = readByte(address + i);
	}
}

uint8_t SelfProgram::readByte(uint32_t address) {
	uint8_t *ptr = (uint8_t*)FLASH_BASE + FLASH_APP_OFFSET + address;
	return *ptr;
}

// Ensure that this code runs from RAM by putting into the .data
// section, because during fast programming, no flash read accesses can
// happen. This also needs noinline to ensure this code is not inlined
// into a function running from flash, and this function cannot call
// other functions that run from flash, so it is a bit more hardcoded
// that it could be.
__attribute__(( __section__(".ramtext"), __noinline__ ))
static void flash_program_row(uint32_t address, uint8_t *data, uint16_t len) {
	#if !defined(STM32G0)
	#warning "Fast programming code written for G0, might not work on other series"
	#endif

	// Wait for previous operations (just in case)
	while ((FLASH_SR & FLASH_SR_BSY) == FLASH_SR_BSY);

	// Enable fast programming.
	FLASH_CR |= FLASH_CR_FSTPG;

	for (uint16_t i = 0; i < 256; i += 4) {
		auto make_word_le = [](uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
			return (uint32_t)a << 0 | (uint32_t)b << 8 | (uint32_t)c << 16 | (uint32_t)d << 24;
		};
		uint32_t value = make_word_le(
			i + 0 < len ? data[i + 0] : 0xff,
			i + 1 < len ? data[i + 1] : 0xff,
			i + 2 < len ? data[i + 2] : 0xff,
			i + 3 < len ? data[i + 3] : 0xff);

		// Program each word in turn
		MMIO32(FLASH_BASE + FLASH_APP_OFFSET + address + i) = value;
	}

	// Wait for completion
	while ((FLASH_SR & FLASH_SR_BSY) == FLASH_SR_BSY);

	// Disable fast programming again
	FLASH_CR &= ~(FLASH_CR_FSTPG);

	// Invalidate fingerprint as the flash has just changed
	SelfProgram::appFwFingerprintValid = false;
}

uint8_t SelfProgram::writePage(uint32_t address, uint8_t *data, uint16_t len) {
	// Can only write to a row boundary
	if (!len || address % FLASH_WRITE_SIZE != 0 || len > FLASH_WRITE_SIZE) {
		return 1;
	}

	// If the address is past the application section don't write anything
	if (address + len > applicationSize) {
		return 3;
	}

	flash_unlock();
	flash_clear_status_flags();

	// If we are the beginning of a page, erase it
	if (address % FLASH_ERASE_SIZE == 0) {
		if (eraseCount < 0xff)
			++eraseCount;
		flash_erase_page((address + FLASH_APP_OFFSET) / FLASH_ERASE_SIZE);
	}

	// If no errors from erase, then program
	if (FLASH_SR == 0)
		flash_program_row(address, data, len);
	flash_lock();


	// Get and clear error flags
	uint32_t errbits = FLASH_SR & 0xffff;
	FLASH_SR = 0xffff;

	// This uses the lower nibble to indicate the first
	// (least-significant) error bit set, and the upper nibble to
	// indicate the number of error bits set (just in case there are
	// multiple). For example, if just PROGERR (bit 3) is set, this
	// would result in 0x13. This leaves values < 16 free to be used
	// above.
	uint8_t res = 0;
	while (errbits) {
		if (errbits & 1)
			res += 0x10;
		else if ((res & 0xf0) == 0)
			++res;
		errbits >>= 1;
	}

	// Invalidate fingerprint as the flash has just changed
	SelfProgram::appFwFingerprintValid = false;

	return res;
}

void SelfProgram::calculateSaltedFingerprint(uint32_t salt) {
    uintptr_t start = FLASH_BASE + FLASH_APP_OFFSET;
    size_t size = applicationSize;

	//Helper to free sha context on return
    struct Context {
        mbedtls_sha256_context ctx;
        Context() {
            mbedtls_sha256_init(&ctx);
        }
        ~Context() {
            mbedtls_sha256_free(&ctx);
        }
    };
    Context context;

    if (mbedtls_sha256_starts_ret(&context.ctx, false) != 0) {
        return;
    }

    if (mbedtls_sha256_update_ret(&context.ctx, reinterpret_cast<uint8_t *>(&salt), sizeof(salt)) != 0) { // Salt
        return;
    }

    if (mbedtls_sha256_update_ret(&context.ctx, (const unsigned char *)start, size) != 0) { // Firmware
        return;
    }

    if (mbedtls_sha256_finish_ret(&context.ctx, appFwFingerprint) != 0) {
        return;
    }

    appFwFingerprintValid = true;
}

bool SelfProgram::checkUnsaltedFingerprint(const unsigned char fingerprint[32])
{
	uintptr_t start = FLASH_BASE + FLASH_APP_OFFSET;
	size_t size = applicationSize - FW_DESCRIPTOR_SIZE; // this is size of entire application flash area without the fw descriptor
	unsigned char calculatedFingerprint[32] = {0};

	if (mbedtls_sha256_ret((const unsigned char *)start, size, calculatedFingerprint, false) != 0) {
		return false;	// Couldn't check = check failed
	}

	return (memcmp(calculatedFingerprint, fingerprint, sizeof(calculatedFingerprint)) == 0);
}