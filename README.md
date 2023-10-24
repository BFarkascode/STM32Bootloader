# STM32Bootloader

Arduino-style bootloader for an STM32 L0x3 mcu using bare metal programming.

I made this code because similar bootloader solutions are difficult to come by, use, port or simply understand.

Also they are mostly done with focus on the BluePill (STM32F103) which is different than the L053 I am using.

I want to provide a step-by-step project evolution later to this bootloader project, sharing my thinking so whoever might come across this code (me included in 10 years of time) would have no trouble to replicate (and understand!) the results.

The Bootloader relies very heavilty on the findings of the projects "NVMDriver", "UARTDriver" and the "NVIC_Usecase". By all intents and purposes, the bootloader it is a significantly scaled up (DMA and IRQ-supported) version of those three projects merged together.
