#include <cstdint>

namespace led{
    /**
     * @brief Sets the colour of the WS2812B-V4 LED.
     */
    void set_rgb(uint8_t red, uint8_t green, uint8_t blue);
}