/*
 * state.h
 *
 *  Created on: Sep 12, 2026
 *      Author: binhvt22
 */

#ifndef MOTORAPP_H_
#define MOTORAPP_H_

#include <stdint.h>

// QUẢN LÝ CÁC MODE ĐIỀU KHIỂN
typedef enum{

	CALIB_ENCODER_MODE = 0,								// MODE CHẠY CALIBRATION ENCODER

	CURRENT_CTRL_MODE,									// MODE ĐIỀU KHIỂN DÒNG (TORQUE)

	SPEED_CTRL_MODE,									// MODE ĐIỀU KHIỂN VẬN TỐC

	POS_CTRL_MODE,										// MODE ĐIỀU KHIỂN VỊ TRÍ

	MIT_CTRL_MODE,										// MODE ĐIỀU KHIỂN MIT

	OPEN_LOOP_CTRL_VOLTAGE								// MODE ĐIỀU KHIỂN VÒNG HỞ ÁP

}ctrl_mode_t;

// QUẢN LÝ CÁC LỆNH TỪ NGƯỜI DÙNG
typedef struct{

	ctrl_mode_t user_cmd_mode;							// MODE ĐIỀU KHIỂN YÊU CẦU TỪ NGƯỜI DÙNG

    float id_ref;										// CURRENT D REFERENCE

    float iq_ref;										// CURRENT Q REFERENCE

    float rpm_ref;										// SPEED REFERENCE

    float pos_ref_deg;									// POSITION (ANGLE) REFERENCE (AFTER GEAR), ĐƠN VỊ ĐỘ

    float vq_ref;										// VQ REFERENCE FOR OPEN LOOP CONTROL

    float vd_ref;										// VD REFERENCE FOR OPEN LOOP CONTROL

    float theta_e_ref;									// THETA ELECTRICAL REFERENCE FOR OPEN LOOP CONTROL

}user_command_t;

// QUẢN LÝ CÁC TRẠNG THÁI CỦA HỆ THỐNG
typedef enum{

	SYS_IDLE = 0,										// TRẠNG THÁI CHỜ

	SYS_START_UP,										// TRẠNG THÁI KHỞI ĐỘNG, HOMING KHI HỆ THỐNG BẮT ĐẦU

	SYS_READY,											// TRẠNG THÁI SẴN SÀNG CHAỴ

	SYS_RUNNING,										// TRẠNG THÁI CHẠY, THỰC HIỆN CÁC

	SYS_FAULT,											// TRẠNG THÁI LỖI

}sys_state_t;

// QUẢN LÝ CÁC STATE KHI KHỞI ĐỘNG
typedef enum{
	
	POWER_UP,											// TRẠNG THÁI BẬT NGUỒN, INIT VÀ KHỞI ĐỘNG FOC, CONTROLLER

	CURRENT_OFFSET,										// XÁC ĐỊNH CURRENT OFFSET

	WAIT_ENCODER,										// WAIT FOR FIRST VALID ENCODER FRAME

	HOMING, 											// HOMING CƠ KHÍ

	START_UP_DONE										// DONE START UP

}start_up_stage_t;

//QUẢN LÝ CÁC CỜ TRẠNG THÁI CỦA HỆ THỐNG
typedef struct{

	uint8_t start_req;									// YÊU CẦU BẮT ĐẦU CHƯƠNG TRÌNH

	uint8_t run_req;									// YÊU CẦU CHẠY

	uint8_t stop_req;									// YÊU CẦU DỪNG

	uint8_t mcu_reset_req;								// YÊU CẦU RESET MCU

	uint8_t change_mode;								// CỜ ĐỔI MODE

	uint8_t current_offset_done;						// CỜ OFFSET DÒNG DONE

	uint8_t homing_done;								// CỜ HOMING DONE
	
	uint8_t clear_fault_done;							// CỜ CLEAR FAULT DONE

}flag_t;


#endif /* MOTORAPP_H_ */
