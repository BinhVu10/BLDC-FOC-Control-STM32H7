// THIS FILE CONTAIN THE CONGIGUATION OF THE PROJECT

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/**************************************************************************************************************
 * MOTOR CONFIGURATION
 * */

/*0: động cơ không cơ khí, 1: ngón cái, 2: đầu ngón cái, 3: các ngón còn lại*/
#define MOTOR_TYPE                     0U

#if (MOTOR_TYPE == 1U)                                  		// NGÓN CÁI
#define GEAR_RATIO                     560.0f
#define ANGLE_MAX                      DEG_TO_RAD(-5.0f) 		// GÓC GIỚI HẠN HÀNH TRÌNH MAX
#define ANGLE_MIN					   DEG_TO_RAD(-115.0f)		// GÓC GIỚI HẠN HÀNH TRÌNH MIN
#define POLE_PAIRS                     2.0f            			// CẶP CỰC
#define CURRENT_MAX                    2.5f
#define CURRENT_RATED                  1.5f             		// DÒNG ĐIỆN ĐỊNH MỨC CỦA ĐỘNG CƠ
#define CURRENT_LIMIT_FACTOR           2.5f             		// HỆ SỐ GIỚI HẠN DÒNG
#define ENCODER_OFFSET                 2.77122092f       		// ENCODER OFFSET CỦA ĐỘNG CƠ 6.4
#define CURRENT_GAIN                   0.5f
#define VOLTAGE_LIMIT                  6.0f
#define CURRENT_HOME                   0.6f             		// DÒNG HOMING
#define HOMING_DRIVE_TIME_MS           1000U            		// THỜI GIAN BƠM DÒNG TRƯỚC KHI CHECK HARD STOP
#define HOMING_STALL_TIME_MS           200U             		// THỜI GIAN XÁC NHẬN VẬN TỐC NẰM TRONG DEADBAND
#define HOMING_SPEED_DEADBAND_RPM      0.1f            			// DEADBAND VẬN TỐC SAU HỘP SỐ
#define SPEED_LIMIT                    40.0f            		// TỐC ĐỘ TỐI ĐA (ĐỂ CLAMP ĐẦU RA CỦA BỘ VỊ TRÍ)

#elif (MOTOR_TYPE == 2U)                                		// ĐẦU NGÓN CÁI
#define GEAR_RATIO                     560.0f
#define ANGLE_MAX                      DEG_TO_RAD(55.0f) 		// GÓC GIỚI HẠN HÀNH TRÌNH MAX
#define ANGLE_MIN					   DEG_TO_RAD(5.0f)			// GÓC GIỚI HẠN HÀNH TRÌNH MIN
#define POLE_PAIRS                     2.0f             		// CẶP CỰC
#define CURRENT_MAX                    2.5f
#define CURRENT_RATED                  1.5f             		// DÒNG ĐIỆN ĐỊNH MỨC CỦA ĐỘNG CƠ
#define CURRENT_LIMIT_FACTOR           2.5f             		// HỆ SỐ GIỚI HẠN DÒNG
#define ENCODER_OFFSET                 6.25643396f       		// ENCODER OFFSET CỦA ĐỘNG CƠ 6.5
#define CURRENT_GAIN                   0.5f
#define VOLTAGE_LIMIT                  6.0f
#define CURRENT_HOME                   0.6f             		// DÒNG HOMING
#define HOMING_DRIVE_TIME_MS           1000U            		// THỜI GIAN BƠM DÒNG TRƯỚC KHI CHECK HARD STOP
#define HOMING_STALL_TIME_MS           200U             		// THỜI GIAN XÁC NHẬN VẬN TỐC NẰM TRONG DEADBAND
#define HOMING_SPEED_DEADBAND_RPM      0.1f            			// DEADBAND VẬN TỐC SAU HỘP SỐ
#define SPEED_LIMIT                    40.0f            		// TỐC ĐỘ TỐI ĐA (ĐỂ CLAMP ĐẦU RA CỦA BỘ VỊ TRÍ)

#elif (MOTOR_TYPE == 3U)                                		// CÁC NGÓN GIỮA
#define GEAR_RATIO                     580.0f
#define ANGLE_MAX                      DEG_TO_RAD(115.0f)		// GÓC GIỚI HẠN HÀNH TRÌNH MAX
#define ANGLE_MIN					   DEG_TO_RAD(5.0f)			// GÓC GIỚI HẠN HÀNH TRÌNH MIN
#define CURRENT_MAX                    5.0f
#define CURRENT_RATED                  2.0f            			// DÒNG ĐIỆN ĐỊNH MỨC CỦA ĐỘNG CƠ
#define CURRENT_LIMIT_FACTOR           2.5f             		// HỆ SỐ GIỚI HẠN DÒNG
#define POLE_PAIRS                     1.0f
//#define ENCODER_OFFSET               	4.60894108f       		// ENCODER OFFSET CỦA ĐỘNG CƠ 3.1
//#define ENCODER_OFFSET               5.23159409f       		// ENCODER OFFSET CỦA ĐỘNG CƠ 3.2
//#define ENCODER_OFFSET               4.59657335f       		// ENCODER OFFSET CỦA ĐỘNG CƠ 6.1
//#define ENCODER_OFFSET               3.09240746f       		// ENCODER OFFSET CỦA ĐỘNG CƠ 6.2
//#define ENCODER_OFFSET               1.86234868f       		// ENCODER OFFSET CỦA ĐỘNG CƠ 6.3
//#define ENCODER_OFFSET               5.00894213f      		// ENCODER OFFSET CỦA ĐỘNG CƠ 3.3
//#define ENCODER_OFFSET                 0.684443116f      		// ENCODER OFFSET CỦA ĐỘNG CƠ OK

