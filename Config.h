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

#ifndef CONFIG_H_
#define CONFIG_H_

#include <stdint.h>
#include <Gpio.h>

#if defined(BOARD_TYPE_prusa_dwarf)
	const uint8_t INFO_HW_TYPE = 42;
    const uint16_t MAX_PACKET_LENGTH = 255;
	const uint8_t INITIAL_ADDRESS = 0x00;
    #define SYSTEM_CORE_CLOCK 64000000
    #define NEEDS_ADDRESS_CHANGE 1
#elif defined(BOARD_TYPE_prusa_modular_bed)
	const uint8_t INFO_HW_TYPE = 43;
    const uint16_t MAX_PACKET_LENGTH = 255;
	const uint8_t INITIAL_ADDRESS = 0x00;
    #define SYSTEM_CORE_CLOCK 64000000
    #define NEEDS_ADDRESS_CHANGE 0
#else
	#error "No board type defined"
#endif

#endif /* CONFIG_H_ */
