
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************aa
  * This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * Copyright (c) 2018 STMicroelectronics International N.V. 
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "bsp.h"
#include "ssUart.h"
#include "ssCli.h"
#include "CliCommonCmds.h"
#include <string.h>
#include "ssSi70xx.h"

//todo: move to compiler switch
//#define UBLOX_SARA_R410
#define UBLOX_SARA_U270




/* Check sanity of defined symbols */
#if defined(UBLOX_SARA_N211)
  #if defined(UBLOX_SARA_U270) || defined(UBLOX_SARA_R410)
    #error "Multiple modems defined!"
  #endif
#elif defined(UBLOX_SARA_U270)
  #if defined(UBLOX_SARA_N211) || defined(UBLOX_SARA_R410)
    #error "Multiple modems defined!"
  #endif
#elif defined(UBLOX_SARA_R410)
  #if defined(UBLOX_SARA_N211) || defined(UBLOX_SARA_U270)
    #error "Multiple modems defined!"
  #endif
#else
  #error "Modem not defined!"
#endif
    

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;
UART_HandleTypeDef huart6;

int32_t stdin_uart_handle;
int32_t stdout_uart_handle;
int32_t stderr_uart_handle;

osThreadId defaultTaskHandle;
osThreadId uartToModemTaskHandle;
osThreadId modemToUartTaskHandle;
osThreadId ledTaskHandle;
osThreadId watchdogTaskHandle;

static I2C_HandleTypeDef hi2c2;

static int32_t hostuart = -1;
static int32_t mtuart = -1;

static int32_t si7021_handle;
char str_buff[50];

const char cliAppName[] = "ModemPassthrough";


/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void StartDefaultTask(void const * argument);
void UartToModemTask(void const * argument);
void ModemToUartTask(void const * argument);
void LedTask(void const * argument);
void WatchdogTask(void const * argument);



/**
  * @brief  The application entry point.
  *
  * @retval None
  */
int main(void)
{
  /* MCU Configuration----------------------------------------------------------*/
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  BSP_init();
  
  /* Init library modules */
  ssUartInit(BSP_UART_COUNT);
  
  __HAL_RCC_I2C1_CLK_ENABLE();

  hi2c2.Instance = I2C1;
  //hi2c2.Init.Timing = 0x20303E5D;
  hi2c2.Init.ClockSpeed = 100000;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  //hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

  HAL_I2C_Init(&hi2c2);
    /* Configure Analogue filter */
  HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE);

  ssSi70xx_Init();
  si7021_handle = ssSi70xx_Open(&hi2c2);
  //configASSERT(si7021_handle != -1);



  ssCliUartConsoleInit();
  ssCliUartConsoleStart();
  CliCommonCmdsInit();
  
  /* start CLI console */
  //ssCliUartConsoleStart();
  //CliCommonCmdsInit();
    
  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  
  osThreadDef(watchdogTask, WatchdogTask, osPriorityHigh, 0, 240);
  watchdogTaskHandle = osThreadCreate(osThread(watchdogTask), NULL);

  osThreadDef(ledTask, LedTask, osPriorityNormal, 0, 128);
  ledTaskHandle = osThreadCreate(osThread(ledTask), NULL);

  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);





  /* Start scheduler */
  osKernelStart();
  
  /* We should never get here as control is now taken by the scheduler */

  while (1)
  {
  }
}


void UartToModemTask(void const * argument)
{
  uint8_t c;
  
  while(1)
  {
    if(ssUartRead(hostuart, &c, 1, 1000) != 0)
    {
      ssUartPutc(mtuart, c);
    }
  }
}

void ModemToUartTask(void const * argument)
{
  uint8_t c;
  
  while(1)
  {
    if(ssUartRead(mtuart, &c, 1, 1000) != 0)
    {
      ssUartPutc(hostuart, c);
    }
  }
}



