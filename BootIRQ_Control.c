/*
 * BootIRQ_Control_STM32L0x3.c				v.1.0
 *
 *  Created on: Oct 24, 2023
 *      Author: Balazs Farkas
 *
 *  Code holds the IRQs and their respective priorities.
 *
 * Includes all the IRQs of the Bootloader.
 *
 * v.1.0.
 * UART1 IRQ for message end detection (without DMA).
 * DMA IRQ for the UART1 Rx on channel 3.
 * Added TIM2 timer interrupt to count seconds.
 *
 */

#include "BootClockDriver_STM32L0x3.h"
#include "BootIRQ_Control.h"
#include "main.h"
#include "stdio.h"

//1) DMA IRQ on UART1
void DMA1_Channel2_3_IRQHandler (void){
	/*
	 * IRQ activated on half transmission, full transmission and error.
	 *
	 * 1)We check, what activated the IRQ.
	 * 2)We pull the appropriate flag according to the activation.
	 * 3)We reset the IRQ.
	 *
	 * Note: we want an indifferent FLASH loader, not one that is not controlled differently depending on if we are at the halfway or end point.
	 *
	 * */
	if ((DMA1->ISR & (1<<10)) == (1<<10)) {										//if we had the half transmission triggered
		Machine_Code_Page_Received = First;
	} else if ((DMA1->ISR & (1<<9)) == (1<<9)) {								//if we had full transmission triggered
		Machine_Code_Page_Received = Second;

		//Note: in order to reset the transfer width, we need to fully reinitialize the DMA (or use circular mode). The transfer width register is a DO NOT TOUCH register when the DMA is active.

		USART1->CR1 &= ~(1<<0);													//disable the UART1
		DMA1_Channel3->CCR &= ~(1<<0);											//we disable the DMA channel
																				//we do not need to reset the DMA memory address to the beginning of the Rx buffer, it should have not changed during the DMA running
		DMA1_Channel3->CNDTR |= ((DMA_transfer_width_UART1)<<0);				//we reload the original transfer width
																				//Note: according to the refman 270, only the CNDTR register needs to be reset
		DMA1_Channel3->CCR |= (1<<0);											//we re-enable the DMA channel
		USART1->CR1 |= (1<<0);													//we re-enable the UART1

	} else if ((DMA1->ISR & (1<<11)) == (1<<11)){								//if we had an error
		printf("DMA transmission error!");
		while(1);
	} else {
		//do nothing
	}
	page_counter++;																//we count, how many times the IRQ is engaged (and thus count the number of pages we are updating)
	DMA1->IFCR |= (1<<8);														//we remove all the interrupt flags from Channel 3

}


//2) UART1 IRQ
void USART1_IRQHandler(void) {
	/*
	 * This IRQ currently only activates on the detection of an idle frame.
	 * Idle frames are the indicators that we don't have incoming data anymore.
	 *
	 * Note: since we are parallel receiving data AND doing other stuff, we MUST leave some time for any concurrent process to activate or conclude.
	 * Note: with an idle frame counter set to 2, we have a delay of roughly 1 ms.
	 */

	Idle_frame_counter++;
	if(Idle_frame_counter >=2){
		UART1_Message_Received = Yes;
		Idle_frame_counter = 0;
	}
	USART1->ICR |= (1<<4);														//Idle detect flag clearing
}

//3) TIM2 IRQ
void TIM2_IRQHandler(void) {

	  if (seconds_counter == Boot_transit_in_sec) {

		printf("De-initializing bootloader drivers...\r\n");
		UART1Deinit();													//we deinit the UART1 driver
		BootTIM2_DEINT();												//we deinit the TIM2 driver
																				//Note: it is highyl recommended to deinit all drivers before jumping to the app
		seconds_counter = 0;
		printf("Jumping to app...\r\n");
	  	GoToApp();																//jumping to the app should unblock the micro from waiting for a reply

	  }

	  seconds_counter++;														//we use the seconds counter to count until 3
	  TIM2->SR &= ~(1<<0);														//we reset the IRQ
}

//4)DMA IRQ priority
//Note: Since the DMA IRQ occurs very often, it goes last in priority
void BootDMAIRQPriorEnable(void) {
	NVIC_SetPriority(DMA1_Channel2_3_IRQn, 3);									//IRQ priority for channel 2 & 3
//	NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);										//IRQ enable for channel 2 & 3
}

//5)UART IRQ priority
void UART1IRQPriorEnable(void) {
	NVIC_SetPriority(USART1_IRQn, 2);											//IRQ priority for channel 2 & 3
//	NVIC_EnableIRQ(USART1_IRQn);												//IRQ enable for channel 2 & 3
																				//Note: we don't want the IRQ to be active all the time, only when we have a message incoming
}

//6)TIM2 IRQ priority
void BootTIM2IRQPriorEnable(void) {
	NVIC_SetPriority(TIM2_IRQn, 1);												//IRQ priority for channel 2 & 3
	NVIC_EnableIRQ(TIM2_IRQn);													//IRQ enable for channel 2 & 3
}
