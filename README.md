# STM32Bootloader

Arduino-style bootloader for an STM32 L0x3 mcu using bare metal programming.

## General description

This is a general bootloader for the STM32L0xx devices. If commanded, it places incoming data at memory position 0x8008000 and jump to that position.

Control and code reception is done using incoming serial. The “master” device can be whatever that can generate such a serial signal, albeit the command construction must follow the guidelines defined within the STM32_UARTDriver project. The machine codes does not need to have a message start sequence.

The bootloader can process for commands from the master. It can:
-	Switch on external control
-	Reboot
-	Switch to code reception mode and then await code
-	De-initialize and activate the app 

In case upon reset the bootloader does not receive a request to switch to external control in 5 seconds, it automatically de-initializes itself and activates the app.

## Previous relevant projects
The following projects should be checked:
-	STM32_UARTDriver
-	STM32_NVMDriver
-	STM32_NVIC_Usecase
-	STM32_DMADriver
-	STM32_ClockDriver

## To read
There is no additional reading material necessary to understand this code.

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

### NVIC (called AppManager here)
The code is identical to the original project, except it has a “reset app” function added to the code.

For code organization, the FLASH page update function is moved into the “BootAppManager.c” source code. This function is constructed to process the ping-pong buffer one half-page at a time as the buffer is being filled. The control of this action is done in the function defined within the “BootExternalController.c” source file.


### DMA
We activate the DMA on the UART when the machine code is coming in. We also use the half-way and full transfer interrupts within the DMA to control something called a “ping-pong buffer”: a buffer that is divided into two parts with one part being loaded while the other part is being processed. This is possible to do since DMA can run in parallel to the main code. A ping-pong buffer allows us to constantly process data as it is incoming without any delays or pauses. The buffer is sized to match two pages of FLASH, or 256 bytes.

The DMA transmission is exactly the same length as the ping-pong buffer, demanding a reset once the buffer is full. This reset time is the bottleneck to go to higher speeds with the baud rate.

## User guide
Let’s look at the code specifically written for this project!

### main.c
The only thing the main.c does is that it listens to the UART bus for a certain byte to come in. If it does come in, it activates the external controller after shutting down the TIM2 timer (for what TIM2 does, check the IRQ controller). If it does not for 5 seconds, the TIM2 IRQ is activated and we transition to the app.

Additionally, there is the "write" which funnels printf to the CubeIDE.

### IRQ controller
This holds all the IRQs (and priority functions) the bootloader is using, something that was previously stored locally for DMA and the UART. I moved them over to improve code readability.

The DMA IRQ is engaged upon both the half-way and the end point of the DMA's activity. Depending on which trigger activated the IRQ, we then generated flags the external controller will use to process the incoming data. Also, if we had a transfer complete IRQ, we reset the DMA and move to the start of the ping-pong buffer, thereby preparing the bootloader for the next two pages to arrive. This reset is the timing bottleneck that limits our UART to 57600 baud rate: if UART is faster, the next byte after two pages of data comes in before the DMA activates, shutting down the DMA. Of note, we only activate the DMA when we are expecting machine code to come in.

The UART IRQ is the same as before and we use it to detect the end of a message.

Lastly, we have a timer interrupt that goes off every time a second passes (TIM2 is set as the timer). If the IRQ is activated 5 times - indicating that 5 seconds have passed - we de-init and activate the app.

### External controller
This is the command center of the bootloader. It expects certain command bytes to come in on the uart (0xaa for app activation, 0xbb for app update and 0xcc for reboot).

The code is a state machine and sets its own flags to allow progression.

In "command and control" mode, we aren't using the DMA and run the setup similar to how we did during the UARTDriver project (that is, we are blocking with our UART). We do activate the DMA within this mode and thus transition to the second part of the state machine, "programmer mode" (we aren't blocking).

When the DMA is active and machine code is coming in, it adjusts the values for the FLASH address depending on which half page we have extracted from the ping-pong buffer.

Of note, all "break" lines break the entire state machine and force the execution to exit it. Thus, if we want to update the app, we need to first go to programmer mode with one uart transmission and then send over the machine code using a separate transmission.

### Additional code - ClockDriver
I am a bit torn about discussing this code since setting up the clocking of the device is pretty simple, yet absolutely crucial at the same time (see figure 17 in the refman). It is something that has been discussed often and many times thus I don't think I can contribute well to explaining it. Also, it is not strictly necessary to write a custom clock driver since, unlike other HAL-based peripheral and setup options, clocking with CubeMx/HAL seems rock solid to me.

Then why did I do this? Well, I wanted to distance myself from HAL as much as possible and to have a better control over what is happening in the mcu in general. In general, one can ignore this code and replace everything with the CubeMx-generated clock config, it should also work (just replace the Delay_ms with HAL_Delay where necessary).

I suppose the only thing that must be said that APB1 and APB2 must be both 16 MHz and AHB1 32 MHz to properly clock all peripherals All in all, every peripheral is clocked connected to either APB1, APB2 or AHB1 with many of them having their own prescalers, decreasing the speed of the peripheral further. Pay attention that timers (that includes the timer for the UART badu rate by the way) usually have a two times multiplier instead on the APB clock they are attached to. In order though to have proper output, all these clockings must be understood, tracked and set. Deviation could completely mess up the harmony between elements!

(Note: in the meantime, I made a ClockDriver project that somewhat covers this additional section.)
