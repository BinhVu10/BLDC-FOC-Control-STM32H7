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
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <foc.h>
#include <foc_math.h>
#include "arm_math.h"
#include <gpio_port.h>
#include <pwm_port.h>
#include "current_sensor.h"
#include <config.h>
#include "Calib.h"
#include "encoder_mt6826.h"
#include "adrc.h"
#include "uart_logger.h"
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

	// KHỞI TẠO BIẾN, STATE , STRUCT FOC
	volatile uint32_t foc_elapsed_time = 0;

	volatile uint32_t spi3_elapsed_time = 0;

	volatile float current_homing = 0;

	uint32_t homing_start_tick = 0U;

	uint32_t homing_stall_start_tick = 0U;

	foc_t foc;																	// MANAGE STATES FOC

	current_sensor_t current_sensor;											// MANAGE STATE FEEDBACK DÒNG

	encoder_t encoder;															// MANAGE FEEDBACK ENCODER

	// KHỞI TẠO STRUCT MANAGE

	volatile sys_state_t sys_state = SYS_IDLE;									// MANAGE STATE PROGRAM

	volatile ctrl_mode_t ctrl_mode = POS_CTRL_MODE;								// MANAGE MODE CONTROL (DEFAULT LÀ POSITION )

	volatile user_command_t user_command = {									// MANAGE USER COMMAND (HIGH LEVEL)

		.user_cmd_mode = POS_CTRL_MODE,

		.pos_ref_deg = 0.0f,

		.id_ref = 0.0f,

		.iq_ref = 0.0f,

		.rpm_ref = 0.0f,

		.vd_ref = 0.0f,

		.vq_ref = 0.0f,

		.theta_e_ref = 0.0f
	};

	// KHỞI TẠO STRUCT MANAGE FLAG
	volatile flag_t sys_flag = { .start_req = 1U, .run_req = 0U, .stop_req = 0U, .mcu_reset_req = 0U,

								.change_mode = 0U, .current_offset_done = 0, .homing_done = 0U, .calib_encoder_offet_done = 0U};

	// KHỞI TẠO START UP STAGE
	volatile start_up_stage_t start_up_stage = POWER_UP;


	// KHỞI TẠO ADRC CONTROLLER
	LADRC_t ladrc;


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
  MX_TIM7_Init();
  MX_UART8_Init();
  /* USER CODE BEGIN 2 */