#define CURRENT_GAIN                   0.5f
#define VOLTAGE_LIMIT                  6.0f
#define CURRENT_HOME                   1.0f             		// DÒNG HOMING
#define HOMING_DRIVE_TIME_MS           1000U            		// THỜI GIAN BƠM DÒNG TRƯỚC KHI CHECK HARD STOP
#define HOMING_STALL_TIME_MS           200U             		// THỜI GIAN XÁC NHẬN VẬN TỐC NẰM TRONG DEADBAND
#define HOMING_SPEED_DEADBAND_RPM      0.1f             		// DEADBAND VẬN TỐC SAU HỘP SỐ
#define SPEED_LIMIT                    30.0f            		// TỐC ĐỘ TỐI ĐA (ĐỂ CLAMP ĐẦU RA CỦA BỘ VỊ TRÍ)


#else                                                   		// LOẠI KHÔNG LẮP CƠ KHÍ TYPE 0
#define POLE_PAIRS                     1.0f
#define GEAR_RATIO                     29.0f
#define CURRENT_MAX                    5.0f
#define CURRENT_RATED                  2.0f             		// DÒNG ĐIỆN ĐỊNH MỨC CỦA ĐỘNG CƠ
#define CURRENT_LIMIT_FACTOR           2.5f             		// HỆ SỐ GIỚI HẠN DÒNG
#define CURRENT_GAIN                   0.5f
#define VOLTAGE_LIMIT                  6.0f
#define CURRENT_HOME                   1.0f             		// DÒNG HOMING
#define HOMING_DRIVE_TIME_MS           1000U            		// THỜI GIAN BƠM DÒNG TRƯỚC KHI CHECK HARD STOP
#define HOMING_STALL_TIME_MS           200U             		// THỜI GIAN XÁC NHẬN VẬN TỐC NẰM TRONG DEADBAND
#define HOMING_SPEED_DEADBAND_RPM      0.1f             		// DEADBAND VẬN TỐC SAU HỘP SỐ
#define SPEED_LIMIT                    18000.0f          		// TỐC ĐỘ TỐI ĐA (ĐỂ CLAMP ĐẦU RA CỦA BỘ VỊ TRÍ)
//#define ENCODER_OFFSET                 0.684443116f      		// ENCODER OFFSET CỦA ĐỘNG CƠ ĐANG DÙNG
#define ENCODER_OFFSET               1.86234868f       			// ENCODER OFFSET CỦA ĐỘNG CƠ 6.3
#define ANGLE_MAX					   99999U					// KHÔNG CHẶN HÀNH TRÌNH
#define ANGLE_MIN					  -99999.0f					// KHÔNG CHẶN HÀNH TRÌNH
#endif

/***************************************************************************************************
 * ENCODER CONFIGURATION
 * */
#define ENCODER_CPR                    32768            		// SỐ XUNG TRÊN 1 VÒNG ENCODER
#define CNT_ENCODER_ERROR_MAX          1000U            		// BỘ ĐẾM LỖI ENCODER

/***************************************************************************************************
 * CONTROL CONFIGURATION
 * */
#define SPEED_BW_HZ					   50.0f											// BĂNG THÔNG VÒNG VẬN TỐC

#define FOC_FREQ_HZ                    20000            								// TẦN SỐ VÒNG DÒNG (FOC), 20KHZ
#define FOC_TS                         (1.0f/(float) FOC_FREQ_HZ)
#define FOC_SPEED_LOOP_HZ              (FOC_FREQ_HZ / 20U) 						 		// TẦN SỐ VÒNG VẬN TỐC, 1KHZ
#define FOC_SPEED_TS                   (1.0f / (float)FOC_SPEED_LOOP_HZ)
#define SPEED_DIV                      ((uint32_t)(FOC_FREQ_HZ / FOC_SPEED_LOOP_HZ))
#define FOC_POS_LOOP_HZ                (FOC_SPEED_LOOP_HZ / 10U) 						// TẦN SỐ VÒNG VỊ TRÍ, 100HZ
#define FOC_POS_TS                     (1.0f / (float)FOC_POS_LOOP_HZ)
#define POS_DIV                        ((uint32_t)(FOC_FREQ_HZ/FOC_POS_LOOP_HZ))

