/*
 *  Created on: Oct 24, 2023
 *  Author: BalazsFarkas
 *  Project: STM32_Bootloader
 *  Processor: STM32L053R8
 *  Header version: 1.0
 *  File: BootDMADriver_STM32L0x3.h
 *  Modified from: STM32_DMADriver/DMADriver_STM32L0x3.h
 *  Change history: N/A
 */

#ifndef INC_BOOTDMADRIVER_CUSTOM_H_
#define INC_BOOTDMADRIVER_CUSTOM_H_

#include "main.h"
#include "stdint.h"


//LOCAL CONSTANT

//LOCAL VARIABLE

//EXTERNAL VARIABLE
extern uint16_t DMA_transfer_width_UART1;

//FUNCTION PROTOTYPES
void BootDMAInit(void);
void DMAChannelUART1RxConfig(uint32_t mem_addr_UART1_Rx);

#endif /* INC_BOOTDMADRIVER_CUSTOM_H_ */
