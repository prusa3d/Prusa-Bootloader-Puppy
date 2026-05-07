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

#include "bootloader.h"
#include "SelfProgram.h"
#include "fault_handlers.hpp"
#include "uart.hpp"
#include <stdio.h>


#if !defined(FLASH_BASE)
// https://github.com/libopencm3/libopencm3-examples/issues/224
#define FLASH_BASE                      (0x08000000U)
#endif

void startApplication() __attribute__((__noreturn__));
void startApplication() {
	const uint32_t *application = (uint32_t*)(FLASH_BASE + FLASH_APP_OFFSET);
	const uint32_t top_of_stack = application[0];
	const uint32_t reset_vector = application[1];

	asm("msr msp, %0; bx %1;" : : "r"(top_of_stack), "r"(reset_vector));
	__builtin_unreachable();
}

int main() {
	runBootloader();
	startApplication();
}
