/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  *
  * This is a rework of the previously written UART driver.
  *
  * v1.0: Bootloader for STM32L0xx.
  * Uses UART1 Rx to receive data from master device. (Not Tx included on the STM32 side!)
  * Uses DMA for machine code reception.
  * Uses half-page FLASH burst to update app.
  * If for 5 seconds, not external controller request arrives, bootloader transitions to app.
  * App is to be stored at address 0x8008000 - look for "App_Section_Start_Addr" int eh code to modify it.
  * App and master controller are not provided.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include <BootUARTDriver_STM32L0x3.h>
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "BootIRQ_Control.h"
#include "BootDMADriver_STM32L0x3.h"
#include "BootAppManager.h"
#include "BootExternalController.h"
#include "BootClockDriver_STM32L0x3.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


//printf transition
int _write(int file, char *ptr, int len)
{
	int DataIdx;
	for (DataIdx = 0; DataIdx < len; DataIdx++) {
		HAL_UART_Transmit(&huart2, (uint8_t *)ptr++, 1, 100);
	}
	return len;
}

//blinky LED
void Toggle_other_LED (void) {
		RCC->IOPENR |=	(1<<1);															//PORTB clocking
		GPIOB->MODER |= (1<<6);															//PB3 output
 		GPIOB->MODER &= ~(1<<7);														//PB3 output

 		uint32_t odr = GPIOB->ODR;
 		if (((odr & (1<<3)) == (1<<3))) {												//if PB3 output was HIGH
 		 	GPIOB->BRR |= (1<<3);														//we reset it
 		} else {
 		 	GPIOB->BSRR |= (1<<3);
 		}
}

uint32_t Rx_Message_buf [64];															//buffer is 64 times 4 bytes / 1 word

uint8_t* Rx_Message_buf_ptr;

uint16_t DMA_transfer_width_UART1;

uint8_t page_counter;

uint8_t seconds_counter;

enum_Yes_No_Selector UART1_Message_Received;

enum_Yes_No_Selector UART1_Message_Started;

enum_First_Second_Selector Machine_Code_Page_Received;									//indicator that we have a full page in the Rx buffer ready to be copied into the FLASH
																						//Note: a 2 page long ping-pong buffer captures one page while the other page is being processed

enum_Yes_No_Selector UART1_DMA_active;													//indicator for DMA activity
																						//Note: UART messaging must not occur while DMA is active!!!

uint32_t flash_page_addr;

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
//  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  SysClockConfig();
  TIM6Config();
  BootTIM2_INT();																		//TIM2 init
  BootTIM2IRQPriorEnable();																//TIM2 IRQ
  UART1Config();																		//UART1 init
  UART1IRQPriorEnable();																//UART1 IRQ - enable is done at a different place
  BootDMAInit();																		//DMA init
  BootDMAIRQPriorEnable();																//DMA IRQ - enable is done at a different place

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  flash_page_addr = App_Section_Start_Addr;												//we define the base address where the app is supposed to be
  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  							//mind, the app's machine code has all the placing information. We need to respect it, otherwise we won't find and run the app.

  UART1_Message_Received = No;															//we reset the message received flag
  UART1_Message_Started = No;															//we reset the message started flag
  Machine_Code_Page_Received = None;
  UART1_DMA_active = No;
  page_counter = 0;
  Rx_Message_buf_ptr = Rx_Message_buf;													//we place the buffer loading pointer to the buffer
  DMA_transfer_width_UART1 = 256;														//DMA transfer width is the entirety of the Rx buffer
  memset(Rx_Message_buf, 0, 64);														//we erase the buffer

  enum_Yes_No_Selector External_Controller_Mode = No;									//this is a local variable that should be wiped upon reset

  seconds_counter = 0;

  printf("Bootloader running...\r\n");

  //detect signal calibration
  RCC->IOPENR |=	(1<<1);																//PORTB clocking
  GPIOB->MODER |= (1<<6);																//PB3 output
  GPIOB->MODER &= ~(1<<7);																//PB3 output

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {


	//The following segment ensures that we only switch to external control if a certain byte (0xc3) is received on the UART1 bus.
	//If there is no byte received, a TIM2 IRQ will deinitialize the bootloader and start the app after a certain number of cycles (set in the IRQ header)

	if (External_Controller_Mode == No) {												//if we are not in external controller mode

		UART1RxMessage();																//we listen to the UART bus for the external control byte

		if (Rx_Message_buf[0] == 0xc3) {

		  printf("External controller activated...\r\n");
		  External_Controller_Mode = Yes;												//this flag will be reset upon reboot only
		  BootTIM2_DEINT();																//we completely shut off the TIM2 timer and its IRQ
		  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  		//Note: TIM2 IRQ governs the automatic transition to the app if there is no command byte received

		} else {

			//do nothing

		}

	} else if (External_Controller_Mode == Yes) {

		UART1_External_Boot_Controller();

	} else {

		//do nothing

	}


    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_5;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
