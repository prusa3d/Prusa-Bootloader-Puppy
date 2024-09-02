/*
 * Copyright (C) 2021 (http://www.3devo.eu)
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
#include "../Config.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/gpio.h>
#include <stdio.h>
#include "../Bus.h"
#include "../BaseProtocol.h"

static const uint32_t BAUD_RATE = 230400;
static const uint32_t MAX_INTER_FRAME = 150; // us
static const uint32_t INTER_FRAME_BITS = (MAX_INTER_FRAME * BAUD_RATE + 1e6 - 1) / 1e6;

#if defined(BOARD_TYPE_prusa_dwarf)
	#define RS485_USART USART1
	#define RCC_RS485_USART RCC_USART1
	#define RST_RS485_USART RST_USART1

#elif defined(BOARD_TYPE_prusa_modular_bed)
	#define RS485_USART USART1
	#define RCC_RS485_USART RCC_USART1
	#define RST_RS485_USART RST_USART1
#else
	#error Unknown board
#endif



void BusInit() {

	/* Setup clocks & GPIO for USART */
	rcc_periph_clock_enable(RCC_RS485_USART);

	/* Setup USART parameters. */
	usart_set_baudrate(RS485_USART, BAUD_RATE);
	usart_set_databits(RS485_USART, 8);
	usart_set_parity(RS485_USART, USART_PARITY_NONE);
	usart_set_stopbits(RS485_USART, USART_CR2_STOPBITS_1);

	usart_set_mode(RS485_USART, USART_MODE_TX_RX);

	usart_set_rx_timeout_value(RS485_USART, INTER_FRAME_BITS);
	usart_enable_rx_timeout(RS485_USART);


#if defined(BOARD_TYPE_prusa_dwarf)
	// Enable Driver Enable on RTS pin
	USART_CR3(RS485_USART) |= USART_CR3_DEM;

	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_set_af(GPIOA, GPIO_AF1, GPIO9 | GPIO10 | GPIO12);
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9 | GPIO10 | GPIO12);
#elif defined(BOARD_TYPE_prusa_modular_bed)
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOD);
	// RX/TX is PA9/PA10, alternate function 1
	gpio_set_af(GPIOA, GPIO_AF1, GPIO9 | GPIO10);
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9 | GPIO10);

	// transmission enable (485-REDE) pin
	gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO6);
	gpio_set_output_options(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ,  GPIO6);
	gpio_clear(GPIOD, GPIO6);
#else
	#error unkown board
#endif

	/* Finally enable the USART. */
	usart_enable(RS485_USART);
}

void BusDeinit() {
	rcc_periph_reset_pulse(RST_RS485_USART);

#if defined(BOARD_TYPE_prusa_dwarf)
	gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO9 | GPIO10 | GPIO12);
	gpio_set_af(GPIOA, GPIO_AF0, GPIO9 | GPIO10 | GPIO12);
	rcc_periph_clock_disable(RCC_GPIOA);
#elif defined(BOARD_TYPE_prusa_modular_bed)
	gpio_set_af(GPIOB, GPIO_AF0, GPIO6 | GPIO7);
	gpio_mode_setup(GPIOB, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO6 | GPIO7);
	gpio_mode_setup(GPIOD, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO6);

	rcc_periph_clock_disable(RCC_GPIOB);
	rcc_periph_clock_disable(RCC_GPIOD);
#else
	#error unkown board
#endif

	rcc_periph_clock_disable(RCC_RS485_USART);
}

// For RS485, MAX_PACKET_LENGTH is defined including the address byte.
// For requests, the address is stored outside of busBuffer, so this
// buffer is 1 byte too long. However, for replies the adress is stored
// inside the buffer, so use the full MAX_PACKET_LENGTH anyway.
static uint8_t busBuffer[MAX_PACKET_LENGTH];
static uint8_t busBufferLen = 0;
static uint8_t busTxPos = 0;
static uint8_t busAddress = 0;
static_assert(MAX_PACKET_LENGTH < (1 << (sizeof(busBufferLen) * 8)), "Code needs changes for bigger packets");

