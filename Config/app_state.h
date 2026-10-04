/*
 * state.h
 *
 *  Created on: Sep 12, 2026
 *      Author: binhvt22
 */

#ifndef APP_STATE_H_
#define APP_STATE_H_

typedef struct{

    float id_ref;							// Current d reference

    float iq_ref;							// Current q reference

    float rpm_ref;							// Speed reference at output shaft (rpm)

    float pos_ref_deg;						// Position reference at output shaft (degree)

}user_command_t;

// QUẢN LÝ CÁC TRẠNG THÁI CỦA HỆ THỐNG
typedef enum{

	SYS_IDLE,								// CHẾ ĐỘ CHỜ

	SYS_START_UP,							// CHẾ ĐỘ KHỞI ĐỘNG, HOMING KHI HỆ THỐNG BẮT ĐẦU

	SYS_RUNNING,							// CHẾ ĐỘ CHẠY, THỰC HIỆN CÁC

	SYS_FAULT								// CHẾ ĐỘ LỖI
}sys_state_t;


// QUẢN LÝ CÁC MODE ĐIỀU KHIỂN
typedef enum{

	CTRL_NONE,									// Chế độ không điều khiển gì (ban đầu mặc định sẽ là như thế)

	CALIB_ENCODER_MODE,							// Mode calib encoder

	CURRENT_CTRL_MODE,							// Mode điều khiển dòng (torque)

	SPEED_CTRL_MODE,							// Mode điều khiển vận tốc

	POS_CTRL_MODE								// Mode điều khiển vị trí

}ctrl_mode_t;



#endif /* APP_STATE_H_ */
