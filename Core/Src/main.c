/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define MAX_LED 9

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

TIM_HandleTypeDef htim1;
DMA_HandleTypeDef hdma_tim1_ch1;

/* USER CODE BEGIN PV */



/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM1_Init(void);
static void MX_ADC1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

volatile uint8_t FLAG_ChangePattern = 0;
volatile uint8_t FLAG_DataSent = 0;

uint8_t LED_Data[MAX_LED][3];  // 3 colors
uint16_t pwmData[(24*MAX_LED)+600];  // 600 = 300 zeros before and after actual data

/* Name		set_LED
 * Desc.	!TODO
 * Inputs	LEDnum	(0 to MAX_LED-1)	number of the LED you want to set the color for
 * 			Red		0 to 255			how much of the color you want
 * 			Green	0 to 255			how much of the color you want
 * 			Blue	0 to 255			how much of the color you want
 */
void set_LED (uint8_t LEDnum, uint8_t Red, uint8_t Green, uint8_t Blue) {
	LED_Data[LEDnum][0] = Green;
	LED_Data[LEDnum][1] = Red;
	LED_Data[LEDnum][2] = Blue;
}

void set_all_LED(uint8_t Red, uint8_t Green, uint8_t Blue) {
	for (int i = 0; i < MAX_LED; i++) {
		set_LED(i, Red, Green, Blue);
	}
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
	FLAG_DataSent = 1;
}

void WS2812C_Send(void) {

	uint32_t index = 0;
	uint32_t color;      // color data is 24 bits

	for (int i = 0; i < 300; i++) {
		pwmData[index] = 0;
		index++;
	}

	for (int LED = 0; LED < MAX_LED; LED++) {     // for each LED
		color = ((LED_Data[LED][0] << 16) | (LED_Data[LED][1] << 8) | (LED_Data[LED][2])); // Concantate color values into a single string
		for (int bit = 23; bit >= 0; bit--) {    // for each bit of color values
			if (color & (1 << bit)) {
				pwmData[index] = 30;   // 50% duty cycle
			} else {
				pwmData[index] = 15;   // 25% duty cycle
			}

			index++;
		}
	}

	// send a bunch of 0% duty cycles to keep line low for the latch command
	for (int i = 0; i < 300; i++) {
		pwmData[index] = 0;
		index++;
	}

	// Start DMA and wait until it's done
	HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t*) pwmData, index);
	while (!FLAG_DataSent) {};
	FLAG_DataSent = 0;

}

void cycle_RGB(void) {
	while (1) {
		set_all_LED(255, 0, 0);
		WS2812C_Send();
		HAL_Delay(500);
		if(FLAG_ChangePattern == 1){break;}

		set_all_LED(0, 255, 0);
		WS2812C_Send();
		HAL_Delay(500);
		if(FLAG_ChangePattern == 1){break;}

		set_all_LED(0, 0, 255);
		WS2812C_Send();
		HAL_Delay(500);
		if(FLAG_ChangePattern == 1){break;}
	}
}

// Based on this image https://en.wikipedia.org/wiki/HSL_and_HSV#/media/File:HSV-RGB-comparison.svg
// Hue range is 0 - 1,535 ((256*6)-1)
void HuetoRGB(uint8_t LEDnum, uint16_t Hue) {
	// The rainbow is broken up into 6 bins, defined by when R, G, B start increasing or decreasing

	if(Hue > 1535) {
		Hue = Hue - 1536;
	}

	uint8_t Red = 0;
	uint8_t Green = 0;
	uint8_t Blue = 0;

	uint8_t Bin = Hue / 256; // The rainbow is broken up into 6 bins, defined by when R, G, B start increasing or decreasing
	uint8_t x = Hue % 256;  // How far along the bin you are

	switch (Bin) {
	case 0:
		Red = 255;
		Green = x;
		break;
	case 1:
		Green = 255;
		Red = 255 - x;
		break;
	case 2:
		Green = 255;
		Blue = x;
		break;
	case 3:
		Blue = 255;
		Green = 255 - x;
		break;
	case 4:
		Blue = 255;
		Red = x;
		break;
	case 5:
		Red = 255;
		Blue = 255 - x;
		break;
	default:
		Red = 255;
		Green = 255;
		Blue = 255;
	}

	set_LED(LEDnum, Red, Green, Blue);

}