enum State {
	StateIdle,
	StateRead,
	StateWrite
};

static State busState = StateIdle;

static bool matchAddress(uint8_t address) {
	return address == getConfiguredAddress();
}

bool BusUpdate() {
	// Uncomment this to enable debug prints in this function
	// Breaks Rs485 communication unless debug baudrate is 20x or so
	// faster than Rs485!
	#define printf(...) do {} while(0)
	uint32_t isr = USART_ISR(RS485_USART);

	/*
	static uint32_t prev_isr = 0;
	if (isr != prev_isr) {
		printf("isr: 0x%lx\n", isr);
		prev_isr = isr;
	}
	*/


	if (isr & USART_ISR_TXE && busState == StateWrite) {
		// TX register empty, writing data clears TXE
		printf("tx: %02x\n", (unsigned)busBuffer[busTxPos]);
		#if defined(BOARD_TYPE_prusa_modular_bed)
			gpio_set(GPIOD, GPIO6); // TE high to enable transmission
		#endif
		usart_send(RS485_USART, busBuffer[busTxPos++]);
		if (busTxPos >= busBufferLen)
		{
			#if defined(BOARD_TYPE_prusa_modular_bed)
				// wait for transmission complete, then clear the TE pin
				while (!(USART_ISR(RS485_USART) & USART_ISR_TC)){}
				gpio_clear(GPIOD, GPIO6); // TE low to enable receive
			#endif
			busState = StateIdle;
		}
		// TODO: Clear error flags and/or RTOF after TX?
	} else if (isr & USART_ISR_RXNE && busState != StateWrite) { // Received data

		// Reading data clears RXNE
		uint8_t data = usart_recv(RS485_USART);
		printf("rx: 0x%02x\n", (unsigned)data);

		if (busState == StateIdle) {
			busAddress = data;
			busState = StateRead;
			busBufferLen = 0;
		} else if (busBufferLen < sizeof(busBuffer)) {
			busBuffer[busBufferLen++] = data;
		} else {
			printf("rx ovf\n");
		}
	}
	if ((isr & (USART_ISR_RTOF)) && busState == StateRead) {
		// Sufficient silence after last RX byte
		printf("rxtimeout after %u bytes\n", busBufferLen);

		// This checks for errors that occurred during any byte
		// in the transfer
		bool rxok = true;
		if (isr & USART_ISR_PE) {
			printf("parity error during transfer\n");
			rxok = false;
		}
		if (isr & USART_ISR_FE ) {
			printf("framing error during transfer\n");
			rxok = false;
		}
		if (isr & USART_ISR_ORE) {
			printf("overrun error during transfer\n");
			rxok = false;
		}

		// Clear timeout flag and any errors
		USART_ICR(RS485_USART) = USART_ICR_RTOCF | USART_ICR_PECF | USART_ICR_FECF | USART_ICR_ORECF;

		bool matched = matchAddress(busAddress);
		printf("address 0x%x %smatched\n", busAddress, matched ? "" : "not ");

		// RX addressed to us, execute the callback and setup for a read.
		if (!rxok || busBufferLen == 0 || !matched) {
			busBufferLen = 0;
		} else {
			busBufferLen = BusCallback(busAddress, busBuffer, busBufferLen, sizeof(busBuffer));
		}
		if (busBufferLen) {
			busState = StateWrite;
			busTxPos = 0;
		} else {
			busState = StateIdle;
		}
	}
	if (busState == StateWrite) {
		USART_CR1(RS485_USART) |= USART_CR1_TXEIE;
		USART_CR1(RS485_USART) &= ~(USART_CR1_RXNEIE | USART_CR1_RTOIE);
	} else {
		USART_CR1(RS485_USART) &= ~USART_CR1_TXEIE;
		USART_CR1(RS485_USART) |= USART_CR1_RXNEIE | USART_CR1_RTOIE;
	}
	#undef printf

	return (busState != StateIdle);  //Return true if busy
}
