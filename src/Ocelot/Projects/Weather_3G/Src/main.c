
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
#include "ssGNSS.h"
#include <ssLogging.h>
#include "cmsis_os.h"

#include "sys/socket.h"
#include "netinet/in.h"


#define GNSS_UART                       USART1
#define REC_BUFF_LEN 128

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;
UART_HandleTypeDef huart6;

int32_t stdin_uart_handle;
int32_t stdout_uart_handle;
int32_t stderr_uart_handle;

osThreadId sendTaskHandle;
osThreadId rcvTaskHandle;
osThreadId gpsTaskHandle;
osThreadId ledTaskHandle;
osThreadId tempHumTaskHandle;
osThreadId watchdogTaskHandle;

static I2C_HandleTypeDef hi2c2;

static int32_t si7021_handle;
char str_buff[50];

const char cliAppName[] = "ModemPassthrough";

typedef enum SystemStatus {

	NWK_STARTUP = 0,
	NWK_CONNECTED

}NetworkStatus_t;

NetworkStatus_t network_status = NWK_STARTUP;

typedef enum GpsStatus {

	GPS_STARTUP = 0,
	GPS_FIX

}GpsStatus_t;

GpsStatus_t gps_status = GPS_STARTUP;


//socket buffers
char udp_buf[128];
char rec_buf[REC_BUFF_LEN];
int8_t udp_sock = -1;
struct addrinfo *res_udp;
struct ssCoord_time gps_value;
int16_t temperature = 0;
int16_t humidity = 0;


/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

void SendTask(void const * argument);
void ReceiveTask(void const * argument);
void GPSTask(void const * argument);
void LedTask(void const * argument);
void TempHumTask(void const * argument);
void WatchdogTask(void const * argument);

void stdio_init(void)
{
  ssUartConfigType config;

  config.baudrate = STDOUT_UART_BAUDRATE;
  config.FlowControl = UART_HWCONTROL_NONE;
  config.Mode = UART_MODE_TX_RX;
  config.Parity = UART_PARITY_NONE;
  config.StopBits = UART_STOPBITS_1;
  config.WordLength = UART_WORDLENGTH_8B;

  stdout_uart_handle = ssUartOpen(STDOUT_UART, &config, STDOUT_USART_BUFFER_SIZE);
  configASSERT(stdout_uart_handle >= 0);
  stderr_uart_handle = stdout_uart_handle;
}

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

  ssUartInit(BSP_UART_COUNT);
  ssLoggingInit();
  stdio_init();


  osThreadDef(watchdogTask, WatchdogTask, osPriorityHigh, 0, 240);
  watchdogTaskHandle = osThreadCreate(osThread(watchdogTask), NULL);

  osThreadDef(gpsTask, GPSTask, osPriorityNormal, 0, 512);
  gpsTaskHandle = osThreadCreate(osThread(gpsTask), NULL);

  osThreadDef(tempHumTask, TempHumTask, osPriorityLow, 0, 256);
  tempHumTaskHandle = osThreadCreate(osThread(tempHumTask), NULL);

  osThreadDef(sendTask, SendTask, osPriorityNormal, 0, 512);
  sendTaskHandle = osThreadCreate(osThread(sendTask), NULL);

  osThreadDef(ledTask, LedTask, osPriorityLow, 0, 64);
  sendTaskHandle = osThreadCreate(osThread(ledTask), NULL);

  /* Start scheduler */
  osKernelStart();
  
  /* We should never get here as control is now taken by the scheduler */

  while (1)	{

  }
}



