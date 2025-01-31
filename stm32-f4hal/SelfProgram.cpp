#include <cstring>

#if 1 // Disable printf to save space
    #include <cstdio>
#else
    #define printf(...) do {} while(0)
#endif

#include "iwdg.hpp"
#include "SelfProgram.h"
#include <stm32f427xx.h>
#include <stm32f4xx_hal_flash.h>
#include <stm32f4xx_hal_flash_ex.h>

/** This is the real memory address where the application starts. The bootloader
 *  considers this to be addr=0.
**/
static const uint32_t application_start = FLASH_BASE + FLASH_APP_OFFSET;


// Output readable error message
void report_error(uint32_t err) {
    switch(err) {
        case HAL_FLASH_ERROR_RD:
            printf("Read Protection error\n");
            break;
        case HAL_FLASH_ERROR_PGS:
            printf("Programming Sequence error\n");
            break;
        case HAL_FLASH_ERROR_PGP:
            printf("Programming Parallelism error\n");
            break;
        case HAL_FLASH_ERROR_PGA:
            printf("Programming Alignment error\n");
            break;
        case HAL_FLASH_ERROR_WRP:
            printf("Write protection error\n");
            break;
        case HAL_FLASH_ERROR_OPERATION:
            printf("Operation error\n");
            break;
        default:
            printf("Unknown error\n");
            break;
    }
};


/** Erase the whole flash except the first sector containing bootloader */
uint8_t SelfProgram::eraseApplicationFlash() {
    HAL_FLASH_Unlock();

    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_FLAG_PGSERR);

    /*  One big erase for whole flash except the first sector containing bootloader.
        This will give us a clean slate to work with.

        This takes ~32s on F427.
    */

    uint32_t t0 = HAL_GetTick();
    uint32_t erase_error{0};
    printf("Erasing sector %d to %d of FLASH_BANK_1\n", 1, 11);

    FLASH_EraseInitTypeDef EraseInitStruct = {
        .TypeErase = FLASH_TYPEERASE_SECTORS,
        .Banks = FLASH_BANK_1,
        .Sector = 1,
        .NbSectors = 11,
    };
    if (HAL_FLASHEx_Erase(&EraseInitStruct, &erase_error) != HAL_OK) {
        printf("Erase failed: %d\n", erase_error);
        return 1;
    }
    printf("Erase successful, %u ms\n", HAL_GetTick() - t0);

    t0 = HAL_GetTick();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_FLAG_PGSERR);

    printf("Erasing BANK_2...\n");
    EraseInitStruct = {
        .TypeErase = FLASH_TYPEERASE_MASSERASE,
        .Banks = FLASH_BANK_2,
    };
    if (HAL_FLASHEx_Erase(&EraseInitStruct, &erase_error) != HAL_OK) {
        printf("Erase failed: %d\n", erase_error);
        return 1;
    }
    printf("Erase successful, %u ms\n", HAL_GetTick() - t0);

    HAL_FLASH_Unlock();
    return 0;
}

/** Sector will be erased by the first write at its beginning(first byte of the
 *  sector). Subsequent writes to the same sector do not need erasing(that would
 *  erase already written data).
 *
 *  Assumption is that data is written sequentially, at least within the same
 *  sector.
 *
 *  This allows “page” to be smaller than the sector size, provided it divides
 *  the sector size evenly (i.e., with no remainder).
 *
 * @param address Address(offset) to write to within the application section of
 *                the flash
 * @param data Data to write
 * @param len Length of data
 * @return 0 on success, 1 on failure
 *
 **/
uint8_t SelfProgram::writePage(uint32_t address, uint8_t *data, uint16_t len) {
    uint32_t flash_address = application_start + address;
    printf("SelfProgram::writePage(%08X, %d)\n", address,  len);
    printf("Real address: %08X\n", flash_address);

    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_FLAG_PGSERR);

    uint32_t err{0};

    // Start at the longest batch that can be written at once
    // Note: DOUBLEWORD(8 bytes) is not supported on our boards(needs high voltage)
    //       see RM0090 Rev 20 pg.85
    const size_t batch_size = 4; // 4 bytes per write(see stm32f4xx_hal_flash.c:154)

    // Write most of it in the largest batches possible(for speed)
    for(size_t i = 0; i < len - (len % batch_size); i += batch_size) {
        uint32_t batch = *reinterpret_cast<uint32_t*>(&data[i]);
        uint32_t current_flash_address = application_start + address + i;
        WatchdogReset();
        if (err = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, current_flash_address, batch) != HAL_OK) {
            printf("Write failed(batch=%d)\n", batch_size);
            printf("Address: %08X, Data: %08X\n", current_flash_address, batch);
            report_error(err);
            return 1;
        }
    }
    // Now write the remaining data byte-by-byte
    for(size_t i = len - (len % batch_size); i < len; ++i) {
        uint8_t byte = data[i];
        uint32_t current_flash_address = application_start + address + i;
        WatchdogReset();
        if (err = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, current_flash_address, byte) != HAL_OK) {
            printf("Write failed(byte)\n");
            printf("Address: %08X, Data: %08X\n", current_flash_address, byte);
            report_error(err);
            return 1;
        }
    }
    HAL_FLASH_Lock();

    // Invalidate the instruction & data cache
    __HAL_FLASH_DATA_CACHE_DISABLE();
    __HAL_FLASH_INSTRUCTION_CACHE_DISABLE();
    __HAL_FLASH_DATA_CACHE_RESET();
    __HAL_FLASH_INSTRUCTION_CACHE_RESET();
    __HAL_FLASH_INSTRUCTION_CACHE_ENABLE();
    __HAL_FLASH_DATA_CACHE_ENABLE();
    printf("Write successful\n");
    return 0;
}
