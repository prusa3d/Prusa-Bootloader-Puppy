#pragma once
#include "stm32f4xx.h"
#include "stm32f4xx_hal_gpio.h"

#define D_MCU_PWR_EN_Pin GPIO_PIN_0
#define D_MCU_PWR_EN_GPIO_Port GPIOG
#define D_LED_POWER_EN_Pin GPIO_PIN_1
#define D_LED_POWER_EN_GPIO_Port GPIOG
#define D_LED_RESET_Pin GPIO_PIN_8
#define D_LED_RESET_GPIO_Port GPIOB
#define D_LED_EN_Pin GPIO_PIN_9
#define D_LED_EN_GPIO_Port GPIOB
#define D_EXT_2_Pin GPIO_PIN_10
#define D_EXT_2_GPIO_Port GPIOF
#define D_POWER_OUT_EN_Pin GPIO_PIN_12
#define D_POWER_OUT_EN_GPIO_Port GPIOB
#define D_SECOND_RESET_Pin GPIO_PIN_7
#define D_SECOND_RESET_GPIO_Port GPIOG


#ifdef __cplusplus
extern "C" {
#endif
void reset_fellow_slaves();
void gpio_init();
void turn_smartled_on();


#ifdef __cplusplus
}
#endif