#pragma once

#include <cstddef>
#include <cstdint>

namespace rtt {

/// Initialize RTT subsystem. Call this once.
void init();

/// Print character via RTT subsystem.
void print(char);

/// Print string via RTT subsystem.
void print(const char *);

/// Print integer as decimal via RTT subsystem.
void print(uint32_t);

#if __cplusplus >= 201703L

/// Print byte as hexadecimal via RTT subsystem.
void rtt::print(std::byte);

#endif

}
