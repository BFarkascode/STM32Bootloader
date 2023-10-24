/*
 * BootClockDriver_STM32L0x3.c			v.1.0
 *
 *  Created on: Oct 24, 2023
 *      Author: Balazs Farkas
 *
 *v.1.0
 * Below is a custom RCC configuration function simplified from the original clock driver.
 * Simplification includes the removal of TIM21 and TIM22, plus the PWM functions on TIM2. None of those are currently in use.
 * Removed constant related to TIM21 and TIM22 timers from the header file.
 * Added a TIM2 based timer with an IRQ at every second.
 *
 *
 * Note: for simple bootloader action, only TIM6 and TIM2 (as a timer) are used only.
 * Note: TIM2 PWM is currently not planed for the bootloader. If this is to change, boot_TIM2 should be merged with TIM22.
 *
 */

#include "BootClockDriver_STM32L0x3.h"
#include "stm32l053xx.h"														//device specific header file for registers

//1)We set up the core clock and the peripheral prescalers/dividers
void SysClockConfig(void) {
	/**
	 * 1)Enable - future - system clock and wait until it becomes available. Originally we are running on MSI.
	 * 2)Set PWREN clock and the VOLTAGE REGULATOR
	 * 3)FLASH prefetch and LATENCY
	 * 4)Set PRESCALER HCLK, PCLK1, PCLK2
	 * 5)Configure PLL
	 * 6)Enable PLL and wait until available
	 * 7)Select clock source for system and wait until available
	 *
	 **/

	//1)
	//HSI16 on, wait for ready flag
	RCC->CR |= (1<<0);															//we turn on HSI16
	while (!(RCC->CR & (1<<2)));												//and wait until it becomes stable. Bit 2 should be 1.


	//2)
	//power control enabled, put to reset value
	RCC->APB1RSTR |= (1<<28);													//reset PWR interface
	PWR->CR |= (1<<11);															//we put scale1 - 1.8V - becasue that is what CubeMx sets originally (should be the reset value here)
	while ((PWR->CSR & (1<<4)));												//and wait until it becomes stable. Bit 4 should be 0.

	//3)
	//Flash access control register - no prefetch, 1WS latency
	FLASH->ACR |= (1<<0);														//1 WS
	FLASH->ACR &= ~(1<<1);														//no prefetch
	FLASH->ACR |= (1<<6);														//preread
	FLASH->ACR &= ~(1<<5);														//buffer cache enable

	//4)Setting up the clocks
	//Note: this part is always specific to the usecase!
	//Here 32 MHz full speed, HSI16, PLL_mul 4, plldiv 2, pllclk, AHB presclae 1, hclk 32, ahb prescale 1, apb1 clock divider 4, apb2 clockdiv 1, pclk1 2, pclk2 1

	//AHB 1
	RCC->CFGR |= RCC_CFGR_HPRE_DIV1;											//this is the same as putting 0 everywhere. Check the stm32l053xx.h for the exact register definitions

	//APB1 1
	RCC->CFGR |= (5<<8);														//we put 101 to bits [10:8]. This should be a DIV4.
																				//Alternatively it could have been just	RCC->CFGR |= RCC_CFGR_PPRE1_DIV4;

	//APB2 1
	RCC->CFGR |= (4<<11);														//we put 100 to bits [13:11]. This should be a DIV2.
																				//Alternatively it could have been just	RCC->CFGR |= RCC_CFGR_PPRE2_DIV2;

	//PLL source HSI16
	RCC->CFGR &= ~(1<<16);

	//PLL_mul 4
	RCC->CFGR |= (1<<18);

	//pll div 2
	RCC->CFGR |= (1<<22);

	//5)Enable PLL
	//enable and ready flag for PLL
	RCC->CR |= (1<<24);															//we turn on the PLL
	while (!(RCC->CR & (1<<25)));												//and wait until it becomes available

	//6)Set PLL as system source
	RCC->CFGR |= (3<<0);														//PLL as source
	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);						//system clock status (set by hardware in bits [3:2]) should be matching the PLL source status set in bits [1:0]

	SystemCoreClockUpdate();													//This CMSIS function must be called to update the system clock! If not done, we will remain in the original clocking (likely MSI).
}

