#include "SelfProgram.h"

#include <cstring>
#include "iwdg.hpp"

uint8_t SelfProgram::writePage(uint32_t address, uint8_t *data, uint16_t len) {
    static constexpr size_t WRITE_SPEED = 2 * 4; // Word is 32-bit (4 bytes) long

    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_FLAG_PGSERR);
    
    // We are able to flash smaller chunks that whole page, so only erase the page if we are at the start of it
    if (address % FLASH_PAGE_SIZE == 0) {
        const auto page = (address + FLASH_APP_OFFSET) / FLASH_PAGE_SIZE;
        FLASH_EraseInitTypeDef EraseInitStruct = {
            .TypeErase = FLASH_TYPEERASE_PAGES,
            .Page = page,
            .NbPages = 1,
        };

        uint32_t erase_error;
        if (HAL_FLASHEx_Erase(&EraseInitStruct, &erase_error) != HAL_OK) {
            return 1;
        }
    }

    size_t i = 0;
    for (; i < (len / WRITE_SPEED); ++i) {
        uint32_t flash_addr = address + (i * WRITE_SPEED) + FLASH_BASE + FLASH_APP_OFFSET;
        uint64_t data_to_write = 0;
        memcpy(&data_to_write, &data[i * WRITE_SPEED], WRITE_SPEED);

        WatchdogReset();

        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, flash_addr, data_to_write) != HAL_OK) {
            return 1;
        }
    }
    if (len % WRITE_SPEED != 0) {
        uint64_t data_to_write = 0;
        memcpy(&data_to_write, &data[i * WRITE_SPEED], len % WRITE_SPEED);

        uint32_t flash_addr = address + ((i + 1) * WRITE_SPEED) + FLASH_BASE + FLASH_APP_OFFSET;

        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, flash_addr, data_to_write) != HAL_OK) {
            return 1;
        }
    }

    HAL_FLASH_Lock();
    return 0;
}
