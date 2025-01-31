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

#ifndef SELFPROGRAM_H_
#define SELFPROGRAM_H_

#include <stdint.h>
#include "Config.h"

void startApplication();

class SelfProgram {
public:

	/**
	 * @brief Erase the application flash area
	 *
	 * Called before writing the application to ensure that the flash is empty.
	 *
	 * __weak default implementation does nothing. Override if required.
	 *
	 * @return 0 on success, 1 on failure
	 */
	static uint8_t eraseApplicationFlash();

	static void readFlash(uint32_t address, uint8_t *data, uint16_t len);

	static uint8_t readByte(uint32_t address);

	static uint8_t writePage(uint32_t address, uint8_t *data, uint16_t len);

	/**
	 * @brief Calculate salted fingerprint of the application.
	 * @param salt 4 B salt added before the application, in this situation 4 B salt should be enough
	 * This fingerprint encompasses the entire application area.
	 * It is used as proof of original puppy to buddy.
	 * Result is stored in internal variables appFwFingerprintValid, appFwFingerprint and appFwFingerprintSalt.
	 */
	static void calculateSaltedFingerprint(uint32_t salt);

	/**
	 * @brief Calculate static fingerprint of the application.
	 * This fingerprint excludes the application descriptors.
	 * It is used to verify application before running it.
	 * @param fingerprint check the app with this fingerprint
	 * @return true to run the application, false if corruption is detected
	 */
	static bool checkUnsaltedFingerprint(const unsigned char fingerprint[32]);

	#if defined(NEED_TRAMPOLINE)
	static void writeTrampoline(uint16_t instruction);

	static uint16_t offsetRelativeJump(uint16_t instruction, int16_t offset);

	static uint16_t trampolineStart;

	// Use a reference to make this an alias to trampolineStart for
	// readability
	static constexpr const uint16_t& applicationSize = trampolineStart;
	#else
	static constexpr const uint32_t applicationSize = APPLICATION_SIZE;
	#endif // defined(NEED_TRAMPOLINE)

	static uint8_t eraseCount;
	static bool appFwFingerprintValid;
	static unsigned char appFwFingerprint[32];
	static uint32_t appFwFingerprintSalt;
};

#endif /* SELFPROGRAM_H_ */