void SendTask(void const * argument) {

	ssUartConfigType config;


	int8_t send_res = 0;


  
	ssLoggingPrint(ESsLoggingLevel_Info, 0, "Starting default task");

	struct addrinfo hints;
	const char serverURL[] = "udp.smart-sense.hr";
	const char serverPort[] = "10004";

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;

	/* Resolve IP address, automatically starts the modem */
	getaddrinfo(serverURL, serverPort, &hints, &res_udp);

	ssLoggingPrint(ESsLoggingLevel_Info, 0, "Opening client socket...");
	udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if(udp_sock >= 0) {

		network_status = NWK_CONNECTED;
	}

	osThreadDef(rcvTask, ReceiveTask, osPriorityNormal, 0, 512);
	rcvTaskHandle = osThreadCreate(osThread(rcvTask), NULL);



	/* Infinite loop, read sensors and send every 5 seconds */
	while(1) {

		sprintf(udp_buf, "temp=%d.%d, hum=%d.%d, lat=%2.6f, lon=%2.6f", temperature/100, temperature % 100, humidity/100, humidity % 100,gps_value.coords.lat, gps_value.coords.lon );

		//send to server
		send_res = sendto(udp_sock, udp_buf, strlen(udp_buf), 0, (const struct sockaddr *)(res_udp->ai_addr), sizeof(res_udp->ai_addr));

		if (send_res != -1) {
			ssLoggingPrint(ESsLoggingLevel_Debug, 0, "UDP Sent: %s", udp_buf);
		}
		else {
			ssLoggingPrint(ESsLoggingLevel_Debug, 0, "UDP ERROR with sending");
		}

		osDelay(5000);
	}
}

void ReceiveTask(void const * argument) {

  int8_t rec_res = 0;

  while (1) {

	  rec_res = recvfrom(udp_sock, rec_buf, REC_BUFF_LEN, 0,  (const struct sockaddr *)(res_udp->ai_addr), sizeof(res_udp->ai_addr));

	  if (rec_res >= 0) {
		  ssLoggingPrint(ESsLoggingLevel_Debug, 0, "UDP Received: %s", rec_buf);
	  }
	  else if (rec_res == -1) {
		  ssLoggingPrint(ESsLoggingLevel_Debug, 0, "UDP ERROR with receving");
	  }
	  vTaskDelay( 250 / portTICK_RATE_MS );
  }
}


/* StartDefaultTask function */
void GPSTask(void const * argument) {

	gps_value.coords.lat = 0;
	gps_value.coords.lon = 0;

	// initialize GPS
	ssLoggingPrint(ESsLoggingLevel_Info, 0, "Initializing GPS receiver...");
	ssGNSSinit(GNSS_UART, 0);

	while(1) {

		ssGNSSGetCoords(&(gps_value.coords));

		ssLoggingPrint(ESsLoggingLevel_Info, 0, "Position: lat=%2.6f, lon=%2.6f", gps_value.coords.lat, gps_value.coords.lon);

		// pulse blue LED  every time we receive fix
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_11, GPIO_PIN_SET); //blue LED ON
		osDelay(500);
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_11, GPIO_PIN_RESET); //blue LED OFF

		// quick and dirty
		if (gps_value.coords.lat != 0) {

			gps_status = GPS_FIX;
		}


    /*
	HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_12);  // LED1_A
    osDelay(1000);
    HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_13);  // LED1_B
    osDelay(1000);
    */
	}
}

/* StartDefaultTask function */
void LedTask(void const * argument) {


	while(1) {

		switch(network_status) {

		case NWK_STARTUP:

			// Switch on RED LED, no blinking
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_13, GPIO_PIN_SET);
			//HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_12);
			break;

		case NWK_CONNECTED:

			//green LED
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_13, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_12, GPIO_PIN_SET);

			break;
		}
	}
}

// Task for reading temperature and humidity
void TempHumTask(void const * argument) {

	gps_value.coords.lat = 0;
	gps_value.coords.lon = 0;

	// initialize GPS
	ssLoggingPrint(ESsLoggingLevel_Info, 0, "Initializing Temperature and Humidity sensor...");

	  // initialize i2c - temperature and humidity sensor uses i2c
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



	while(1) {


		// read temperature
		if(ssSi70xx_ReadTemperature(si7021_handle, 100, &temperature) == SS_SI70XX_OK) {

			ssLoggingPrint(ESsLoggingLevel_Info, 0, "Temperature: %d.%d C\n\r", temperature/100, temperature % 100);
		}

		// read humidity
		if(ssSi70xx_ReadHumidity(si7021_handle, 100, &humidity) == SS_SI70XX_OK) {

			ssLoggingPrint(ESsLoggingLevel_Info, 0, "Humidity: %d.%d C\n\r", humidity/100, humidity % 100);
		}

	}
}

/* StartDefaultTask function */
void WatchdogTask(void const * argument)
{

  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
    HAL_GPIO_TogglePin(BSP_WD_GPIO_PORT, BSP_WD_PIN);
    //HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_11); //LED0
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
