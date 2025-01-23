#include "BaseProtocol.h"

#include "stm32f4xx_hal_cortex.h"

void resetSystem() {
    HAL_NVIC_SystemReset();
}
