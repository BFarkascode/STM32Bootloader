/*
 *  Created on: 24 Oct 2023
 *  Author: BalazsFarkas
 *  Project: STM32_Bootloader
 *  Processor: STM32L053R8
 *  Header version: 1.0
 *  File: BootAppManager.h
 *  Modified from: N/A
 *  Change history: N/A
 */

#ifndef INC_APPMANAGER_CUSTOM_H_
#define INC_APPMANAGER_CUSTOM_H_

#include <BootNVMDriver_STM32L0x3.h>
#include "main.h"
#include "stdint.h"
#include "stdio.h"



//LOCAL CONSTANT
static const uint32_t App_Section_Start_Addr = 0x8008000;					//this is the app section's address. It is defined in the linker files.
static const uint32_t Boot_Section_Start_Addr = 0x8000000;					//this is the boot section's address. It is defined in the boot's linker file.

//LOCAL VARIABLE


//EXTERNAL VARIABLE
extern uint32_t Rx_Message_buf [64];

//FUNCTION PROTOTYPES
void GoToApp(void);
void UpdatePageInApp (uint32_t loc_var_current_flash_page_addr, uint8_t current_data_page_select);
void ReBoot(void);
void ResetApp(void);

#endif /* INC_APPMANAGER_CUSTOM_H_ */
