/*
 *  Created on: Oct 24, 2023
 *  Author: BalazsFarkas
 *  Project: STM32_Bootloader
 *  Processor: STM32L053R8
 *  Program version: 1.0
 *  File: BootDMADriver_STM32L0x3.c
 *  Modified from: STM32_DMADriver/DMADriver_STM32L0x3.c
 *  Change history:
 *
 * Code holds the DMA controller for the bootloader.
 *
 * v.1.0
 * Below is a custom DMA driver running within the Bootloader.
 * It initialises the seven DMA channels according to page 263 of the refman.
 * Currently runs only the UART1 Rx on Channel3.
 *
 */

#include "BootDMADriver_STM32L0x3.h"

//1)We set up the basic driver for the DMA
void BootDMAInit(void){
	/* Initialize DMA
	 *
	 * 1)Enable clocking
	 * 2)Remap channels to be used
	 *
	 * Note: UART_Rx could be on Channel 5 as well.
	 * */

	//1)
	RCC->AHBENR |= (1<<0);														//enable DMA clocking
																				//clocking is directly through the AHB

	//2)
	DMA1_CSELR->CSELR |= (3<<8);												//DMA1_Channel3: will be requested at 4'b0011 for UARI1_RX
}


//2)We set up a channel for the UART1 Rx
void DMAChannelUART1RxConfig(uint32_t mem_addr_UART1_Rx){
	/* Configure the DMA channel for UART1 Rx - channel3
	 *
	 * 1)Ensure that selected adc dma channel is disabled
	 * 2)Configure channel parameters: transfer length, transfer direction, address increment, transfer type (peri-to-mem)
	 * 3)Provide peri and mem addresses
	 * 4)Provide transfer width
	 *
	 * Note: transfer width is in bytes since we have 8-bit words!
	 *
	 */
	//1)
	DMA1_Channel3->CCR &= ~(1<<0);												//we disable this DMA channel, if it is activated

	//2)
	DMA1_Channel3->CCR |= (1<<1);												//we enable the transfer complete interrupt within the DMA channel
	DMA1_Channel3->CCR |= (1<<2);												//we enable the half-transfer interrupt within the DMA channel
	DMA1_Channel3->CCR |= (1<<3);												//we enable the error interrupt within the DMA channel
	DMA1_Channel3->CCR &= ~(1<<4);												//we read from the peripheral
																				//circular mode is off - we will use the IRQ to restart the DMA after TC
	DMA1_Channel3->CCR &= ~(1<<6);												//peripheral increment is not used - we have just the RDR register to read from
	DMA1_Channel3->CCR |= (1<<7);												//memory increment is used
	DMA1_Channel3->CCR &= ~(3<<8);												//peri side data length is 8 bits - we have 8 bit words
	DMA1_Channel3->CCR &= ~(3<<10);												//mem side data length is 8 bits - we capture 8 bit words
	DMA1_Channel3->CCR |= (3<<12);												//priority level is set as VERY HIGH - we have only one channel in use
																				//mem-to-mem is not used
	//3)
	DMA1_Channel3->CPAR = (uint32_t) (&(USART1->RDR));							//we want the data to be extracted from the UART's RDR register
																				//RXNE control is not necessary, the DMA will reset it for us

	DMA1_Channel3->CMAR = mem_addr_UART1_Rx;									//this is the address (!) of the memory buffer we want to funnel data into

	//4)
	DMA1_Channel3->CNDTR |= ((DMA_transfer_width_UART1)<<0);					//we want to have an element burst of "DMA_transfer_width_UART1"
																				//transfer_width_UART1 is set in bytes (!)
}
