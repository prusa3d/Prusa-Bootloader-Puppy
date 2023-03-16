#include "led.hpp"

#if defined(BOARD_TYPE_prusa_dwarf)

#include "timing_precise.h"
#include "Gpio.h"
#include "libopencmsis/core_cm3.h"

namespace led {
constexpr auto time_short_ns { 220 }; // doc: ns 220 - 0:380/1:420
constexpr auto time_long_ns { 580 };  // doc: ns 580 - 1000
constexpr auto time_reset_us { 280 }; // doc: us 280+

/**
 * @brief custom pin handling because Gpio.h is not fast enough
 *
 */
constexpr enum rcc_periph_clken neo_clock{RCC_GPIOB};
constexpr uint32_t neo_port{GPIOB};
constexpr uint16_t neo_pin_mask{GPIO6};

FORCE_INLINE void fast_led_set(){
    GPIO_BSRR(neo_port) = neo_pin_mask;
}

FORCE_INLINE void fast_led_reset(){
    GPIO_BSRR(neo_port) = (neo_pin_mask << 16);
}

FORCE_INLINE void send_zero() {
    fast_led_set();
    timing_delay_cycles(timing_nanoseconds_to_cycles(time_short_ns));
    fast_led_reset();
    timing_delay_cycles(timing_nanoseconds_to_cycles(time_long_ns));
}

FORCE_INLINE void send_one() {
    fast_led_set();
    timing_delay_cycles(timing_nanoseconds_to_cycles(time_long_ns));
    fast_led_reset();
    timing_delay_cycles(timing_nanoseconds_to_cycles(time_short_ns));
}

FORCE_INLINE void send_reset() {
    fast_led_reset();
    timing_delay_cycles(timing_microseconds_to_cycles(time_reset_us));
}

#pragma GCC push_options
#pragma GCC optimize("O3")
void set_rgb(uint8_t red, uint8_t green, uint8_t blue) {
    //  Requires heavy optimizations because the timings are strict - all called functions need to be inlined, otherwise the pragma O3 won't be applied to them.
    const uint32_t col = (green << 16) | (red << 8) | blue; // concat the colors
    rcc_periph_clock_enable(neo_clock);
    gpio_mode_setup(neo_port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, neo_pin_mask);
    send_reset();
    __disable_irq();
    for (int i = 0; i < 24; ++i) {
        if (col & (1 << (23 - i))) { // if bit set (from highest bit to lowest)
            send_one();
        } else {
            send_zero();
        }
    }
    __enable_irq();
    rcc_periph_clock_disable(neo_clock);
}
};
#pragma GCC pop_options
#else
namespace led{
    void set_rgb([[maybe_unused]] uint8_t red,[[maybe_unused]] uint8_t green,[[maybe_unused]] uint8_t blue) {}
};
#endif
