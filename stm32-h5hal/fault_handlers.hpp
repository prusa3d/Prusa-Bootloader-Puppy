#pragma once

#include "stm32h5xx_hal_flash.h"
#include "stm32h5xx_hal_cortex.h"

extern "C" {
    void HardFault_Handler() {
        // reset MCU
        HAL_NVIC_SystemReset();
    }

    void MemManage_Handler() __attribute__((alias("HardFault_Handler")));
    void BusFault_Handler() __attribute__((alias("HardFault_Handler")));
    void UsageFaultHandler() __attribute__((alias("HardFault_Handler")));

    void NMI_Handler() {
        /* TODO: This needs to be tested somehow
        const auto error = HAL_FLASH_GetError();
        if (error & HAL_FLASH_ERROR_ECCD) {
            const auto fault_address = FLASH->ECCDETR & FLASH_ECCR_ADDR_ECC;
            const auto bank = FLASH->ECCDETR & FLASH_ECCR_BK_ECC;
            if (fault_address > FLASH_APP_OFFSET || bank != 0) {
                [[maybe_unused]] uint32_t erase_error;
                FLASH_EraseInitTypeDef erase_def {
                    .TypeErase = FLASH_TYPEERASE_SECTORS,
                    .Banks = (bank == 0)? FLASH_BANK_1: FLASH_BANK_2,
                    .Sector = fault_address / FLASH_SECTOR_SIZE,
                    .NbSectors = 1,
                };
                HAL_FLASH_Unlock();
                __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ECCC | FLASH_FLAG_ECCD);
                HAL_FLASHEx_Erase(&erase_def, &erase_error);
                HAL_FLASH_Lock();
            }
        }*/
        HardFault_Handler();
    }

    void SysTick_Handler() {
        HAL_IncTick();
    }

}
