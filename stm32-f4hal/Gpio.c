#include "Gpio.h"

#include "stm32f4xx.h"
#include "stm32f4xx_hal_gpio.h"

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
        HAL_GPIO_WritePin(GPIOG, D_MCU_PWR_EN_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOG, D_LED_POWER_EN_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, D_LED_EN_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, D_POWER_OUT_EN_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOF, D_EXT_2_Pin, GPIO_PIN_RESET);

        GPIO_InitStruct.Pin = D_MCU_PWR_EN_Pin | D_LED_POWER_EN_Pin | D_SECOND_RESET_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = D_LED_EN_Pin | D_POWER_OUT_EN_Pin | D_LED_RESET_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = D_EXT_2_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

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

#if defined(BOARD_TYPE_prusa_baseboard)
    void reset_fellow_slaves() {
        HAL_GPIO_WritePin(D_SECOND_RESET_GPIO_Port, D_SECOND_RESET_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(D_LED_RESET_GPIO_Port, 	D_LED_RESET_Pin, 	GPIO_PIN_SET);
        HAL_Delay(100);
        HAL_GPIO_WritePin(D_SECOND_RESET_GPIO_Port, D_SECOND_RESET_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(D_LED_RESET_GPIO_Port, 	D_LED_RESET_Pin, 	GPIO_PIN_RESET);
    }

    void turn_smartled_on() {
        HAL_GPIO_WritePin(D_MCU_PWR_EN_GPIO_Port, D_MCU_PWR_EN_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(D_LED_EN_GPIO_Port, D_LED_EN_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(D_LED_POWER_EN_GPIO_Port, D_LED_POWER_EN_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(D_POWER_OUT_EN_GPIO_Port, D_POWER_OUT_EN_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(D_EXT_2_GPIO_Port, D_EXT_2_Pin, GPIO_PIN_RESET);
        reset_fellow_slaves();
    }
#endif
