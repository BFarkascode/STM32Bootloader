/*
 *  Created on: 24 Oct 2023
 *  Author: BalazsFarkas
 *  Project: STM32_Bootloader
 *  Processor: STM32L053R8
 *  Header version: 1.0
 *  File: BootExternalController.h
 *  Modified from: N/A
 *  Change history: N/A
 */

#ifndef INC_EXTERNALCONTROLLER_CUSTOM_H_
#define INC_EXTERNALCONTROLLER_CUSTOM_H_


#include "main.h"
#include "BootAppManager.h"

//LOCAL CONSTANT

//LOCAL VARIABLE

//EXTERNAL VARIABLE
extern uint32_t Rx_Message_buf [64];
extern enum_Yes_No_Selector UART1_DMA_active;
extern enum_Yes_No_Selector UART1_Message_Received;
extern uint8_t page_counter;
extern enum_First_Second_Selector Machine_Code_Page_Received;
extern uint32_t flash_page_addr;

//FUNCTION PROTOTYPES
void UART1_External_Boot_Controller (void);

#endif /* INC_EXTERNALCONTROLLER_CUSTOM_H_ */
