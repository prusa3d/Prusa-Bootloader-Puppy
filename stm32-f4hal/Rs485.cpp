#include "Bus.h"
#include "BaseProtocol.h"
#include "Config.h"

#include <stm32f4xx_hal.h>
#include <stm32f4xx_ll_gpio.h>
#include <stm32f4xx_ll_usart.h>
#include <stm32f4xx_ll_rcc.h>
#include <stm32f4xx_ll_bus.h>

#include "stm32f427xx.h"

#include <cstdint>
#include <cstring>
#include <cassert>

#if defined(BOARD_TYPE_prusa_baseboard)
    #include "Gpio.h"
    #define D_RS485_FLOW_CONTROL_Pin LL_GPIO_PIN_4
    #define D_RS485_FLOW_CONTROL_GPIO_Port GPIOD
    #define USART_CHANNEL USART2
    #define D_RS485_TX_Pin LL_GPIO_PIN_5
    #define D_RS485_RX_Pin LL_GPIO_PIN_6
    #define APB_BUS_CLOCK_ENABLE LL_APB1_GRP1_PERIPH_USART2
    #define AHB_BUS_GPIO_Port LL_AHB1_GRP1_PERIPH_GPIOD
#elif defined(BOARD_TYPE_prusa_smartled01)
    #include "Gpio.h"
    #define D_RS485_FLOW_CONTROL_Pin LL_GPIO_PIN_12
    #define D_RS485_FLOW_CONTROL_GPIO_Port GPIOC
    #define USART_CHANNEL USART3
    #define D_RS485_TX_Pin LL_GPIO_PIN_10
    #define D_RS485_RX_Pin LL_GPIO_PIN_11
    #define APB_BUS_CLOCK_ENABLE LL_APB1_GRP1_PERIPH_USART3
    #define AHB_BUS_GPIO_Port LL_AHB1_GRP1_PERIPH_GPIOC
#else
    #error "Undefined modbus channel and flow control gpio"
#endif

void BusInit() {
    gpio_init();
    LL_GPIO_InitTypeDef GPIO_InitStruct{};
    UART_HandleTypeDef USART_InitStruct{};


    // WIP: Can't find which clock source is used for USART2, if any(non-default)
    // LL_RCC_SetUSARTClockSource(LL_RCC_USART2_CLKSOURCE_PCLK1);

    /* Peripheral clock enable */
    LL_APB1_GRP1_EnableClock(APB_BUS_CLOCK_ENABLE);
    LL_AHB1_GRP1_EnableClock(AHB_BUS_GPIO_Port);

    /**USART2 GPIO Configuration
    PD5   ------> USART3_TX
    PD6   ------> USART3_RX
    */
    GPIO_InitStruct.Pin = D_RS485_TX_Pin | D_RS485_RX_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_7; // In case of other ports this could be possibly changed.
    LL_GPIO_Init(D_RS485_FLOW_CONTROL_GPIO_Port, &GPIO_InitStruct);

    /* Setup USART2_RTS */
    memset(&GPIO_InitStruct, 0, sizeof(GPIO_InitStruct));
    GPIO_InitStruct.Pin = D_RS485_FLOW_CONTROL_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(D_RS485_FLOW_CONTROL_GPIO_Port, &GPIO_InitStruct);
    LL_GPIO_ResetOutputPin(D_RS485_FLOW_CONTROL_GPIO_Port, D_RS485_FLOW_CONTROL_Pin);

    /* USART2 setup*/
    USART_InitStruct.Instance = USART_CHANNEL;
    USART_InitStruct.Init.BaudRate = 230400;
    USART_InitStruct.Init.WordLength = UART_WORDLENGTH_8B;
    USART_InitStruct.Init.StopBits = UART_STOPBITS_1;
    USART_InitStruct.Init.Parity = UART_PARITY_NONE;
    USART_InitStruct.Init.Mode = UART_MODE_TX_RX;
    USART_InitStruct.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    USART_InitStruct.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&USART_InitStruct) != HAL_OK)
    {
        assert(false);
    }

    // Note: STM32F427 does not support Rx timeout and FIFO.
    // TODO: Timeout is implemented in BusUpdate() function.
    LL_USART_ConfigAsyncMode(USART_CHANNEL);
    LL_USART_Enable(USART_CHANNEL);

    #if defined(BOARD_TYPE_prusa_baseboard)
        turn_smartled_on();
    #endif
}