// implements a rainbow gradient as per
// https://en.wikipedia.org/wiki/HSL_and_HSV#/media/File:HSV-RGB-comparison.svg
void Rainbow(void) {

	// All red as starting point
	uint8_t Red = 255;
	uint8_t Green = 0;
	uint8_t Blue = 0;
	uint32_t delay = 2;

	set_all_LED(Red, Green, Blue);
	WS2812C_Send();

	// R max, G increasing
	Blue = 0;
	for (int i = 0; i < 256; i++) {
		Green = i;
		set_all_LED(Red, Green, Blue);
		WS2812C_Send();
		HAL_Delay(delay);
	}

	// G max, R decreasing
	Blue = 0;
	for (int i = 0; i < 256; i++) {
		Red = 255 - i;
		set_all_LED(Red, Green, Blue);
		WS2812C_Send();
		HAL_Delay(delay);
	}

	// G max, B increasing
	Red = 0;
	for (int i = 0; i < 256; i++) {
		Blue = i;
		set_all_LED(Red, Green, Blue);
		WS2812C_Send();
		HAL_Delay(delay);
	}

	// B max, G decreasing
	Red = 0;
	for (int i = 0; i < 256; i++) {
		Green = 255 - i;
		set_all_LED(Red, Green, Blue);
		WS2812C_Send();
		HAL_Delay(delay);
	}

	// B max, R increasing
	Green = 0;
	for (int i = 0; i < 256; i++) {
		Red = i;
		set_all_LED(Red, Green, Blue);
		WS2812C_Send();
		HAL_Delay(delay);
	}

	// R max, B decreasing
	Green = 0;
	for (int i = 0; i < 256; i++) {
		Blue = 255 - i;
		set_all_LED(Red, Green, Blue);
		WS2812C_Send();
		HAL_Delay(delay);
	}

}

void GradientRainbowDiag(void) {
	while (1) {
		for (uint16_t i = 0; i < 1536; i++) {
			HuetoRGB(2, i + 160);

			HuetoRGB(1, i + 120);
			HuetoRGB(5, i + 120);

			HuetoRGB(0, i + 80);
			HuetoRGB(4, i + 80);
			HuetoRGB(8, i + 80);

			HuetoRGB(3, i + 40);
			HuetoRGB(7, i + 40);

			HuetoRGB(6, i);

			WS2812C_Send();
			HAL_Delay(20);

			if(FLAG_ChangePattern == 1){break;}
		}
		if(FLAG_ChangePattern == 1){break;}
	}
}





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
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM1_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

	  GradientRainbowDiag();
	  FLAG_ChangePattern = 0;
	  cycle_RGB();
	  FLAG_ChangePattern = 0;


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

  __HAL_FLASH_SET_LATENCY(FLASH_LATENCY_1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode = ADC_SCAN_SEQ_FIXED;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.LowPowerAutoPowerOff = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.SamplingTimeCommon1 = ADC_SAMPLETIME_1CYCLE_5;
  hadc1.Init.OversamplingMode = DISABLE;
  hadc1.Init.TriggerFrequencyMode = ADC_TRIGGER_FREQ_HIGH;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_12;
  sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 60-1;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.BreakFilter = 0;
  sBreakDeadTimeConfig.BreakAFMode = TIM_BREAK_AFMODE_INPUT;
  sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;
  sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
  sBreakDeadTimeConfig.Break2Filter = 0;
  sBreakDeadTimeConfig.Break2AFMode = TIM_BREAK_AFMODE_INPUT;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

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
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin : PB7 */
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */


// Code that triggers when the button interrupt happens
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	if (GPIO_Pin == GPIO_PIN_7) {
		// Code for what interrupt does goes here
		// Set the flag to change the pattern. This should exit out of the current infinite while loop.
		FLAG_ChangePattern = 1;
		// Change color pattern, so I guess increment/wrap a (global?) pattern index variable?
	} else {
		__NOP();
	}
}

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
#ifdef USE_FULL_ASSERT
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
