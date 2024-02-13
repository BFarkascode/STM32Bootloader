/*
 *  Created on: Oct 24, 2023
 *  Author: BalazsFarkas
 *  Project: STM32_Bootloader
 *  Processor: STM32L053R8
 *  Header version: 1.0
 *  File: BootIRQ_Control_STM32L0x3.h
 *  Modified from: N/A
 *  Change history: N/A
 */

#ifndef INC_BOOTIRQ_CONTROL_CUSTOM_H_
#define INC_BOOTIRQ_CONTROL_CUSTOM_H_

#include "stdint.h"
#include "string.h"
#include "main.h"
#include "stm32l053xx.h"

//LOCAL CONSTANT
static const uint8_t Boot_transit_in_sec = 5;								//defines how many TIM2 IRQs we wait before leaving the bootloader

//LOCAL VARIABLE
static uint8_t Idle_frame_counter = 0;

//EXTERNAL VARIABLE
extern enum_Yes_No_Selector UART1_Message_Received;
extern enum_Yes_No_Selector UART1_Message_Started;
extern enum_First_Second_Selector Machine_Code_Page_Received;
extern uint16_t DMA_transfer_width_UART1;
extern uint8_t page_counter;
extern uint8_t seconds_counter;

//FUNCTION PROTOTYPES
void UART1IRQPriorEnable(void);
void BootDMAIRQPriorEnable(void);
void BootTIM2IRQPriorEnable(void);

#endif /* INC_BOOTIRQ_CONTROL_CUSTOM_H_ */