void BusDeinit() {
    LL_USART_Disable(USART_CHANNEL);
    LL_USART_DeInit(USART_CHANNEL);
}

static uint8_t busBuffer[MAX_PACKET_LENGTH];
static uint8_t busBufferLen = 0;
static uint8_t busTxPos = 0;
static uint8_t busAddress = 0;
static_assert(MAX_PACKET_LENGTH < (1 << (sizeof(busBufferLen) * 8)), "Code needs changes for bigger packets");

enum State {
	StateIdle,
	StateRead,
	StateWrite
};

static State busState = StateIdle;
/// Idle counter
static uint32_t idle_ctr = 0;

static bool matchAddress(uint8_t address) {
	return address == getConfiguredAddress();
}
static volatile uint32_t bytes_received{0};
static volatile uint32_t bytes_transmitted{0};
bool BusUpdate() {
    if (LL_USART_IsActiveFlag_TXE(USART_CHANNEL) && busState == StateWrite) {
        LL_USART_SetTransferDirection(USART_CHANNEL, LL_USART_DIRECTION_TX); // Disable receiver during writing
        LL_GPIO_SetOutputPin(D_RS485_FLOW_CONTROL_GPIO_Port, D_RS485_FLOW_CONTROL_Pin);
        LL_USART_TransmitData8(USART_CHANNEL, busBuffer[busTxPos++]);
        if (busTxPos >= busBufferLen) {
            // wait until transmission is done
            while(!LL_USART_IsActiveFlag_TC(USART_CHANNEL)) {}
            LL_GPIO_ResetOutputPin(D_RS485_FLOW_CONTROL_GPIO_Port, D_RS485_FLOW_CONTROL_Pin);
            LL_USART_SetTransferDirection(USART_CHANNEL, LL_USART_DIRECTION_TX_RX);
            busState = StateIdle;
            idle_ctr = 0;
        }
    } else if (LL_USART_IsActiveFlag_RXNE(USART_CHANNEL) && busState != StateWrite) {
        uint8_t data = LL_USART_ReceiveData8(USART_CHANNEL);
        bytes_received++;
        if (busState == StateIdle) {
            busAddress = data;
            busState = StateRead;
            busBufferLen = 0;
        } else if (busBufferLen < sizeof(busBuffer)) {
            busBuffer[busBufferLen++] = data;
        } else {
            // printf("rx ovf\n");
        }
        idle_ctr = 0;
    }
    if(busState == StateRead) {
        idle_ctr++;
    }
    if(idle_ctr > 1000 && busState == StateRead) {
        bool rxok = true;
        if (LL_USART_IsActiveFlag_PE(USART_CHANNEL)) {
            rxok = false;
        }
        if (LL_USART_IsActiveFlag_FE(USART_CHANNEL)) {
            rxok = false;
        }
        if (LL_USART_IsActiveFlag_ORE(USART_CHANNEL)) {
            rxok = false;
        }

        LL_USART_ClearFlag_FE(USART_CHANNEL);
        LL_USART_ClearFlag_PE(USART_CHANNEL);
        LL_USART_ClearFlag_ORE(USART_CHANNEL);

        bool matched = matchAddress(busAddress);
        if (!rxok || busBufferLen == 0 || !matched) {
            busBufferLen = 0;
        } else {
            busBufferLen = BusCallback(busAddress, busBuffer, busBufferLen, sizeof(busBuffer));
        }

        if (busBufferLen > 0) {
            busState = StateWrite;
            busTxPos = 0;
        } else {
            busState = StateIdle;
        }
    }

    return busState != StateIdle;
}
