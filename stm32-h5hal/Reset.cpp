#include "BaseProtocol.h"

#include "stm32h5xx_hal_cortex.h"

void resetSystem() {
    HAL_NVIC_SystemReset();
}
