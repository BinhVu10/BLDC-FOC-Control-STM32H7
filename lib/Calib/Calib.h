/*
 * Calib.h
 *
 *  Created on: Sep 26, 2026
 *      Author: binhvt22
 */

#ifndef CALIB_CALIB_H_
#define CALIB_CALIB_H_

#include <stdint.h>
#include <stdbool.h>
#include "foc.h"

// GỌI NGOÀI ISR; MODE PHẢI DỪNG CONTROL VÀ DMA TRƯỚC KHI GỌI.
// CYCLE_COUNT >= 2: BỎ CHU KỲ ĐẦU, LẤY TRUNG BÌNH CÁC CHU KỲ CÒN LẠI.
// TRUE: ĐÃ CẬP NHẬT OFFSET; FALSE: GIỮ OFFSET CŨ. KHÔNG CHUYỂN STATE/MODE.
bool run_calib_encoder(foc_t *foc, encoder_t *encoder, uint8_t cycle_count);


#endif /* CALIB_CALIB_H_ */
