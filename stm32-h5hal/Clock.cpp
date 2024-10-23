#include "bootloader.h"

#include <stm32h5xx_ll_rcc.h>
#include <stm32h5xx_ll_pwr.h>
#include <stm32h5xx_ll_system.h>
#include <stm32h5xx_ll_utils.h>

void ClockInit() {
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_3);
    while(LL_FLASH_GetLatency()!= LL_FLASH_LATENCY_3) { }

    LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE3);
    while (LL_PWR_IsActiveFlag_VOS() == 0) { }
    LL_RCC_HSI_Enable();

     /* Wait till HSI is ready */
    while(LL_RCC_HSI_IsReady() != 1) { }

    LL_RCC_HSI_SetCalibTrimming(64);
    LL_RCC_HSI_SetDivider(LL_RCC_HSI_DIV_2);
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);

     /* Wait till System clock is ready */
    while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI) { }

    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
    LL_RCC_SetAPB3Prescaler(LL_RCC_APB3_DIV_1);
    LL_SetSystemCoreClock(64000000);

     /* Update the time base */
    HAL_InitTick (TICK_INT_PRIORITY);
}

void ClockDeinit() {
}
