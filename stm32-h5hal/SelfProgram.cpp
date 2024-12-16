#include "SelfProgram.h"

#include <stm32h5xx_ll_icache.h>

#include <cstring>
#include "iwdg.hpp"

uint8_t SelfProgram::writePage(uint32_t address, uint8_t *data, uint16_t len) {
    static constexpr size_t WRITE_SPEED = 4 * 4; // Word is 32-bit (4 bytes) long

    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_FLAG_PGSERR);
    
    const bool first_bank = (address + FLASH_APP_OFFSET) < FLASH_BANK_SIZE;
    const auto bank = first_bank ? FLASH_BANK_1 : FLASH_BANK_2;
    const size_t sectors_per_bank = FLASH_BANK_SIZE / FLASH_SECTOR_SIZE;
    const auto sector = ((address + FLASH_APP_OFFSET) / FLASH_SECTOR_SIZE) % sectors_per_bank;
    FLASH_EraseInitTypeDef EraseInitStruct = {
        .TypeErase = FLASH_TYPEERASE_SECTORS,
        .Banks = bank,
        .Sector = sector,
        .NbSectors = 1,
    };

    uint32_t erase_error;
    if (HAL_FLASHEx_Erase(&EraseInitStruct, &erase_error) != HAL_OK) {
        return 1;
    }

    for (size_t i = 0; i < (len / WRITE_SPEED); ++i) {
        uint32_t flash_addr = address + (i * WRITE_SPEED) + FLASH_BASE + FLASH_APP_OFFSET;
        uint32_t data_addr = (uint32_t)(&data[i * WRITE_SPEED]);

        WatchdogReset();

        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD, flash_addr, data_addr) != HAL_OK) {
              return 1;
        }
    }
    if (len % WRITE_SPEED != 0) {
        uint8_t bytes[WRITE_SPEED];
        memset(bytes, 0, WRITE_SPEED);
        memcpy(bytes, data, len % WRITE_SPEED);

        uint32_t flash_addr = address + ((len / WRITE_SPEED) * WRITE_SPEED) + FLASH_BASE + FLASH_APP_OFFSET;
        uint32_t data_addr = (uint32_t)(&bytes);

        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD, flash_addr, data_addr) != HAL_OK) {
            return 1;
        }
    }

    HAL_FLASH_Lock();
    LL_ICACHE_Invalidate();
    return 0;
}

