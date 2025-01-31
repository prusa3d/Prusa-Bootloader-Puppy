#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "stm32f427xx.h"

int _write(int file, char *ptr, int len)
{
    for (int i = 0; i < len; i++)
    {
        ITM_SendChar(*ptr++);
    }
    return len;
}
