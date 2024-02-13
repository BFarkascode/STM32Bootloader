/*
 *  Created on: Oct 24, 2023
 *  Author: BalazsFarkas
 *  Project: STM32_Bootloader
 *  Processor: STM32L053R8
 *  Header version: 1.0
 *  File: BootUARTDriver_STM32L0x3.h
 *  Modified from: STM32_UARTDriver/UARTDriver_STM32L0x3.h
 *  Change history: N/A
 */

#ifndef INC_UARTDRIVER_CUSTOM_H_
#define INC_UARTDRIVER_CUSTOM_H_

#include "stdint.h"
#include "main.h"

//LOCAL CONSTANT
static const uint8_t UART_message_start_byte = 0xF0;		//the message start sequence is (twice this byte)

//LOCAL VARIABLE
static enum_Yes_No_Selector UART1_Start_Byte_Detected_Once = No;

//EXTERNAL VARIABLE
extern enum_Yes_No_Selector UART1_Message_Received;
extern enum_Yes_No_Selector UART1_Message_Started;
extern uint32_t Rx_Message_buf [64];						//we have a 32 bit MCU
extern uint8_t* Rx_Message_buf_ptr;							//UART data is only 8 bits

//FUNCTION PROTOTYPES
void UART1Config (void);
uint8_t UART1RxByte (void);
void UART1RxMessage(void);
void UART1DMAEnable (void);
void UART1Deinit(void);


#endif /* INC_UARTDRIVER_CUSTOM_H_ */
