// THIS FILE CONTAIN THE CONGIGUATION OF THE PROJECT

#ifndef CONFIG_H
#define CONFIG_H

/*
 * MOTOR CONFIGUARE
 * */
#include <stdint.h>

#define POLE_PAIRS 				        1
#define GEAR_RATIO 				        29

#define ENCODER_CPR						32768
#define CNT_ENCODER_ERROR_MAX			1000U

//#define ENCODER_OFFSET                  3.25721645f                    					// Encoder offset của động cơ 5.2
//#define ENCODER_OFFSET					2.934792995f
#define ENCODER_OFFSET					0.684443116f										// ENCODER OFFSET CỦA ĐỘNG CƠ OK


#define CURRENT_RATED					2.0f						                         // DÒNG ĐIỆN ĐINH MỨC CỦA ĐỘNG CƠ
#define CURRENT_LIMIT_FACTOR			2.5						    					     // HỆ SỐ GIỚI HẠN DÒNG
#define SPEED_LIMIT						18000.0f										     // TỐC ĐỘ TỐI ĐA (ĐỂ CLAMP ĐẦU RA CỦA BỘ VỊ TRÍ)

#define VD_CALIB						5.0f										         // ĐIỆN ÁP VD ĐỂ CALIB
#define VQ_CALIB						0.0f

/*
 * CONTROL CONFIGUARE
 * */
#define FOC_FREQ_HZ						20000												 // TẦN SỐ VÒNG DÒNG (FOC), 20KHZ
#define FOC_TS						    (1.0f/(float) FOC_FREQ_HZ)							 // CHU KỲ VÒNG DÒNG (FOC)
#define FOC_SPEED_LOOP_HZ                (FOC_FREQ_HZ / 20U)                                 // TẦN SỐ VÒNG VẬN TỐC, 1KHZ
#define FOC_SPEED_TS					(1.0f / (float)FOC_SPEED_LOOP_HZ)					 // CHU KỲ VÒNG VẬN TỐC
#define SPEED_DIV						((uint32_t)(FOC_FREQ_HZ / FOC_SPEED_LOOP_HZ))		 // BIẾN ĐẾM ĐIỀU KHIỂN VÒNG VẬN TỐC
#define FOC_POS_LOOP_HZ                  (FOC_SPEED_LOOP_HZ / 10U)                           // TẦN SỐ VÒNG VỊ TRÍ, 100HZ
#define FOC_POS_TS                       (1.0f / (float)FOC_POS_LOOP_HZ)                     // CHU KỲ LẤY MẪU VÒNG VỊ TRÍ
#define POS_DIV                         ((uint32_t)(FOC_FREQ_HZ/FOC_POS_LOOP_HZ))            // BIẾN ĐẾM ĐIỀU KHIỂN VÒNG VỊ TRÍ


// CONTROLLER PI CURRENT D-Q
#define KP_Iq							0.0f
#define KI_Iq							2000.0f
#define KP_Id							0.0f
#define KI_Id							2000.0f
#define AW_GAIN_CURRENT					0.1f												 // HỆ SỐ ANTI-WINDUP BACK-CALCULATION

// CONTROLLER PI SPEED
//#define KP_SPEED						1.0f
//#define KI_SPEED						150.0f
#define KP_SPEED						0.015f
#define KI_SPEED						2.0f
#define SPEED_FILTER_ALPHA				0.95f
#define AW_GAIN_SPEED					0.15f

// CONTROLLER PI POSITION
#define KP_POS			        		100.0f
#define KI_POS					    	0.0f
#define AW_GAIN_POS				    	0.1f

/*
 * ADC CONFIGUARE
 * */
#define ADC_VREF 						3.3f
#define ADC_MAX						    65535.0f									// 16 BIT
#define GAIN 							0.5f										// V/A
#define ADC_TO_CURRENT                  (ADC_VREF / (ADC_MAX * GAIN))
#define OFFSET_SAMPLE_NUM		        4096										// 4096 MẪU ADC
#define VBUS_DIV_GAIN			        5.7f								       	// GAIN ĐIỆN ÁP TỪ MẠCH CHIA ÁP 47K/10K


// HELPER PARAMETER
#define CALIB_CYCLE                     5U

#define PI				               3.14159265f
#define TWO_PI      					6.283185307f

#define TWO_BY_SQRT3 	               1.15470053838f
#define TWO_BY_THREE                   0.6666666666666667f
#define SQRT3                          1.7320508075688773f
#define SQRT3_BY_TWO                   0.8660254037844386f
#define ONE_BY_SQRT3 	               0.57735026919f

#define CONSTRAIN(val, min, max) \
    ((val) <= (min) ? (min) : ((val) >= (max) ? (max) : (val)))

#define RAD_TO_DEG(rad)                 ((rad) * 57.29577951308232f)  // 180/Π
#define DEG_TO_RAD(deg)                 ((deg) * 0.017453292519943f)  // Π/180
#define RADS_TO_RPM			            9.54929659f

#endif