//2) TIM6 setup for precise delay generation (removal of HAL_Delay)
void TIM6Config (void) {
	/**
	 * TIM6 is a basic clock that is configured to provide a counter for a simple delay function (see below).
	 * It is connected to AP1B which is clocking at 16 MHz currently (see above the clock config for the PLL setup to generate 32 MHz and then the APB1 clock divider left at DIV4 and a PCLK multiplier of 2 - not possible to change)
	 * 1)Enable TIM6 clocking
	 * 2)Set prescaler and ARR
	 * 3)Enable timer and wait for update flag
	 **/

	//1)
	RCC->APB1ENR |= (1<<4);														//enable TIM6 clocking

	//2)

	TIM6->PSC = 16 - 1;															// 16 MHz/16 = 1 MHz -- 1 us delay
																				// Note: the timer has a prescaler, but so does APB1!
																				// Here APB1 PCLK is 16 MHz

	TIM6->ARR = 0xFFFF;															//Maximum ARR value - how far can the timer count?

	//3)
	TIM6->CR1 |= (1<<0);														//timer counter enable bit
	while(!(TIM6->SR & (1<<0)));												//wait for the register update flag - UIF update interrupt flag
																				//update the timer if we are overflown/underflow with the counter was reinitialised.
																				//This part is necessary since we can update on the fly. We just need to wait until we are done with a counting cycle and thus an update event has been generated.
																				//also, almost everything is preloaded before it takes effect
																				//update events can be disabled by writing to the UDIS bits in CR1. UDIS as LOW is UDIS ENABLED!!!s
}


//3) Delay function for microseconds
void Delay_us(int micro_sec) {
	/**
	 * 1)Reset counter for TIM6
	 * 2)Wait until micro_sec
	 **/
	TIM6->CNT = 0;
	while(TIM6->CNT < micro_sec);												//Note: this is a blocking timer counter!
}


//4) Delay function for milliseconds
void Delay_ms(int milli_sec) {
	for (uint32_t i = 0; i < milli_sec; i++){
		Delay_us_custom(1000);													//we call the custom microsecond delay for 1000 to generate a delay of 1 millisecond
	}
}


//5) TIM2 setup for a timer trigger at 3000 ms
void BootTIM2_INT (void) {
	/**
	 * Configuration is identical to TIM21/TIM22.
	 *
	 * Note: TIM2 is planned to be a PWM source outside the bootloader.
	 *
	 **/

	//1)
	RCC->APB1ENR |= (1<<0);														//enable TIM2 clocking

	//2)

	TIM2->PSC = 16000 - 1;														// 16 MHz/16000 = 1 kHz -- 1 ms delay
																				//Note: we can't use the same PSC as with TIM21 since it is still too fast that way to reach 500 ms.

	TIM2->ARR = TIM2_timer_interrupt;											//We want to count until 2999 only

	//3)
	TIM2->CR1 |= (1<<2);														//we want a trigger only when overflow happens
	TIM2->DIER |= (1<<0);														//update interrupt enabled. The SR registers's bit [0] will have the flag for this interrupt.

	TIM2->CR1 |= (1<<0);														//timer counter enable bit
																				//Note: after init, the timer should be generating a trigger every 500 ms
}


//6) TIM2 full deinit function
void BootTIM2_DEINT (void) {
	TIM2->CNT = 0;																//reset counter
	TIM2->CR1 |= (1<<0);														//we shut off the TIM2 timer - used to transition to the app upon timeout
	NVIC_DisableIRQ(TIM2_IRQn);													//we disable the TIM2 IRQ
	TIM2->SR &= ~(1<<0);														//we clear the TIM2 IRQ trigger flag
}
