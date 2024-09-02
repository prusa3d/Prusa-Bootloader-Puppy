#include "fan.hpp"

#include "Gpio.h"

#include <libopencm3/stm32/desig.h>

void StartFan() {
#if defined(BOARD_TYPE_prusa_dwarf)
    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO6);
    gpio_set(GPIOC, GPIO6);
#endif
}
