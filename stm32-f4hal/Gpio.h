#pragma once
#include "stm32f4xx.h"
#include "stm32f4xx_hal_gpio.h"

#if defined(BOARD_TYPE_prusa_baseboard)
    // UV LED
    #define D_UV_LED_ENABLE_Port GPIOD
    #define D_UV_LED_ENABLE_Pin GPIO_PIN_2
    // ID pins for baseboard variants
    #define D_SLX_ID_GPIO_Port GPIOA
    #define D_SLX_ID_Pin GPIO_PIN_12
    #define D_CX_ID_GPIO_Port GPIOG
    #define D_CX_ID_Pin GPIO_PIN_8
    #define D_WX_ID_GPIO_Port GPIOG
    #define D_WX_ID_Pin GPIO_PIN_9

#elif defined(BOARD_TYPE_prusa_smartled01)
    #define D_P5V_PWR_EN_Pin GPIO_PIN_9
    #define D_P5V_PWR_EN_GPIO_Port GPIOA // ON! (or blinking?)
    #define D_LED_EN_Pin GPIO_PIN_15
    #define D_LED_EN_GPIO_Port GPIOB // OFF!
    /*
     * TODO (Nemec): Add fans to be turned on during bootup!
     * FAN1
     * FAN2
    */
#else
    #error "Undefined pin definitopm"
#endif

#ifdef __cplusplus
extern "C" {
#endif
void gpio_init();

#ifdef __cplusplus
}
#endif
