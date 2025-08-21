#include "Gpio.h"

#include "stm32f4xx.h"
#include "stm32f4xx_hal_gpio.h"
#include <stdint.h>

extern uint8_t info_hw_type;

void gpio_init() {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    #if defined(BOARD_TYPE_prusa_baseboard)

        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Pin = D_UV_LED_ENABLE_Pin;
        HAL_GPIO_Init(D_UV_LED_ENABLE_Port, &GPIO_InitStruct);

        // ID pins for baseboard variants
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Pin = D_SLX_ID_Pin;
        HAL_GPIO_Init(D_SLX_ID_GPIO_Port, &GPIO_InitStruct);

        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Pin = D_CX_ID_Pin;
        HAL_GPIO_Init(D_CX_ID_GPIO_Port, &GPIO_InitStruct);

        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Pin = D_WX_ID_Pin;
        HAL_GPIO_Init(D_WX_ID_GPIO_Port, &GPIO_InitStruct);

        HAL_GPIO_WritePin(D_UV_LED_ENABLE_Port, D_UV_LED_ENABLE_Pin, GPIO_PIN_RESET); // Turn off UV LED by default

        // Determine the baseboard type based on ID pins
        GPIO_PinState is_slx = HAL_GPIO_ReadPin(D_SLX_ID_GPIO_Port, D_SLX_ID_Pin);
        GPIO_PinState is_cx = HAL_GPIO_ReadPin(D_CX_ID_GPIO_Port, D_CX_ID_Pin);
        GPIO_PinState is_wx = HAL_GPIO_ReadPin(D_WX_ID_GPIO_Port, D_WX_ID_Pin);

        if (is_slx == GPIO_PIN_RESET) {
            info_hw_type = 53; // Baseboard SLX variant
        } else if (is_cx == GPIO_PIN_RESET) {
            info_hw_type = 54; // Baseboard CX variant
        } else if (is_wx == GPIO_PIN_RESET) {
            info_hw_type = 55; // Baseboard WX variant
        } else {
            info_hw_type = 51; // Baseboard unknown, use default
        }



    #elif defined(BOARD_TYPE_prusa_smartled01)
        HAL_GPIO_WritePin(GPIOB, D_LED_EN_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOA, D_P5V_PWR_EN_Pin, GPIO_PIN_RESET);

        GPIO_InitStruct.Pin = D_LED_EN_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = D_P5V_PWR_EN_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        // TODO: Add fans!

    #endif
}
