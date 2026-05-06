/// @file

/// *preboot* is a short program contained in the first sector of the FLASH,
/// which prevents the board from being bricked by bootloader update.

#include <cstdint>
#include <stm32c0xx.h>

extern "C" const uint32_t bootloader_start[];  /// defined by linker script
extern "C" const uint32_t application_start[]; /// defined by linker script

#define NOINLINE __attribute__((noinline))

/// Keep the function separate from compute_crc() for better codegen. Avoid
/// using std::span for the same reason. The function fits into two 64-bit
/// instruction cachelines of the STM32C0's FLASH peripheral, so we do not
/// need to move it to SRAM to avoid CPU fighting with CRC over FLASH.
[[nodiscard]] static NOINLINE uint32_t compute_crc_internal(const uint32_t *first, const uint32_t *last) {
    while (first != last) {
        WRITE_REG(CRC->DR, *first++);
    }
    return READ_REG(CRC->DR);
}

/// This function uses hardware-acceleration for computing CRC-32. To avoid
/// confusing bootloader/application, we make sure to reset it after usage.
[[nodiscard]] static uint32_t compute_crc(const uint32_t *first, const uint32_t *last) {
    // reset
    SET_BIT(RCC->AHBRSTR, RCC_AHBRSTR_CRCRST);
    CLEAR_BIT(RCC->AHBRSTR, RCC_AHBRSTR_CRCRST);

    // enable
    SET_BIT(RCC->AHBENR, RCC_AHBENR_CRCEN);
    (void)READ_REG(RCC->AHBENR); // dummy read to enforce delay

    // compute
    const uint32_t result = compute_crc_internal(first, last);

    // disable
    CLEAR_BIT(RCC->AHBENR, RCC_AHBENR_CRCEN);

    // reset
    SET_BIT(RCC->AHBRSTR, RCC_AHBRSTR_CRCRST);
    CLEAR_BIT(RCC->AHBRSTR, RCC_AHBRSTR_CRCRST);

    // done
    return result;
}

extern "C" [[noreturn]] void preboot_jump(const uint32_t *vector_table);

extern "C" [[noreturn]] void preboot_reset_handler() {
    if (compute_crc(bootloader_start, application_start) == 0) {
        // If CRC matches, bootloader was flashed correctly:
        //  * factory always flashes correctly
        //  * application attempted bootloader update and was successful
        // In either case, it is safe to enter bootloader.
        preboot_jump(bootloader_start);
    } else {
        // If CRC doesn't match, bootloader was not flashed correctly.
        // Since factory always flashes correctly, there must be a valid
        // application that attempted to update bootloader and failed.
        // Therefore it is safe to enter application, which will eventually
        // update the bootloader to a valid state.
        preboot_jump(application_start);
    }
}

extern "C" [[noreturn]] void preboot_default_handler() {
    NVIC_SystemReset();
}
