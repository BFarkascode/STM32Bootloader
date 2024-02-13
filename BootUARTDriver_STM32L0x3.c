/*
 *  Created on: Oct 24, 2023
 *  Author: BalazsFarkas
 *  Project: STM32_Bootloader
 *  Processor: STM32L053R8
 *  Program version: 1.0
 *  File: BootUARTDriver_STM32L0x3.c
 *  Modified from: STM32_UARTDriver/UARTDriver_STM32L0x3.c
 *  Change history:
 *
 * v.1.0.
 * Slight rework of the previously written UART1 driver code.
 * Deinit function added.
 *
 */

#include <BootClockDriver_STM32L0x3.h>
#include "BootUARTDriver_STM32L0x3.h"
#include "stm32l053xx.h"

//1)UART init (no DMA)
void UART1Config (void)
{
	/**We wish to recreate the CubeMX generate uart1 control. That uart1 has 115200 baud, 8 bit word length, no parity, 1 stop bit and 16 sample oversampling

	 * Steps to implement
	 * 1)Set up clock for the UART and the GPIO - RCC registers for uart1 and for GPIOA (PA2 and PA3 are the uart1 pins)
	 * 2)Configure the pins - set mode register for pins (alternative mode), GPIO output speed, set alternate function registers (AF registers)
	 * 3)Config UART - word length (CR1), baud rate (BRR), stop bits (CR2), enable pin (CR1), DMA enable (CR3)
	 *
	 * 4)USARt will commence by controlling the TE bit. Write data to TDR (clears TXE bit). Wait until TC if high to indicate that Tx is done.
	 * 5)TXE indicates that TDR is empty.
	 * 6)TC can have an interrupt if TCIE is HIGH.
	 *
	 * Note: DMA must be deinitalized before we change to the app from the boot.
	 * Note: idle is a full frame of 1s. Break character is a full frame of 0s, followed by two stop bits.
	 *
	**/


	//1)Set the clock source, enable hardware
	RCC->APB2ENR |= (1<<14);															//APB2 is the peripheral clock enable. Bit 14 is to enable the uart1 clock
	RCC->IOPENR |= (1<<0);																//IOPENR enables the clock on the PORTA (PA10/D2 is Rx, PA9/D8 is Tx for USART1)
//	RCC->CCIPR |= (2<<0);																//we could clock the UART using HSI16 as source (16 MHz). We currently clock on APB2 isnetad, which is also 16 MHz.

	//2)Set the GPIOs
	GPIOA->MODER &= ~(1<<18);															//AF for PA9
	GPIOA->MODER &= ~(1<<20);															//AF for PA10
	GPIOA->OSPEEDR |= (3<<18) | (3<<20);												//very high speed PA9 and PA10
	GPIOA->AFR[1] |= (1<<6) | (1<<10);													//from page 46 of the device datasheet, USART1 is on AF4 for PA9 and PA10 on the HIGH register of the AFR
																						//OTYPER and PUPDR are not written to since we want push/pull and no pull resistors

	//3)Configure UART
	USART1->CR1 = 0x00;																	//we clear the UART1 control registers
	USART1->CR2 = 0x00;																	//we clear the UART1 control registers
	USART1->CR3 = 0x00;																	//we clear the UART1 control registers
																						//Note: registers can only be manipulated when UE bit in CR1 is reset
	USART1->CR1 &= ~(1<<12);															//M0 word length register. Can only be written to when UART is disabled. M[1:0] = 00 is 1 start bit, 8 data bits, n stop bits
	USART1->CR1 &= ~(1<<28);															//M1 word length register. Works with M0.

	USART1->CR1 &= ~(1<<15);															//oversampling is 16

	USART1->CR1 |= (1<<2);																//enable the Rx part of UART
	USART1->CR1 |= (1<<3);																//enable the Tx part of UART - currently removed

	USART1->CR2 &= ~(1<<12);															//One stop bit
	USART1->CR2 &= ~(1<<13);

	USART1->CR3 |= (1<<11);																//one bit sampling on data
	USART1->CR3 |= (1<<12);																//overrun error disabled
																						//LSB first, CPOL clock polarity is standard, CPHA clock phase is standard

//	USART1->BRR |= 0x683;																//we want to have a baud rate of 9600 with HSI16 as source (refman 779 proposes values for 32 MHz) and oversampling of 16

	USART1->BRR |= 0x116;																//57600 baud rate using 16 MHz clocking and oversampling of 16
																						//Note: 115200 baud rate is just barely too fast for the DMA to restart between incoming UART bytes

	//4)Enable the interrupts, set up errors
	USART1->CR1 |= (1<<4);																//IDIE enabled. It activates the main USART1 IRQ.
																						//Note: an idle frame is the word length, plus stop bit, plus start bit. This will be a bit longer than 1 ms (1.04 ms to be precise) at 9600 baud rate
																						//Note: on an Adalogger, the Serial1 commands are NOT blocking!

}



