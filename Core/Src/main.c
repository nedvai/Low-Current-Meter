/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "ssd1306.h"
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

/* USER CODE BEGIN PV */
#define ADC_MAX        4095.0f
#define ADC_VREF       3.3f

#define R_SHUNT        10.0f

#define GAIN_HIGH      1.47f
#define GAIN_LOW       21.0f

#define SAMPLE_COUNT   200
#define NO_CURRENT_RMS_MA   0.05f
#define NO_CURRENT_AMP_MA   0.08f
uint16_t high_samples[SAMPLE_COUNT];
uint16_t low_samples[SAMPLE_COUNT];

char tx_buffer[128];

float irms_mA = 0.0f;
float iamp_mA = 0.0f;

uint8_t using_low_range = 1;

uint32_t last_oled_update = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void Format_mA(char *text, float value);
void Send_Result_UART(void);
void Send_Result_OLED(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint16_t Read_ADC_Channel(uint32_t channel)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    HAL_ADC_Stop(&hadc);

    /* Disable both channels first */
    sConfig.Rank = ADC_RANK_NONE;

    sConfig.Channel = ADC_CHANNEL_0;
    HAL_ADC_ConfigChannel(&hadc, &sConfig);

    sConfig.Channel = ADC_CHANNEL_1;
    HAL_ADC_ConfigChannel(&hadc, &sConfig);

    /* Enable only selected channel */
    sConfig.Channel = channel;
    sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;

    if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
    {
        return 0;
    }

    if (HAL_ADC_Start(&hadc) != HAL_OK)
    {
        return 0;
    }

    if (HAL_ADC_PollForConversion(&hadc, 100) != HAL_OK)
    {
        HAL_ADC_Stop(&hadc);
        return 0;
    }

    uint16_t value = HAL_ADC_GetValue(&hadc);

    HAL_ADC_Stop(&hadc);

    return value;
}

float my_abs(float x)
{
    if (x < 0.0f)
    {
        return -x;
    }

    return x;
}

float my_sqrt(float x)
{
    if (x <= 0.0f)
    {
        return 0.0f;
    }

    float guess = x;

    for (int i = 0; i < 12; i++)
    {
        guess = 0.5f * (guess + x / guess);
    }

    return guess;
}

void Calculate_Current(void)
{
    uint32_t sum_high = 0;
    uint32_t sum_low = 0;

    uint16_t high_min = 4095;
    uint16_t high_max = 0;

    uint16_t low_min = 4095;
    uint16_t low_max = 0;

    for (int i = 0; i < SAMPLE_COUNT; i++)
    {
        high_samples[i] = Read_ADC_Channel(ADC_CHANNEL_0); // PA0
        low_samples[i]  = Read_ADC_Channel(ADC_CHANNEL_1); // PA1

        sum_high += high_samples[i];
        sum_low  += low_samples[i];

        if (high_samples[i] < high_min) high_min = high_samples[i];
        if (high_samples[i] > high_max) high_max = high_samples[i];

        if (low_samples[i] < low_min) low_min = low_samples[i];
        if (low_samples[i] > low_max) low_max = low_samples[i];
    }

    float offset_high = (float)sum_high / (float)SAMPLE_COUNT;
    float offset_low  = (float)sum_low  / (float)SAMPLE_COUNT;

    float gain;
    float offset;
    uint16_t *selected_samples;

    /*
       LOW range has higher gain, so use it unless it is near clipping.
       If LOW range gets too close to 0 or 4095, switch to HIGH range.
    */
    if (low_min > 200 && low_max < 3895)
    {
        using_low_range = 1;
        gain = GAIN_LOW;
        offset = offset_low;
        selected_samples = low_samples;
    }
    else
    {
        using_low_range = 0;
        gain = GAIN_HIGH;
        offset = offset_high;
        selected_samples = high_samples;
    }

    float sum_square = 0.0f;
    float peak_counts = 0.0f;

    for (int i = 0; i < SAMPLE_COUNT; i++)
    {
        float ac_counts = (float)selected_samples[i] - offset;

        if (my_abs(ac_counts) > peak_counts)
        {
            peak_counts = my_abs(ac_counts);
        }

        float ac_voltage = (ac_counts * ADC_VREF) / ADC_MAX;
        float current_A = ac_voltage / (gain * R_SHUNT);

        sum_square += current_A * current_A;
    }

    float rms_A = my_sqrt(sum_square / (float)SAMPLE_COUNT);

    float amp_voltage = (peak_counts * ADC_VREF) / ADC_MAX;
    float amp_A = amp_voltage / (gain * R_SHUNT);

    irms_mA = rms_A * 1000.0f;
    iamp_mA = amp_A * 1000.0f;

    /* Remove tiny noise near zero */
    if (irms_mA < NO_CURRENT_RMS_MA)
    {
        irms_mA = 0.0f;
    }

    if (iamp_mA < NO_CURRENT_AMP_MA)
    {
        iamp_mA = 0.0f;
    }
}

void Format_mA(char *text, float value)
{
    int value_x100 = (int)(value * 100.0f + 0.5f);

    if (value_x100 < 0)
    {
        value_x100 = 0;
    }

    sprintf(text, "%d.%02d", value_x100 / 100, value_x100 % 100);
}

void Send_Result_UART(void)
{
    char irms_text[16];
    char iamp_text[16];

    Format_mA(irms_text, irms_mA);
    Format_mA(iamp_text, iamp_mA);

    sprintf(tx_buffer,
            "RANGE=%s  IRMS=%s mA  IAMP=%s mA\r\n",
            using_low_range ? "LOW" : "HIGH",
            irms_text,
            iamp_text);

    HAL_UART_Transmit(&huart2,
                      (uint8_t*)tx_buffer,
                      strlen(tx_buffer),
                      100);
}

void Send_Result_OLED(void)
{
    char value_text[16];

    OLED_Clear();

    OLED_SetCursor(0, 0);
    OLED_WriteString("RANGE=");
    OLED_WriteString(using_low_range ? "LOW" : "HIGH");

    OLED_SetCursor(0, 1);
    OLED_WriteString("IRMS=");
    Format_mA(value_text, irms_mA);
    OLED_WriteString(value_text);
    OLED_WriteString("mA");

    OLED_SetCursor(0, 2);
    OLED_WriteString("IAMP=");
    Format_mA(value_text, iamp_mA);
    OLED_WriteString(value_text);
    OLED_WriteString("mA");

    OLED_SetCursor(0, 3);
    OLED_WriteString("AC CURRENT");

    OLED_Update();
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
  MX_ADC_Init();
  MX_I2C2_Init();
  MX_USART2_UART_Init();
  MX_TIM6_Init();
  /* USER CODE BEGIN 2 */
OLED_Init(&hi2c2);

OLED_Clear();
OLED_SetCursor(0, 0);
OLED_WriteString("SROVES NERA");
OLED_SetCursor(0, 2);
OLED_WriteString("PRIJUNKITE");
OLED_Update();

last_oled_update = HAL_GetTick();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	    Calculate_Current();

	    Send_Result_UART();

	    if ((HAL_GetTick() - last_oled_update) >= 1000)
	    {
	        last_oled_update = HAL_GetTick();
	        Send_Result_OLED();
	    }

	    HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);

	    HAL_Delay(200);

	/*	    irms_mA = 1.23f;
    iamp_mA = 1.74f;
    using_low_range = 1;

    Send_Result_OLED();

    HAL_Delay(1000); */
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
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