void StartDefaultTask(void const * argument)
{ 
  
  ssUartConfigType config;
  
#ifdef UBLOX_SARA_N211
  config.baudrate = 9600;
  config.FlowControl = UART_HWCONTROL_NONE;
#else
  config.baudrate = 115200;
  config.FlowControl = UART_HWCONTROL_RTS_CTS;
#endif
  config.Mode = UART_MODE_TX_RX;
  config.Parity = UART_PARITY_NONE;
  config.StopBits = UART_STOPBITS_1;
  config.WordLength = UART_WORDLENGTH_8B;
  mtuart = ssUartOpen(USART3, &config, 1024);
  //configASSERT(mtuart >= 0);
  

#ifdef UBLOX_SARA_N211
  config.baudrate = 9600;
#else
  config.baudrate = 115200;
#endif
  config.FlowControl = UART_HWCONTROL_NONE;
  config.Mode = UART_MODE_TX_RX;
  config.Parity = UART_PARITY_NONE;
  config.StopBits = UART_STOPBITS_1;
  config.WordLength = UART_WORDLENGTH_8B;
  hostuart = ssUartOpen(USART6, &config, 1024);

  
  
  // power up the modem
#if defined(UBLOX_SARA_N211)
  HAL_GPIO_WritePin(GPIOD, MDM_RESET_Pin, GPIO_PIN_RESET);
  osDelay(100);
  HAL_GPIO_WritePin(GPIOD, MDM_RESET_Pin, GPIO_PIN_SET);
#elif defined(UBLOX_SARA_R410)
  HAL_GPIO_WritePin(GPIOD, MDM_RESET_Pin, GPIO_PIN_SET);
  
  HAL_GPIO_WritePin(MDM_PWR_ON_GPIO_Port, MDM_PWR_ON_Pin, GPIO_PIN_SET);
  osDelay(1000);
  HAL_GPIO_WritePin(MDM_PWR_ON_GPIO_Port, MDM_PWR_ON_Pin, GPIO_PIN_RESET);
  osDelay(1000);
  HAL_GPIO_WritePin(MDM_PWR_ON_GPIO_Port, MDM_PWR_ON_Pin, GPIO_PIN_SET);
  osDelay(250);
#elif defined(UBLOX_SARA_U270)
  HAL_GPIO_WritePin(MDM_RESET_GPIO_Port, MDM_RESET_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(MDM_PWR_ON_GPIO_Port, MDM_PWR_ON_Pin, GPIO_PIN_RESET);
  osDelay(50);
  HAL_GPIO_WritePin(MDM_PWR_ON_GPIO_Port, MDM_PWR_ON_Pin, GPIO_PIN_SET);
  osDelay(10);
  HAL_GPIO_WritePin(MDM_PWR_ON_GPIO_Port, MDM_PWR_ON_Pin, GPIO_PIN_RESET);
  osDelay(150);
  HAL_GPIO_WritePin(MDM_PWR_ON_GPIO_Port, MDM_PWR_ON_Pin, GPIO_PIN_SET);
  osDelay(100);
#endif

  osThreadDef(uartToModemTask, UartToModemTask, osPriorityNormal, 0, 240);
  uartToModemTaskHandle = osThreadCreate(osThread(uartToModemTask), NULL);
  
  osThreadDef(modemToUartTask, ModemToUartTask, osPriorityNormal, 0, 240);
  modemToUartTaskHandle = osThreadCreate(osThread(modemToUartTask), NULL);

  /* Infinite loop */
  for(;;)
  {
    osDelay(1000);

    int16_t temperature;

#read temperature every 5 seconds
    if(ssSi70xx_ReadTemperature(si7021_handle, 100, &temperature) == SS_SI70XX_OK) {

  	  //sprintf(str_buff, "Read: %d.%d C\n\r", temperature/100, temperature % 100);
  	  //ssUartPuts(hostuart, str_buff);
    	//printf("Read: %d.%d C\n\r", temperature/100, temperature % 100);
    }

  }
  /* USER CODE END 5 */ 
}


/* StartDefaultTask function */
void LedTask(void const * argument)
{ 
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
    HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_12);
    osDelay(1000);
    HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_13);
    osDelay(1000);        
  }
  /* USER CODE END 5 */ 
}

/* StartDefaultTask function */
void WatchdogTask(void const * argument)
{

  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
    HAL_GPIO_TogglePin(BSP_WD_GPIO_PORT, BSP_WD_PIN);
    HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_11);
    osDelay(100);
  }
  /* USER CODE END 5 */ 
}



/**
  * @brief  This function is executed in case of error occurrence.
  * @param  file: The file name as string.
  * @param  line: The line in file as a number.
  * @retval None
  */
void _Error_Handler(char *file, int line)
{
  /* User can add his own implementation to report the HAL error return state */
  while(1)
  {
  }
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
