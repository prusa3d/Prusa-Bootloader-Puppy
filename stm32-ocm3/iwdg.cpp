#include "iwdg.hpp"

void WatchdogStart() {
#ifndef DISABLE_WATCHDOG
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
#endif
}

void WatchdogReset() {
#ifndef DISABLE_WATCHDOG
	static bool started = false;
	if (started) {
		IWDG_KR = IWDG_KR_RESET; // Reset watchdog
	} else if ((IWDG_SR & (IWDG_SR_WVU | IWDG_SR_RVU | IWDG_SR_PVU)) == 0x00u) {
		started = true;
	}
#endif
}
