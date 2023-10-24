/*
 * BootClockDriver_STM32L0x3.h			v.1.0
 *
 *  Created on: Oct 24, 2023
 *      Author: Balazs Farkas
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