//  foc_voltage_bus_read(&foc);
//
//  run_calib_encoder(&foc, &encoder, CALIB_CYCLE);

  uint32_t start_time = HAL_GetTick();

  /* USER CODE END 2 */

  /* Infinite loop */

  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  // LED BLINKING JUST FOR VISUALIZE 1 secs
	  if(HAL_GetTick() - start_time >= 1500U){

		  HAL_GPIO_TogglePin(LED_STATUS_GPIO_Port, LED_STATUS_Pin);					// Nháy led

		  start_time = HAL_GetTick();

	  }

	  //YÊU CẦU RESET MCU
	  if(sys_flag.mcu_reset_req == 1){

		  NVIC_SystemReset();

	  }

	  switch(sys_state){

	  	  // IDLE: KHÔNG LÀM GÌ CẢ (HIỆN TẠI CHƯA DEVELOP CASE NÀY)
		  case SYS_IDLE:

			  // NẾU CHƯA START THÌ TẮT DRIVER
			  if(sys_flag.start_req != 1){

				  drv_disable();												// TẮT DRIVER

				  pwm_set(1000, 1000, 1000);									// KHÔNG CHẠY MOTOR

				  sys_state = SYS_IDLE;											// CHƯA START THÌ VẪN Ở IDLE

				  break;
			  }

			  sys_flag.start_req = 0U;											// RESET FLAG

			  start_up_stage = POWER_UP;								      	// SET STAGE POWER UP

			  sys_state = SYS_START_UP;								      		// MOVE TO START UP STATE

			  break;

		 // MODE START UP KHI BẬT NGUỒN
		  case SYS_START_UP:

			  switch(start_up_stage){

			  	  // BẬT DRIVER, INIT FOC, CONTROLLER, ADC, PWM
			  	  case POWER_UP:

			  		  encoder_init(&encoder);													// INIT ENCODER

			  		  current_homing = 0.0f;													// RESET DÒNG HOME

			  		  homing_start_tick = 0U;													// RESET THỜI GIAN HOME

			  		  homing_stall_start_tick = 0U;

			  		  foc_init(&foc);															// KHỞI TẠO FOC

					  ladrc_init(&ladrc, CURRENT_MAX, -CURRENT_MAX);							// KHỞI TẠO ADRC SPEED CONTROLLER

			  		  // SLAVE INIT (CHƯA DEVELOP)

			  		  current_sensor_init(&current_sensor);										// KHỞI TẠO CURRENT SENSOR

			  		  drv_enable();																// BẬT DRIVER CURRENT

			  		  adc_start();																// BẬT ADC INJECTED

			  		  pwm_start();																// BẬT TIMER TẠO TRGO, DUTY 3 PHA BẰNG 50%

			  		  // SET DUTY 3 PHA BẰNG 50% NHẰM CALIB CURRENT SENSOR
			  		  pwm_set((uint32_t)  htim1.Init.Period / 2U,

			  				  (uint32_t) htim1.Init.Period / 2U,

			  				  (uint32_t) htim1.Init.Period / 2U);

			  		  start_up_stage = CURRENT_OFFSET;

			  		  sys_flag.current_offset_done = 0U;

			  		  sys_flag.homing_done = 0U;

			  		  break;

			  	 // CALIB CURRENT OFFSET
			  	  case CURRENT_OFFSET:

			  		  // OFFSET DÒNG SẼ LẤY TRONG NGẮT ADC INJECTED
			  		  if(sys_flag.current_offset_done != 1){

			  			  break;

			  		  }

			  		  foc.offset_current_a = current_sensor.offset_a;

			  		  foc.offset_current_b = current_sensor.offset_b;

			  		  start_up_stage = WAIT_ENCODER;

			  		  break;

			  	 // CHỜ FRAME ENCODER HỢP LỆ ĐẦU TIÊN
			  	  case WAIT_ENCODER:

			  		  // CHỜ FIRST VALUE CỦA ENCODER
			  		  if(encoder.angle_initialized == 0U){

			  			  break;

			  		  }

					  sys_flag.calib_encoder_offet_done = 0;

					  start_up_stage = AUTO_CALIB;

					  break;

				  case AUTO_CALIB:
				  {
					  foc_off(&foc);

					  ladrc_reset(&ladrc);

					  // ADC ISR KHÔNG START DMA MỚI; CHỜ FRAME ĐANG CHẠY KẾT THÚC.
					  uint32_t dma_wait_tick = HAL_GetTick();

					  while (encoder.dma_busy && (HAL_GetTick() - dma_wait_tick < 10U)) {

					  }

					  if (encoder.dma_busy || !run_calib_encoder(&foc, &encoder, CALIB_CYCLE)) {

						  foc_off(&foc);

						  drv_disable();

						  sys_state = SYS_FAULT;

						  break;
					  }

					  // BỎ DELTA TÍCH LŨY TỪ CÁC LẦN ĐỌC POLLING TRƯỚC KHI ĐO TỐC ĐỘ LẠI.
					  encoder.angle_delta_sum = 0.0f;

					  encoder.speed_cnt = 0U;

					  encoder.speed_after_gear = 0.0f;

					  encoder.speed_filter_after_gear = 0.0f;

					  sys_flag.calib_encoder_offet_done = 1;

			  		  current_homing = 0.0f;

			  		  homing_start_tick = 0U;

			  		  homing_stall_start_tick = 0U;

			  		  start_up_stage = HOMING;

			  		  break;
				  }

			  	 // HOMING MECHANICAL
			  	  case HOMING:

			  		  uint32_t now = HAL_GetTick();

			  		  if(homing_start_tick == 0U){

			  			  homing_start_tick = now;

			  		  }

			  		// KHỞI TẠO DÒNG HOME
			  		  if(MOTOR_TYPE == 1)

			  			  current_homing = CURRENT_HOME;						// BƠM DÒNG DƯƠNG �?Ể HOME NẾU TYPE 1

			  		  else

			  			  current_homing = - CURRENT_HOME;						// BƠM DÒNG ÂM FOR HOME NẾU KHÔNG PHẢI TYPE 1


			  		// VERIFY TOUCH THE MECHANICAL LIMIT (VỀ HOME)
			  		  if((now - homing_start_tick) >= HOMING_DRIVE_TIME_MS){

			  			  float speed_abs = encoder.speed_filter_after_gear;

			  			  if(speed_abs < 0.0f){

			  				  speed_abs = -speed_abs;

			  			  }

			  			  if(speed_abs <= HOMING_SPEED_DEADBAND_RPM){

			  				  if(homing_stall_start_tick == 0U){

			  					  homing_stall_start_tick = now;

			  				  }
			  				  else if((now - homing_stall_start_tick) >= HOMING_STALL_TIME_MS){

			  					  sys_flag.homing_done = 1U;

			  				  }
			  			  }

			  			  else{

			  				  homing_stall_start_tick = 0U;

			  			  }
			  		  }

			  		  // UPDATE GÓC HOME KHI HOMING DONE
			  		  if(sys_flag.homing_done == 1){

			  			  sys_flag.homing_done = 0U;

			  			  encoder.mechanical_zero_angle = encoder.multi_angle_m ;

			  			  encoder.mechanical_angle = 0.0f;

			  			  encoder.mechanical_angle_deg = 0.0f;

			  			  encoder.angle_delta_sum = 0.0f;

			  			  encoder.speed_cnt = 0U;

			  			  encoder.speed_after_gear = 0.0f;

			  			  encoder.speed_filter_after_gear = 0.0f;

			  			  current_homing = 0.0f;

			  			  // KHỞI TẠO GIỚI HẠN MECHANICAL

			  			  start_up_stage = START_UP_DONE;

			  			  break;

			  		  }

			  		  break;

			  	 // START_UP DONE
			  	  case START_UP_DONE:

			  		  start_up_stage = START_UP_DONE;

			  		  sys_state = SYS_READY;

			  		  HAL_Delay(200);														// DELAY 200MS FOR STABILIZING

			  		  break;

			  }

			  break;

		 // MODE READY: SẴN SÀNG CHẠY KHI CÓ YÊU CẦU
		  case SYS_READY:

			  if(sys_flag.stop_req == 1){

				  sys_flag.stop_req = 0 ;

				  foc_off(&foc);															 // TẮT FOC

				  sys_state = SYS_READY;													 // STAY READY

				  break;
			  }

			  if(sys_flag.run_req == 1 || sys_flag.change_mode == 1){

				  sys_flag.change_mode = 0;										              // RESET CỜ

				  sys_flag.run_req = 0 ;									  	              // RESET CỜ

				  // RESET TOÀN BỘ CONTROL STATE TRƯỚC KHI CHẠY MODE MỚI
				  foc_off(&foc);

				  ladrc_reset(&ladrc);

				  ctrl_mode = user_command.user_cmd_mode;

				  sys_state = SYS_RUNNING;									    		     // CONVERT QUA MODE RUNNING

				  HAL_Delay(200);															 // DELAY FOR STABILIZING

				  break;
			  }

			  break;

	  	  // RUNNING: MODE CONTROL CHẠY
	  	  case SYS_RUNNING:

	  		 // KHI CÓ YÊU CẦU DỪNG
	  		  if(sys_flag.stop_req == 1){

	  			  sys_flag.stop_req = 0;													// RESET CỜ

	  			  sys_state = SYS_READY;												    // STOP CONTROL VÀ QUAY VỀ READY

	  			  break;
	  		  }

	  		  // KHI RUNNING MÀ NGƯỜI DÙNG CONVERT MODE (SAU NÀY SẼ CÓ THỂ THAY �?ỔI KHI THỰC HIỆN MASTER SPI)
	  		  if(ctrl_mode != user_command.user_cmd_mode){

	  			  // DỪNG CONTROL TRƯỚC; ADC ISR SẼ CHƯA  OUTPUT VỀ 0
	  			  sys_state = SYS_READY;

	  			  sys_flag.change_mode = 1;

	  			  // SET POSITION , SPEED , DÒNG HIỆN TẠI LÀ �?IỂM BẮT �?ẦU NẾU �?ỔI MODE
	  			  user_command.pos_ref_deg = encoder.mechanical_angle_deg;

	  			  user_command.rpm_ref = 0.0f;

	  			  user_command.id_ref = 0.0f;

	  			  user_command.iq_ref = 0.0f;

	  			  break;
	  		  }

	  		  break;

          // FAULT: KHI XẢY RA FAULT (CHƯA DEVELOP)
	  	  case SYS_FAULT:

	  		  break;

	  	  default:

	  		  // STOP MOTOR
	  		  pwm_set((uint32_t)  htim1.Init.Period / 2U, (uint32_t) htim1.Init.Period / 2U, (uint32_t) htim1.Init.Period / 2U);

	  		  break;
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

		// KÉO CHÂN LÊN 1 END TRUYỀN SPI DMA
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

	// NẾU MÀ CHƯA GET OFFSET XONG THÌ SẼ LẤY OFFSET (SẼ CHƯA  LÊN CASE START_UP)
    if (sys_flag.current_offset_done == 0) {

      sys_flag.current_offset_done = current_sensor_get_offset(&current_sensor);

      return;
    }

    //VOLTAGE BUS READ
	 foc_voltage_bus_read(&foc);

	 // READ CURRENT
	 current_sensor_read(&foc, &current_sensor);

	 // CALIB GIỮ QUYỀN ĐIỀU KHIỂN PWM VÀ SPI POLLING; ADC VẪN ĐO DÒNG/VBUS.
	 if ((sys_state == SYS_START_UP) && (start_up_stage == AUTO_CALIB)) {

		 return;

	 }

	 encoder_Update_Speed(&encoder);

	 encoder_Start_Read_DMA(&encoder);

    // RUNNING: DISPATCH MODE CONTROL
    if (sys_state == SYS_RUNNING) {

		switch (ctrl_mode) {

			case POS_CTRL_MODE:
			{

				// BẢO VỆ GÓC TRONG HÀNH TRÌNH MECHANICAL

				float pos_ref_rad = DEG_TO_RAD(user_command.pos_ref_deg);

				pos_ref_rad = CONSTRAIN(pos_ref_rad, ANGLE_MIN, ANGLE_MAX);

				foc_ctrl_pos_run(&foc, &encoder, pos_ref_rad);										// RUN POSITION CONTROLLER

				break;
			}

			case SPEED_CTRL_MODE:

				// BẢO VỆ SPEED TRONG DẢI HOAT ĐỘNG

				if(user_command.rpm_ref > SPEED_LIMIT){

					user_command.rpm_ref = SPEED_LIMIT;

				}

				if (user_command.rpm_ref < -SPEED_LIMIT){

					user_command.rpm_ref = - SPEED_LIMIT;
				}

				if(user_command.pos_ref_deg < ANGLE_MIN){

					user_command.pos_ref_deg = ANGLE_MIN;

				}

				foc_speed_ctrl_run(&foc, &encoder, user_command.rpm_ref);							// RUN SPEED CONTROLLER

//				foc.speed_cnt++;
//
//				if(foc.speed_cnt >= SPEED_DIV){
//
//					// TEST ADRC SPEED CONTROLLER
//
//					foc.iq = ladrc_update(&ladrc, user_command.rpm_ref, encoder.speed_after_gear);
//
//					foc.id = 0 ;
//
//					foc.speed_cnt = 0 ;
//				}
//
//					foc_current_limit_dq(&foc, &foc.id, &foc.iq);
//
//					foc_current_ctrl_run(&foc, &encoder, foc.id, foc.iq);

				break;

			case CURRENT_CTRL_MODE:
			{

				float id_ref = user_command.id_ref;

				float iq_ref = user_command.iq_ref;

				foc_current_limit_dq(&foc, &id_ref, &iq_ref);										// SATURATION DÒNG

				foc.id = id_ref;

				foc.iq = iq_ref;

				foc_current_ctrl_run(&foc, &encoder, foc.id, foc.iq);								// RUN CURRENT CONTROLLER

				break;
			}

			case OPEN_LOOP_CTRL_VOLTAGE:
			{
				// TÍNH DÒNG FEEDBACK ID IQ FOR LOG DATA TUNNING KHI CẦN

				const float sin_theta = arm_sin_f32(encoder.theta_e);

				const float cos_theta = arm_cos_f32(encoder.theta_e);

				clarke_park_transform(foc.ia_fb, foc.ib_fb, foc.ic_fb, sin_theta, cos_theta, &foc.id_fb, &foc.iq_fb);

				foc_open_loop_control(&foc, user_command.vd_ref, user_command.vq_ref, user_command.theta_e_ref);

				break;
			}

			case CALIB_ENCODER_MODE:																// DEVELOP SAU

			case MIT_CTRL_MODE:																		// DEVELOP SAU

			default:

				foc_off(&foc);																		// TẮT MOTOR, RESET BỘ CONTROL

				ladrc_reset(&ladrc);

				break;
		}
    }

    // START_UP/HOMING: CHỈ CHẠY VÒNG DÒNG SAU KHI ENCODER HỢP LỆ.
    else if ((sys_state == SYS_START_UP) && (start_up_stage == HOMING)) {

		if (encoder.angle_initialized == false) {

			foc_off(&foc);

			return;
		}

		foc_current_ctrl_run(&foc, &encoder, 0.0f, current_homing);

    }

    // STATE/STAGE CÒN LẠI KHÔNG CONTROL
    else {

		  foc_off(&foc);

		  ladrc_reset(&ladrc);

		  return;

    }

    foc_elapsed_time = __HAL_TIM_GET_COUNTER(&htim2);

  }
}

