/*
 *  Created on: Oct 24, 2023
 *  Author: BalazsFarkas
 *  Project: STM32_Bootloader
 *  Processor: STM32L053R8
 *  Header version: 1.0
 *  File: BootNVMDriver_STM32L0x3.h
 *  Modified from: STM32_NVMDriver/NVMDriver_STM32L0x3.h
 *  Change history: N/A
 */

#ifndef INC_NVMDRIVER_CUSTOM_H_
#define INC_NVMDRIVER_CUSTOM_H_

#include "stdint.h"
#include "stm32l053xx.h"

extern uint32_t Rx_Message_buf [64];

void NVM_Init (void);
void FLASHErase_Page(uint32_t flash_page_addr);
void FLASHUpd_Word(uint32_t flash_word_addr, uint32_t updated_flash_value);
void FLASHIRQPriorEnable(void);

__attribute__((section(".RamFunc"))) void FLASHUpd_HalfPage(uint32_t loc_var_current_flash_half_page_addr, uint8_t full_page_cnt_in_buf, uint8_t half_page_cnt_in_page);		//Note: this function MUST run from RAM, not FLASH!

#endif /* INC_NVMDRIVER_CUSTOM_H_ */
