#if defined(STM32H5)
    #include <stm32h5xx_ll_iwdg.h>
#elif defined(STM32C0)
    #include <stm32c0xx_ll_iwdg.h>
#elif defined(STM32F4)
    #include <stm32f4xx_ll_iwdg.h>
#else
    #error
#endif

void WatchdogStart() {
#ifndef DISABLE_WATCHDOG
    LL_IWDG_Enable(IWDG);                             // also starts low speed internal clock @ 32kHz
    LL_IWDG_EnableWriteAccess(IWDG);                  // enable writing prescaler, reload counter and window
    LL_IWDG_SetPrescaler(IWDG, LL_IWDG_PRESCALER_32); // 32 kHz -> 1kHz
    LL_IWDG_SetReloadCounter(IWDG, 1000);             // 1kHz -> 1s
    LL_IWDG_SetWindow(IWDG, 0xfff);                   // maximum value
    while (!LL_IWDG_IsReady(IWDG)) {}                 // busy wait, if we halt here we are dead anyway
    LL_IWDG_ReloadCounter(IWDG);                      // also disables write access
#endif
}

void WatchdogReset() {
#ifndef DISABLE_WATCHDOG
    LL_IWDG_ReloadCounter(IWDG);
#endif
}
