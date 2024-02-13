/*
 *  Created on: 24 Oct 2023
 *  Author: BalazsFarkas
 *  Project: STM32_Bootloader
 *  Processor: STM32L053R8
 *  Program version: 1.0
 *  File: BootAppManager.c
 *  Modified from: N/A
 *  Change history:
 *
 *  Code holds the control commands for the bootloader.
 *
 *v.1.0.
 *Below is a simple function to allow the bootloader to control/update/restart the app section.
 *
 */

#include "BootAppManager.h"


//1) Jump to app
/*
 *	What we do here is that we define a function pointer into which we copy the function pointer we (should) find at App_Section_Start_Addr + 4.
 *  From the NVIC table, an app placed by the linker at "App_Section_Start_Addr" will have the reset vector at App_Section_Start_Addr + 4.
 *  Once the copy was successful, we call the function pointer and thus switch to the app.
 *  Mind, the app is a stand-alone element that is limited to the FLASH area App_Section_Start_Addr and after, thanks to the app's linker.
 *
 *  Note: the vector tables will be updated after the jumps given the system files and the linkers are properly set.
 *  Note: after the jump, we start with the startup assembly file (so a full reset occurs)
 *
 * */

void GoToApp(void)
{
	uint32_t App_reset_vector_addr;																	//this is the address of the app's reset vector (which is also a function pointer!)
	void (*Start_App_func_ptr)(void);																//the local function pointer we define

	if((*(uint32_t*)App_Section_Start_Addr) == 0x20002000)											//we check, what is stored at the App_Section_Addr. It should be the very first word of the app's code.
																									//This value should be the reset value of the stack pointer in RAM.
																									//Note: the exact value stored at the App_Section_Addr needs to be checked (it seems to be 0x20002000)
																									//Note: the memory monitor reads out the memory values upside-down! (there is an endian switch during the process)
	{
		printf("APP found. Starting...\r\n");
		App_reset_vector_addr = *(uint32_t*)(App_Section_Start_Addr + 4);							//we define a pointer to APP_ADDR + 4 and then dereference it to extract the reset vector for the app
																									//JumpAddress will hold the reset vector address (which won't be the same as APP_ADDR + 4, the address is just stored there)
		Start_App_func_ptr = App_reset_vector_addr;													//we call the local function pointer with the address of the app's reset vector
																									//Note: for the bootloader, this address is an integer. In reality, it will be a function pointer once the app is placed.
		__set_MSP(*(uint32_t*) App_Section_Start_Addr);												//we move the stack pointer to the APP address
		Start_App_func_ptr();																		//here we call the APP reset function through the local function pointer
	} else {
		printf("No APP found. \r\n");
	}

}

//2)FLASH update
/*
 * We update a page (32 words or 128 bytes) of FLASH memory.
 * The page must be erased first to avoid data corruption.
 * The function takes in an array pointer to then use that pointer to step through the array.
 * The array stores the machine code in hex format.
 *
 * */
void UpdatePageInApp (uint32_t page_addr_in_FLASH, uint8_t full_page_select_in_buf) {
	/*
	 * We update one (!) page in the app by reading in values through a pointer.
	 * We are using the function by relying on local variables. Stepping (page selection) is done externally. Half pages are selected within those pages using a "for" loop.
	 * The page is first erased, then replaced by an array of 32 words (32 x 32 = 1 kbit, which is 128 bytes).
	 * It is not possible to erase smaller section than 128 bytes.
	 *
	 * Note: the pointer must be properly manipulated to allow the right FLASH elements to be updated. Failing to do so will corrupt the app we intend to update.
	 *
	 * */

	 FLASHErase_Page(page_addr_in_FLASH);
	 //Note: we select the page, then we select the half-page within that page

	 for(uint8_t half_page_select_in_buf = 0; half_page_select_in_buf < 2; half_page_select_in_buf++) {														//copying two half pages demand a loop of 2
		FLASHUpd_HalfPage(page_addr_in_FLASH, full_page_select_in_buf, half_page_select_in_buf);	//unlike the word by word version where we passed the pointer value, we pass just the Rx_buffer position data - where we should read from it
		page_addr_in_FLASH = page_addr_in_FLASH + 0x40;												//we increment the address value by half a page
																									//or I can just pass addresses in there instead? well, no, not really since the data is not kept at a predefined address
																									//After 32 steps, we have updated a full page worth of FLASH area.
	 }

}


//3) Reboot
/*
 *	This function reboots the microcontroller using software.
 *	It is slower than using hardware reset, but the outcome is the same.
 *	The code is practically the same as boot jump, just the other direction.
 *
 * */

void ReBoot(void)
{
	uint32_t Boot_reset_vector_addr;																//this is the address of the app's reset vector (which is also a function pointer!)
	void (*Start_Boot_func_ptr)(void);																//the local function pointer we define

	if((*(uint32_t*)Boot_Section_Start_Addr) == 0x20002000)											//we check, what is stored at the Boot_Section_Addr. It should be the very first word of the app's code.
																									//we do this check since we may run a device without a bootloader
	{
		printf("Rebooting...\r\n");
		Boot_reset_vector_addr = *(uint32_t*)(Boot_Section_Start_Addr + 4);							//we define a pointer to APP_ADDR + 4 and then dereference it to extract the reset vector for the app
																									//JumpAddress will hold the reset vector address (which won't be the same as APP_ADDR + 4, the address is just stored there)
		Start_Boot_func_ptr = Boot_reset_vector_addr;													//we call the local function pointer with the address of the app's reset vector
																									//Note: for the bootloader, this address is an integer. In reality, it will be a function pointer once the app is placed.
		__set_MSP(*(uint32_t*) Boot_Section_Start_Addr);												//we move the stack pointer to the APP address
		Start_Boot_func_ptr();																		//here we call the APP reset function through the local function pointer
	} else {
		printf("Boot not found. \r\n");
	}

}


//4) Reset app
/*
 *	This function resets the app using software. It does not use the bootloader.
 *	Mind, resetting the app only makes sense when run within the app's code. Thus, it does not have the address control like jumping in and out of the boot.
 *
 * */

void ResetApp(void) {
	uint32_t App_reset_vector_addr;																	//this is the address of the app's reset vector (which is also a function pointer!)

	void (*Start_App_func_ptr)(void);																//the local function pointer we define

	printf("Resetting app...\r\n");

	App_reset_vector_addr = *(uint32_t*)(App_Section_Start_Addr + 4);								//we define a pointer to APP_ADDR + 4 and then dereference it to extract the reset vector for the app
																									//JumpAddress will hold the reset vector address (which won't be the same as APP_ADDR + 4, the address is just stored there)
	Start_App_func_ptr = App_reset_vector_addr;														//we call the local function pointer with the address of the app's reset vector
																									//Note: for the bootloader, this address is an integer. In reality, it will be a function pointer once the app is placed.
	__set_MSP(*(uint32_t*) App_Section_Start_Addr);													//we move the stack pointer to the APP address
	Start_App_func_ptr();																			//here we call the APP reset function through the local function pointer
}
