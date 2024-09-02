#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/flash.h>


#include "../bootloader.h"


void ClockInit()
{
    // use internal oscilator, run on 64MHz
    rcc_clock_setup(&rcc_clock_config[RCC_CLOCK_CONFIG_HSI_PLL_64MHZ]);
}

void ClockDeinit()
{
    // switch to internal oscilator, and turn PLL off
	rcc_set_sysclk_source(RCC_HSI);
	rcc_wait_for_sysclk_status(RCC_HSI);
    rcc_osc_off(RCC_PLL);
    while (rcc_is_osc_ready(RCC_PLL));

    rcc_clock_setup(&rcc_clock_config[RCC_CLOCK_CONFIG_HSI_16MHZ]);
}

