/*
 *  Created on: 24 Oct 2023
 *  Author: BalazsFarkas
 *  Project: STM32_Bootloader
 *  Processor: STM32L053R8
 *  Program version: 1.0
 *  File: BootExternalController.c
 *  Modified from: N/A
 *  Change history:
 *
 * v.1.0
 * UART1-based external controller.
 *
 *
 */

#include "BootExternalController.h"


//1)UART1 Rx-based external controller

/*
 * Below is a serial-based bootloader controller that expects single commands or full pages of machine code on UART1.
 * Single commands are sent over using a start sequence. Capture is done using polling. We don't use DMA.
 * Full pages are sent over without (!) a start sequence. Capture is done using DMA.
 * There is no end sequence for the UART messages. The end-of-message is triggered in both above cases if the bus is idle.
 * In both cases, incoming data is stored in a 64 word (256 bytes) long Rx buffer.
 * In Programmer Mode, the Rx buffer is used as a ping-pong buffer. It is also circularly loaded in case we have more than 2 pages of machine code to load.
 *
 * Writing to FLASH within this particular iteration is done using half-page write bursts, which is significantly faster than writing word-by-words
 *
 * The reason why the code is so convoluted is that we don't have a master in UART. Thus the state of the bus must be used to govern, what happens.
 *
 * The code does not have a Tx element on the UART1. The stm32 can not communicate with the partner device. It does communicate with the PC using UART2.
 *
 * */

void UART1_External_Boot_Controller (void) {

	  //Command and Control Mode
	  if(UART1_DMA_active == No) {														//in C&C Mode, we expect a sequence of bytes (0xF0F0) followed by a command sequence
		  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	//we poll for the starting sequence but do not log it
		  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	//we are NOT using DMA!

		  UART1RxMessage();														//we call the UART function - no DMA - to scan for a command sequence
		  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	//this order logs at maximum 256 bytes of incoming UART messages
		  switch (Rx_Message_buf[0]) {

		  case 0xaa:																	//activate/jump to app
			  printf("De-initializing bootloader drivers...\r\n");
			  UART1Deinit();
			  printf("Jumping to app...\r\n");
			  GoToApp();																//we simply jump to the APP and leave the bootloader
			  break;

		  case 0xbb:																	//switch to programmer mode
			  printf("Update app...\r\n");
			  memset(Rx_Message_buf, 0, 64);											//wipe the buffer

			  UART1Deinit();														//we completely deinitialize the UART1

			  DMAChannelUART1RxConfig(&Rx_Message_buf[0]);						//DMA channel reconfig - necessary after DMA shut off to ensure functionality
			  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	//Note: transfer width is 256 bytes, which is 2 pages of FLASH
			  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	//Note: the above 3 functions are mere config functions and do not activate the DMA

		      UART1DMAEnable();	 	  	  	  	  	  	  	  	  	  	  	  	//we activate the DMA and the idle detection UART IRQ
		      	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	//capture the incoming machine code (64 words) with DMA
			  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	//here we want to have the DMA running and using the IRQs (for starters, only the TC IRQ to generate a flag)

		      UART1_DMA_active = Yes;

			  printf("Awaiting machine code...\r\n");

			  break;

		  case 0xcc:																	//reboot
			  ReBoot();
			  break;

		  default:
			  //do nothing
			  break;
		  }

	  //Programmer Mode
	  } else if (UART1_DMA_active == Yes) {								  	  	  	  	//defined by the DMA being active (response to the command 0xbb)

		  if (UART1_Message_Received == Yes) {											//in Programmer Mode if we detect that the bus is idle

			  UART1Deinit();														//we de-initialize the UART completely
			  UART1_DMA_active = No;													//remove the DMA flag
			  UART1_Message_Received = No;												//remove the message received flag
			  printf("%d pages of machine app code have been updated \r\n", page_counter);	//we publish the page counter results
			  page_counter = 0;															//we reset the page counter
			  flash_page_addr = App_Section_Start_Addr;									//we move the flash pointer to the start of the app for additional updates
			  memset(Rx_Message_buf, 0, 64);											//we wipe the UART buffer
			  USART1->CR1 |= (1<<0);													//we re-enable the UART1 without DMA

		  } else if (UART1_Message_Received == No){										//if the bus is not idle

			  switch (Machine_Code_Page_Received) {										//we check if we are at the first or the second part of the 64 word long Rx buffer

			  case First:																//if we are in the front - triggered by the DMA halfway point
				  UpdatePageInApp(flash_page_addr, 0);									//we pass the address as well as from where in the buffer we intend to read the data
				  flash_page_addr = flash_page_addr + 0x80;								//we step the page address by one page
				  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	//Note: we select the page to process on this level
				  Machine_Code_Page_Received = None;									//we remove the page detection flag
				  break;

				  //Note: the FLASH copying MUST ALWAYS BE faster than the data reception!

			  case Second:																//if we are in the back - triggered by the DMA TC point
				  UpdatePageInApp(flash_page_addr, 1);
				  flash_page_addr = flash_page_addr + 0x80;								//we step the page address by one page
				  Machine_Code_Page_Received = None;									//we remove the page detection flag

				  //Note: there is a small delay between the page_received flag (being "first" or "second") and the "message received" flag being "yes".
				  //Note: the "standby" state of the main loop must be able to sample the page_received state change before the message received "yes"
				  //Note: in other words, the main loop has to be fast enough to detect a change in the Rx buffer state before it detects that the bus is idle
				  //Note: current standby cycle time (sampling time) of DMA standby is 2 us...

				  break;

			  case None:
				  //do nothing
				  break;

			  default:
				  //do nothing
				  break;
			  }

		  } else {
			  //do nothing
		  }

		  //Note: the DMA needs to be restarted when the logging reaches the end of the Rx buffer. This takes time.
		  //UART needs to be slower than the DMA restart time, otherwise only every second set of data will be captured.

	  } else {
		  //do nothing
	  }
}
