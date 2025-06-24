#include "bootloader.h"

// #include <stm32f4xx_ll_rcc.h>
// #include <stm32f4xx_ll_pwr.h>
// #include <stm32f4xx_ll_system.h>
// #include <stm32f4xx_ll_utils.h>
#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f427xx.h"
#include "stm32f4xx_hal_rcc.h"
#include "stm32f4xx_hal_rcc_ex.h"
#include "stm32f4xx_hal_flash.h"
#include "stm32f4xx_hal_flash_ex.h"

#if defined(BOARD_TYPE_prusa_baseboard)
    #define OSC_INIT_RCC_PLLM 6
    #define CLK_INIT_APB2CLKDivider RCC_HCLK_DIV4
#elif defined(BOARD_TYPE_prusa_smartled01)
    #define OSC_INIT_RCC_PLLM 12
    #define CLK_INIT_APB2CLKDivider RCC_HCLK_DIV2
#else
    #error "Undefined clock config"
#endif

void ClockInit() {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Configure the main internal regulator output voltage
     */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = OSC_INIT_RCC_PLLM;
    RCC_OscInitStruct.PLL.PLLN = 168;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        while(1);
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = CLK_INIT_APB2CLKDivider;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
    {
        while(1);
    }

    TPI->ACPR = HAL_RCC_GetHCLKFreq() / 2000000 - 1;
}

void ClockDeinit() {
}
