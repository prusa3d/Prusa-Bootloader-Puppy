#include "power_panic.hpp"

#include "iwdg.hpp"
#include "Gpio.h"

void WaitForEndOfPowerPanic() {
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
		WatchdogReset();
	}
}