// NGẮT TIMER
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM7){

    	float log_data[UART_LOG_N];

    	// INIT ALL DATA TO NaN
    	UART_Log_Init(log_data);

		// FEEDBACK AND APPLIED VOLTAGE
		log_data[LOG_IQ]       = foc.iq_fb;

		log_data[LOG_ID]       = foc.id_fb;

		log_data[LOG_VQ]       = foc.vq;

		log_data[LOG_VD]       = foc.vd;

		log_data[LOG_SPEED]    = encoder.speed_filter_after_gear;

		log_data[LOG_POSITION] = encoder.mechanical_angle_deg;

		log_data[LOG_V_BUS] = foc.v_bus;

		log_data[LOG_SYS_STAGE] = sys_state;

		log_data[LOG_BOOT_STAGE] = 0;

		switch (ctrl_mode) {

			case POS_CTRL_MODE:

				log_data[LOG_POSITION_REF] = user_command.pos_ref_deg;

				log_data[LOG_SPEED_REF] = foc.omega_ref_rpm;

				log_data[LOG_IQ_REF] = foc.iq;

				log_data[LOG_ID_REF] = foc.id;

				break;

			case SPEED_CTRL_MODE:

				log_data[LOG_SPEED_REF] = user_command.rpm_ref;

				log_data[LOG_IQ_REF] = foc.iq;

				log_data[LOG_ID_REF] = foc.id;

				break;

			case CURRENT_CTRL_MODE:

				log_data[LOG_IQ_REF] = foc.iq;

				log_data[LOG_ID_REF] = foc.id;

				break;

			case OPEN_LOOP_CTRL_VOLTAGE:

			default:

				break;
		}

    	//SEND UART DMA
    	UART_Log_Send(&huart8, log_data);
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
