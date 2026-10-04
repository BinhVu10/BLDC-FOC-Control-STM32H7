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
#include "memorymap.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <foc.h>
#include <foc_math.h>
#include <gpio_port.h>
#include <pwm_port.h>
#include "current_sensor.h"
#include <config.h>
#include "Calib.h"
#include "encoder_mt6826.h"

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

	// KHỞI TẠO CÁC BIẾN, TRẠNG THÁI, STRUCT
	volatile uint32_t foc_elapsed_time = 0;

	volatile uint32_t spi3_elapsed_time = 0;

	volatile sys_state_t state;											// QUẢN LÝ STATE PROGRAM

	volatile ctrl_mode_t ctrl_mode;										// QUẢN LÝ MODE CONTROL

	volatile user_command_t user_command = {							// QUẢN LÝ USER COMMAND (HIGH LEVEL)
		.pos_ref_deg = 0.0f,

		.id_ref = 0.0f,

		.id_ref = 0.0f,

		.rpm_ref = 0.0f
	};
	uint8_t new_mode = 0xff;


	foc_t foc;															// QUẢN LÝ STATES FOC

	current_sensor_t current_sensor;									// QUẢN LÝ STATE FEEDBACK DÒNG

	encoder_t encoder;													// QUẢN LÝ FEEDBACK ENCODER

	static volatile bool current_offset_done = false;					// FLAG CHECK OFFSET


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MPU_Config(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  // BẬT I-CACHE
  SCB_EnableICache();

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_ADC2_Init();
  MX_ADC3_Init();
  MX_SPI3_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

  foc_init(&foc);												// KHỞI TẠO FOC

  current_sensor_init(&current_sensor);							// KHỞI TẠO CURRENT SENSOR

  encoder_init(&encoder);										// KHỞI TẠO ENCODER

  // SET DUTY 3 PHA BẰNG 50% NHẰM CALIB CURRENT SENSOR

  drv_enable();													// BẬT DRIVER �?Ể CURRENT SENSE AMPLIFIER HOẠT �?ỘNG

  HAL_Delay(500);												// CURRENT SENSE AMPLIFIER ỔN �?ỊNH

  adc_start();													// BẬT ADC INJECTED

  pwm_start();													// BẬT TIMER TẠO TRGO, DUTY 3 PHA BẰNG 50%

  pwm_set((uint32_t)  htim1.Init.Period / 2U, (uint32_t) htim1.Init.Period / 2U, (uint32_t) htim1.Init.Period / 2U);

  while (!current_offset_done) {

  }

//  foc_voltage_bus_read(&foc);
//
//  run_calib_encoder(&foc, &encoder, CALIB_CYCLE);

//
//  pwm_set(0U, 0U, 0U);


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {


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

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 50;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInitStruct.PLL2.PLL2M = 4;
  PeriphClkInitStruct.PLL2.PLL2N = 10;
  PeriphClkInitStruct.PLL2.PLL2P = 2;
  PeriphClkInitStruct.PLL2.PLL2Q = 2;
  PeriphClkInitStruct.PLL2.PLL2R = 2;
  PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_3;
  PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOMEDIUM;
  PeriphClkInitStruct.PLL2.PLL2FRACN = 0;
  PeriphClkInitStruct.AdcClockSelection = RCC_ADCCLKSOURCE_PLL2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

// HÀM NGẮT SPI
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
	// NGẮT SPI3 LÀ ENCODER DMA, THỜI GIAN HIỆN TẠI LÀ 1-2 US
	if(hspi->Instance == SPI3){

//		  __HAL_TIM_SET_COUNTER(&htim2, 0);

		// KÉO CHÂN LÊN 1 ĐỂ KẾT THÚC TRUYỀN SPI DMA
		HAL_GPIO_WritePin(ENCODER_CS_GPIO_Port, ENCODER_CS_Pin, GPIO_PIN_SET);

		// READ VALUE ENCODER
		encoder_Read_Angle(&encoder);

		encoder.dma_busy = false;

//		spi3_elapsed_time = __HAL_TIM_GET_COUNTER(&htim2);

	}
}

// NẾU LỖI NHẢY VÀO SET ENCODER LỖI
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
	if (hspi->Instance == SPI3) {

		encoder_DMA_Error(&encoder);

	}
}


// NGẮT ADC
void HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef *hadc){

  if (hadc->Instance == ADC1) {

	  __HAL_TIM_SET_COUNTER(&htim2, 0);


	// NẾU MÀ CHƯA GET OFFSET XONG THÌ SẼ LẤY OFFSET
    if (!current_offset_done) {

      current_offset_done = current_sensor_get_offset(&current_sensor);

    }

//      KHI LẤY OFFSET XONG, NGẮT ADC RUN, CHU KỲ FOC HOẠT �?ỘNG
    else {

    	// VOLTAGE BUS READ
    	foc_voltage_bus_read(&foc);

      // READ CURRENT
      current_sensor_read(&foc, &current_sensor);

      // CẬP NHẬT VẬN TỐC
      encoder_Update_Speed(&encoder);

      // BẮT ĐẦU ĐỌC ENCODER DMA
      encoder_Start_Read_DMA(&encoder);

      // CHẠY VÒNG VỊ TRÍ KHI ĐÃ KHỞI TẠO
      if (encoder.is_angle_initlize == true) {

        foc_ctrl_pos_run(&foc, &encoder, DEG_TO_RAD(user_command.pos_ref_deg));

      }

//	  if (encoder.is_angle_initlize == true) {
//
//	    foc_speed_ctrl_run(&foc, &encoder, user_command.rpm_ref);
//	  }

    foc_elapsed_time = __HAL_TIM_GET_COUNTER(&htim2);

    }

  }

}

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

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

}
  /* USER CODE END Error_Handler_Debug */


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
