#include "otp.hpp"

// FIXME: Make better otp read algorithm
void read_otp(std::size_t offset, uint8_t* data, std::size_t len) {
    if (offset % 2 == 0 && len % 2 == 0) {
        uint16_t *bigger_data = (uint16_t *)data;
        for (std::size_t i = 0; i < len/2; ++i) {
            bigger_data[i] = *(uint16_t *)(OTP_START_ADDR + offset + (i*2));
        }
    }
}
