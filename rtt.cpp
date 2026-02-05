#include "rtt.hpp"

// #define RTT_ENABLED

#ifdef RTT_ENABLED

    #warning "#undef RTT_ENABLED in release build"

struct RTTControlBlock {
    char id[16];
    int up_buffer_count;
    int down_buffer_count;
    // exactly one up buffer so just inline it here
    const char *up_buffer_name;
    char *up_buffer_data;
    unsigned up_buffer_size;
    unsigned up_buffer_write_offset;
    unsigned up_buffer_read_offset;
    unsigned up_buffer_flags;
    // no down buffer
};

static char up_buffer[1024];
static volatile RTTControlBlock rtt_control_block;

[[nodiscard]] static bool maybe_print_char(char c) {
    unsigned write_offset = rtt_control_block.up_buffer_write_offset + 1;
    if (write_offset == rtt_control_block.up_buffer_size) {
        write_offset = 0;
    }

    if (write_offset != rtt_control_block.up_buffer_read_offset) {
        *(rtt_control_block.up_buffer_data + rtt_control_block.up_buffer_write_offset) = c;
        rtt_control_block.up_buffer_write_offset = write_offset;
        return true;
    } else {
        return false;
    }
}

void rtt::init() {
    rtt_control_block.up_buffer_count = 1;
    rtt_control_block.down_buffer_count = 0;
    rtt_control_block.up_buffer_name = "";
    rtt_control_block.up_buffer_data = up_buffer;
    rtt_control_block.up_buffer_size = sizeof(up_buffer);
    rtt_control_block.up_buffer_write_offset = 0;
    rtt_control_block.up_buffer_read_offset = 0;
    rtt_control_block.up_buffer_flags = 0;
    auto id = rtt_control_block.id;
    *id++ = 'S';
    *id++ = 'E';
    *id++ = 'G';
    *id++ = 'G';
    *id++ = 'E';
    *id++ = 'R';
    *id++ = ' ';
    *id++ = 'R';
    *id++ = 'T';
    *id++ = 'T';
}

void rtt::print(char c) {
    while (!maybe_print_char(c)) {
        // TODO this was giving me infinite loop, so let's break away and loose chars for now.
        break;
    }
}

void rtt::print(const char *str) {
    for (;;) {
        char c = *str++;
        if (c)
            rtt::print(c);
        else
            break;
    }
}

void rtt::print(uint32_t num) {
    char buffer[12];
    int i = 0;
    do {
        buffer[i++] = '0' + (num % 10);
        num = num / 10;
    } while (num > 0);
    while (i) {
        rtt::print(buffer[--i]);
    }
}

    #if __cplusplus >= 201703L

void rtt::print(std::byte byte) {
    const auto v = (uint32_t)byte;
    const char digits = "0123456789abcdef";
    const char hi = digits[(v >> 4) & 0xf];
    const char lo = digits[(v >> 0) & 0xf];
    rtt::print(hi);
    rtt::print(lo);
}

    #endif

#else

void rtt::init() {}
void rtt::print(char) {}
void rtt::print(const char *) {}
void rtt::print(uint32_t) {}

    #if __cplusplus >= 201703L

void rtt::print(std::byte) {}

    #endif

#endif