//2)UART1 DMA enable
void UART1DMAEnable (void) {

	USART1->CR1 &= ~(1<<0);																//disable the UART1

	NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);												//we enable the IRQ for the DMA

	NVIC_EnableIRQ(USART1_IRQn);														//we enable the UART1 IRQ

	DMA1_Channel3->CCR |= (1<<0);														//we enable the DMA channel

	USART1->CR3 |= (1<<6);																//DMA enabled on Rx (DMAR bit)

	USART1->CR3 |= (1<<13);																//DMA is disabled on reception error
																						//RXNE must be cleared or the DMA request removed to clear this

	USART1->CR1 |= (1<<0);																//enable the UART1
}



//3)UART1 get one word
uint8_t UART1RxByte (void)
{
	/* Code to acquire one byte - one Rx transmission.
	 * 1)Check UART state/interrupt register for ready flag
	 * 2)Extract data from the UART RX buffer
	 * **/

	USART1->CR1 |= (1<<0);																//enable the UART1
	uint8_t rx_buf;

	//1)
	while(!((USART1->ISR & (1<<5)) == (1<<5)));											//RXNE bit. Goes HIGH when the data register is ready to be read out.

	//2)
	rx_buf = USART1->RDR;																//reading the RDR clears the RXE flag
	USART1->CR1 &= ~(1<<0);																//disable the UART1
	return rx_buf;
}

//4)UART1 get the message - with message start sequence
void UART1RxMessage(void) {
	/*
	 * 1)We enable the UART.
	 * 2)While the timeout is not reached
	 * 3)We check if we have the incoming message sequence detected
	 * 4)If yes, we transfer incoming bytes to a memory buffer
	 * 5)After the IRQ is triggered and we leave the state machine-in-the-loop, we reset everything and shut down the UART
	 *
	 * Note: the start of the message is detected when 0xFOFO comes through the bus.
	 * Note: the end of the message is detected when the bus has been idle for 2 bytes. This MUST be matched on the transmitter side by implementing an adequately sized delay between messages. Failing to do so freezes the bus.
	 * Note: the bus is VERY noisy. If we don't sample the bus exactly where we should, we can capture fake data.
	 *
	 * */

	//1)
	USART1->CR1 |= (1<<0);																//enable the UART1

	//2)
	while (UART1_Message_Received == No){												//we execute the following section until the IRQ is triggered by an idle line
		switch (UART1_Message_Started){
		case No:
			while(!((USART1->ISR & (1<<5)) == (1<<5)));									//RXNE bit. Goes HIGH when the data register is ready to be read out.
			uint8_t Rx_byte_buf = USART1->RDR;											//reading the RDR clears the RXE flag

			//3)
			if(Rx_byte_buf == UART_message_start_byte) {								//we detected the start byte
				switch (UART1_Start_Byte_Detected_Once){
				case No:																//if this was the first time
					UART1_Start_Byte_Detected_Once = Yes;
					break;
				case Yes:																//if this was the second time
					UART1_Start_Byte_Detected_Once = No;
					UART1_Message_Started = Yes;
					break;
				default:
					break;
				}

			} else {
				UART1_Start_Byte_Detected_Once = No;									//if the start byte is not detected twice in a sequence, we discard
			}
			break;

		//4)
		case Yes:

			if((USART1->ISR & (1<<5)) == (1<<5)){										//if we have data in the RX buffer
																						//Note: we don't use a while loop here to avoid being stuck (RXNE test comes after the idle test, which could freeze the bus)
				USART1->ICR |= (1<<4);													//we clear the idle flag
				NVIC_EnableIRQ(USART1_IRQn);											//we activate the IRQ
																						//Note: the IRQ should only be active when we are receiving a message. Activating it earlier can lead to false triggers.
																						//Note: we use an IRQ to end the message since we can't use a specific sequence to stop it. That sequence can very much be in the machine code.
																						//Note: 0xFFFF is different to two idle frames due to the position of the start bit
				*(Rx_Message_buf_ptr++) = USART1->RDR;									//we read out and reset the RXNE flag
			} else {
				//do nothing
			}
			break;

		default:
			break;
		}
	}

	//5)
	NVIC_DisableIRQ(USART1_IRQn);
	USART1->CR1 &= ~(1<<0);																//disable the UART1
	UART1_Message_Received = No;														//we reset the message received flag
	UART1_Message_Started = No;															//we reset the message started flag
	Rx_Message_buf_ptr = Rx_Message_buf;												//we reset the buf pointer to its original spot
}


//5)UART1 full deinit
	/*
	 * This function is used to deactivate all functions within the UART1 (DMA included).
	 * It is highly recommended to fully de-initialize UART1 before we transition to the app.
	 *
	 */
void UART1Deinit(void) {
	USART1->CR1 &= ~(1<<0);																//disable the UART1
	USART1->CR3 &= ~(1<<6);																//DMA disabled on Rx (DMAR bit)
	DMA1_Channel3->CCR &= ~(1<<0);														//we disable the DMA channel
	NVIC_DisableIRQ(DMA1_Channel2_3_IRQn);												//we disable the IRQ for the DMA
	NVIC_DisableIRQ(USART1_IRQn);														//disable UART1 IRQ
}
