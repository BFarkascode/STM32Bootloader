# STM32Bootloader

Arduino-style bootloader for an STM32 L0x3 mcu using bare metal programming.

This is a general bootloader for the STM32L0xx devices. If commanded, it places incoming data at memory position 0x8008000 and jump to that position.

Control and code reception is done using incoming serial. The “master” device can be whatever that can generate such a serial signal, albeit the command construction must follow the guidelines defined within the STM32_UARTDriver project.

The bootloader can process for commands from the master. It can:
-	Switch on external control
-	Reboot
-	Switch to code reception mode and then await code
-	De-initialize and activate the app 

In case upon reset the bootloader does not receive a request to switch to external control in 5 seconds, it automatically de-initializes itself and activates the app.

## To read
There is no additional reading material necessary to understand this code. On the other hand, the following project must be understood profoundly:
-	STM32_UARTDriver
-	STM32_NVMDriver
-	STM32_NVIC_Usecase
-	STM32_DMADriver

## Particularities
Here I want to touch upon the modifications that I had to implement on the projects I mentioned above to make them work together.

### UART
We are running the serial communication in only one direction (Rx) at a baud rate of 57600. This was done so because Tx from the STM32 is not necessary for such bootloader application.

Control is done by simply polling the UART bus for a specific sequence (see the “external controller” part below). We are using here the blocking (!) UART message reception function since we can assume that if we are controlling the STM32 externally, we wouldn’t want it to do anything unless specifically told to. This is a slow and inefficient way to transfer data, albeit we don’t actually care for the command section.

On the other hand, we do care a lot about the speed of data transfer when transferring the machine code from the master device. When the device expects incoming machine code, the UART is engaged using DMA. The DMA interrupts as halfway and end of transmission is used then control the ping-pong buffer (see below).

The speed of the UART is chosen to be 57600 since anything faster does not allow enough time for the DMA to be reengaged between transmissions.

We added a small function to enable the DMA on UART and another small function to de-initialise the UART completely. This latter is necessary to run the UART with and without DMA in the same code. Failing to completely reset the UART – that is, running it in manual mode while DMA is active or vice versa - will freeze the execution.

### NVM
The app is placed in the memory position 0x8008000. As such, whatever machine code is coming through the serial into the bootloader, it must be compatible with that memory position.

We are running half-page burst FLASH updates since it is significantly faster than the word-by-word version.

We removed the EXTI, wanting to engage any FLASH update using UART commands instead.

### NVIC
The code is identical to the original project, except it has a “reset app” function added to the code.

For code organization, the FLASH page update function is moved into the “BootAppManager.c” source code. This function is constructed to process the ping-pong buffer one half-page at a time as the buffer is being filled. The control of this action is done in the function defined within the “BootExternalController.c” source file.


### DMA
We activate the DMA on the UART when the machine code is coming in. We also use the half-way and full transfer interrupts within the DMA to control something called a “ping-pong buffer”: a buffer that is divided into two parts with one part being loaded while the other part is being processed. This is possible to do since DMA can run in parallel to the main code. A ping-pong buffer allows us to constantly process data as it is incoming without any delays or pauses. The buffer is sized to match two pages of FLASH, or 256 bytes.

The DMA transmission is exactly the same length as the ping-pong buffer, demanding a reset once the buffer is full. This reset time is the bottleneck to go to higher speeds with the baud rate.

## User guide
Let’s look at the code specifically written for this project!
