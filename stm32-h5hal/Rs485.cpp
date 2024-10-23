#include "Bus.h"
#include "BaseProtocol.h"
#include "Config.h"

#include <stm32h5xx_hal.h>
#include <stm32h5xx_ll_gpio.h>
#include <stm32h5xx_ll_usart.h>
#include <stm32h5xx_ll_rcc.h>
#include <stm32h5xx_ll_bus.h>

#include <cstdint>
#include <cstring>

#if BOARD_TYPE == prusa_xbuddy_extension
#define D_RS485_FLOW_CONTROL_Pin LL_GPIO_PIN_14
#define D_RS485_FLOW_CONTROL_GPIO_Port GPIOC
#define USART_CHANNEL USART1
#else
#error "Undefined modbus channel and flow control gpio"
#endif

void BusInit() {
    LL_GPIO_InitTypeDef GPIO_InitStruct{};
    LL_USART_InitTypeDef USART_InitStruct{};

    LL_RCC_SetUSARTClockSource(LL_RCC_USART1_CLKSOURCE_PCLK2);

    /* Peripheral clock enable */
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);

    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);

    /**USART1 GPIO Configuration
    PB14   ------> USART1_TX
    PB15   ------> USART1_RX
    */
    GPIO_InitStruct.Pin = LL_GPIO_PIN_14 | LL_GPIO_PIN_15;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_4;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    memset(&GPIO_InitStruct, 0, sizeof(GPIO_InitStruct));

    LL_GPIO_ResetOutputPin(D_RS485_FLOW_CONTROL_GPIO_Port, D_RS485_FLOW_CONTROL_Pin);

    GPIO_InitStruct.Pin = D_RS485_FLOW_CONTROL_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(D_RS485_FLOW_CONTROL_GPIO_Port, &GPIO_InitStruct);

    USART_InitStruct.PrescalerValue = LL_USART_PRESCALER_DIV1;
    USART_InitStruct.BaudRate = 230400; // Default puppy baud rate
    USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
    USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
    USART_InitStruct.Parity = LL_USART_PARITY_NONE;
    USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
    USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
    USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
    LL_USART_Init(USART_CHANNEL, &USART_InitStruct);
    LL_USART_SetTXFIFOThreshold(USART_CHANNEL, LL_USART_FIFOTHRESHOLD_1_8);
    LL_USART_SetRXFIFOThreshold(USART_CHANNEL, LL_USART_FIFOTHRESHOLD_1_8);
    LL_USART_SetRxTimeout(USART_CHANNEL, 35); // Acording to modbus spec you should wait 3.5 characters 
                                              // after sending a message (and also before). We are sending
                                              // 10 bits per byte, so we wait for 35 bits before timeout.
    LL_USART_EnableRxTimeout(USART_CHANNEL);
    LL_USART_DisableFIFO(USART_CHANNEL);
    LL_USART_ConfigAsyncMode(USART_CHANNEL);
    LL_USART_Enable(USART_CHANNEL);

    // We want to enable auto baud rate detection, since testers are unable to run on 230 400
    LL_USART_SetAutoBaudRateMode(USART_CHANNEL, LL_USART_AUTOBAUD_DETECT_ON_STARTBIT);
    LL_USART_EnableAutoBaudRate(USART_CHANNEL);
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

static bool matchAddress(uint8_t address) {
	return address == getConfiguredAddress();
}

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
        }
    } else if (LL_USART_IsActiveFlag_RXNE(USART_CHANNEL) && busState != StateWrite) {
        uint8_t data = LL_USART_ReceiveData8(USART_CHANNEL);
        if (busState == StateIdle) {
            busAddress = data;
            busState = StateRead;
            busBufferLen = 0;
        } else if (busBufferLen < sizeof(busBuffer)) {
            busBuffer[busBufferLen++] = data;
        } else {
            // printf("rx ovf\n");
        }
    } else if (LL_USART_IsActiveFlag_RTO(USART_CHANNEL) && busState == StateRead) {
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
        LL_USART_ClearFlag_RTO(USART_CHANNEL);

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
