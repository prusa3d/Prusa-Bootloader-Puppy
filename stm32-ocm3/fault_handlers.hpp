#pragma once

#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/g0/flash.h>

extern "C" {
	// Ensure that any faults reset the system, rather than blocking
	// in a loop. This ensures the board can be addressed even when
	// a fault happens
	void hard_fault_handler() { scb_reset_system(); }
	void mem_manage_handler() __attribute__((alias("hard_fault_handler")));
	void bus_fault_handler() __attribute__((alias("hard_fault_handler")));
	void usage_fault_handler() __attribute__((alias("hard_fault_handler")));

	/**
	 * @brief Non-Maskable Interrupt.
	 * Usually means non-recoverable hardware error.
	 */
	void nmi_handler() {
		// When there is an error in flash, ECC will trigger an NMI interrupt
		uint32_t eccr = FLASH_ECCR;
		FLASH_ECCR = eccr;            // Clear interrupt flags
		if (eccr & FLASH_ECCR_ECCD) { // ECC detection that could not be corrected

			// Get address of the error, starts by 0 without the 0x08000000 offset
			uint32_t fault_address = ((eccr & FLASH_ECCR_ADDR_ECC_MASK) >> FLASH_ECCR_ADDR_ECC_SHIFT) * 8;
			if (fault_address > FLASH_APP_OFFSET) // Error is in application space
			{
				flash_unlock();
				flash_clear_status_flags();
				flash_erase_page(fault_address / FLASH_ERASE_SIZE); // Erase the offending page
				flash_lock();
			} else // Error is in bootloader, no way to fix this
			{
				while (1);
			}
		}

		scb_reset_system();
	}
}

