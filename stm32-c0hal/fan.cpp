#include <stm32c0xx_ll_gpio.h>
#include <stm32c0xx_ll_bus.h>

#define FAN1_Pin LL_GPIO_PIN_0
#define FAN1_GPIO_Port GPIOB
#define FAN2_Pin LL_GPIO_PIN_1
#define FAN2_GPIO_Port GPIOB

void StartFan() {
    // Init fan pwm pins as generic output pins
    LL_GPIO_InitTypeDef GPIO_InitStruct{};

    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);
    LL_GPIO_ResetOutputPin(FAN1_GPIO_Port, FAN1_Pin);
    LL_GPIO_ResetOutputPin(FAN2_GPIO_Port, FAN2_Pin);

    GPIO_InitStruct.Pin = FAN1_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(FAN1_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = FAN2_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(FAN2_GPIO_Port, &GPIO_InitStruct);

    // Set fan pins to high
    LL_GPIO_SetOutputPin(FAN1_GPIO_Port, FAN1_Pin);
    LL_GPIO_SetOutputPin(FAN2_GPIO_Port, FAN2_Pin);
}
