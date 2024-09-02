#include "otp.hpp"
#include <cstring>

void read_otp(std::size_t offset, uint8_t* data, std::size_t len) {
    memcpy(data, (uint8_t *)OTP_START_ADDR + offset, len);
}
