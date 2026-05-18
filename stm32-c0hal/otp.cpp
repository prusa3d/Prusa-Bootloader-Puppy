#include "otp.hpp"

#include <cstring>

void read_otp(std::size_t offset, uint8_t* data, std::size_t len) {
    // Unlike STM32H5, STM32C0
    //  * does not support unaligned access to memory
    //  * supports reading OTP by bytes
    // So, we can just memcpy here.
    memcpy(data, (const uint8_t *)(OTP_START_ADDR + offset), len);
}
