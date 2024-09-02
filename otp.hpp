#pragma once

#include <cstdint>

// OTP area
#if defined(STM32G0)
static constexpr uint32_t OTP_START_ADDR = 0x1FFF7000UL;
static constexpr uint32_t OTP_SIZE = 1024;
#elif defined(STM32H5)
static constexpr uint32_t OTP_START_ADDR = 0x08FFF000UL;
static constexpr uint32_t OTP_SIZE = 0x800;
#else
#error "Missing OTP definition"
#endif

// Puppies are using OTP_V5 format
struct __attribute__((packed)) OTP_v5 {
    uint8_t version = 0; // Data structure version (1 bytes)
    uint16_t size = 0; // Data structure size (uint16_t little endian)
    uint8_t bomID = 0; // BOM ID (1 bytes)
    uint32_t timestamp = 0; // UNIX Timestamp from 1970 (uint32_t little endian)
    uint8_t datamatrix[24] = { 0 }; // DataMatrix ID 1 (24 bytes)
};

void read_otp(std::size_t offset, uint8_t* data, std::size_t len);

inline OTP_v5 get_OTP_data() {
    OTP_v5 data{};
    read_otp(0, (uint8_t *)&data, sizeof(data));
    return data;
}