// CONTROLLER PI CURRENT D-Q
#define KP_Iq                          0.0f
#define KI_Iq                          1000.0f
#define KP_Id                          0.0f
#define KI_Id                          2000.0f
#define AW_GAIN_CURRENT                0.1f

// CONTROLLER PI SPEED
//#define KP_SPEED                     1.0f
//#define KI_SPEED                     150.0f
#define KP_SPEED                       0.025f
#define KI_SPEED                       2.5f
#define SPEED_FILTER_ALPHA             0.85f
#define AW_GAIN_SPEED                  0.15f

// CONTROLLER PI POSITION
#define KP_POS                         100.0f
#define KI_POS                         0.0f
#define AW_GAIN_POS                    0.1f

// ADRC SPEED CONTROLLER CONTINOUS TIME
#define ADRC_TS						   FOC_SPEED_TS								// CHU KỲ ĐIỀU KHIỂN ADRC SPEED
#define ESO_BW_HZ					   200.0f									// BĂNG THÔNG BẰNG 1.5 LẦN BĂNG THÔNG VÒNG VẬN TỐC
#define ESO_BW						   TWO_PI*ESO_BW_HZ							// BĂNG THÔNG W_ESO
#define ESO_L1						   30.0f									// GAIN L1 MIỀN LIÊN TỤC
#define ESO_L2						   900.0f									// GAIN L2 MIỀN LIÊN TỤC
#define ADRC_B0					  	   150.0f									// THAM SỐ ĐỐI TƯỢNG
#define ADRC_KP						   TWO_PI*SPEED_BW_HZ						// GAIN BỘ ĐIỀU KHIỂN - CONTROL OUTPUT
#define ADRC_KI						   0.0f										// GAIN BỘ ĐIỀU KHIỂN TÍCH PHÂN
#define ADRC_AW						   0.20f									// ANTI-WINDUP TÍCH PHÂN

// ADRC SPEED CONTROLLER DISCRETE TIME
#define ADRC_DISCRETE_BW_HZ			   SPEED_BW_HZ								// BĂNG THÔNG BỘ ĐIỀU KHIỂN ADRC
#define ADRC_DISCRETE_TS			   FOC_SPEED_TS								// CHU KỲ ĐIỀU KHIỂN ADRC SPEED
#define K_ESO						   3U										// HỆ SỐ TƯƠNG ĐỐI GIỮA BỘ QUAN SÁT VÀ BỘ ĐIỀU KHIỂN
#define ADRC_DISCRETE_B0			   200U										// CRITICAL GAIN
#define ADRC_DISCRETE_KP			   0.0f										// HỆ SỐ KP RỜI RẠC
#define ADRC_DISCRETE_KI			   0.0f										// HẸ SỐ KI RỜI RẠC
#define ESO_L1_DISCRETE				   0.0f										// GAIN L1 MIỀN RỜI RẠC
#define ESO_L2_DISCRETE				   0.0f										// GAIN L2 MIỀN RỜI RẠC
#define ADRC_DISCRETE_AW			   0.15f									// HỆ SỐ ANTI-WINDUP


/************************************************************************************************************
 * ADC CONFIGUARE
 * */
#define ADC_VREF                       3.3f
#define ADC_MAX                        65535.0f         						// 16 BIT
#define ADC_TO_CURRENT                 (ADC_VREF / (ADC_MAX * CURRENT_GAIN))
#define OFFSET_SAMPLE_NUM              4096
#define VBUS_DIV_GAIN                  5.7f            							// GAIN ĐIỆN ÁP TỪ MẠCH CHIA ÁP 47K/10K

/************************************************************************************************************
 * HELPER PARAMETER
 * */
#define CALIB_CYCLE                    2U

#define PI                             3.14159265f
#define TWO_PI                         6.283185307f

#define TWO_BY_SQRT3                   1.15470053838f
#define TWO_BY_THREE                   0.6666666666666667f
#define SQRT3                          1.7320508075688773f
#define SQRT3_BY_TWO                   0.8660254037844386f
#define ONE_BY_SQRT3                   0.57735026919f

#define CONSTRAIN(val, min, max) 	   ((val) <= (min) ? (min) : ((val) >= (max) ? (max) : (val)))

#define MAX(a, b) 					   (((a) > (b)) ? (a) : (b))


#define RAD_TO_DEG(rad)                ((rad) * 57.29577951308232f)  			// 180/PI
#define DEG_TO_RAD(deg)                ((deg) * 0.017453292519943f)  			// PI/180
#define RADS_TO_RPM                    9.54929659f

#define VD_CALIB                       3.0f            							// ĐIỆN ÁP VD ĐỂ CALIB
#define VQ_CALIB                       0.0f

#endif
