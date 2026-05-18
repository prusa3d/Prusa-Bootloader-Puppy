#include "BaseProtocol.h"

#if defined(STM32H5)
#include "stm32h5xx_hal_cortex.h"
#elif defined(STM32C0)
#include "stm32c0xx_hal_cortex.h"
#elif defined(STM32F4)
#include "stm32f4xx_hal_cortex.h"
#else
#error
#endif

void resetSystem() {
    HAL_NVIC_SystemReset();
}
