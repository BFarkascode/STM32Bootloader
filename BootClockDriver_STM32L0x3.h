/*
 *  Created on: Oct 24, 2023
 *  Author: BalazsFarkas
 *  Project: STM32_Bootloader
 *  Processor: STM32L053R8
 *  Program version: 1.0
 *  File: BootClockDriver_STM32L0x3.c
 *  Modified from: STM32_ClockDriver/ClockDriver_STM32L0x3.c
 *  Change history: N/A
 */

#ifndef INC_BOOTRCCTIMPWMDELAY_CUSTOM_H_
#define INC_BOOTRCCTIMPWMDELAY_CUSTOM_H_

#include "stdint.h"

//LOCAL CONSTANT
//constant for the seconds counter (sets the TIM2 IRQ)
const static uint16_t TIM2_timer_interrupt = 0x7cf;										//currently at 1000 ms

//FUNCTION PROTOTYPES
void SysClockConfig(void);
void TIM6Config (void);
void Delay_us(int micro_sec);
void Delay_ms(int milli_sec);
void BootTIM2_INT (void);
void BootTIM2_DEINT (void);

#endif /* BOOTRCCTIMPWMDELAY_CUSTOM_H_ */
