#pragma once

/**
 * @brief Start IWDG and configure watchdog time.
 */
void WatchdogStart();
/**
 * @brief Wait for the IWDG peripheral to finish configuration and reset IWDG.
 */
void WatchdogReset();
