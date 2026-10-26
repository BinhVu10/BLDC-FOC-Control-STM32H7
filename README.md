# BLDC/PMSM FOC Control on STM32H743

Firmware điều khiển một động cơ BLDC/PMSM bằng Field-Oriented Control (FOC) trên STM32H743VIH6. Project hiện đã có các vòng điều khiển dòng, tốc độ và vị trí chạy theo ngắt ADC đồng bộ PWM, startup state machine có tự động calibration encoder và homing, chuyển control mode cơ bản, cùng telemetry UART phục vụ tuning. ADRC và PID tổng quát đã có mã nền nhưng chưa tham gia control runtime; bước phát triển tiếp theo là hoàn thiện `SYS_FAULT`, các cơ chế bảo vệ và truyền thông với master.

> Trạng thái tài liệu: đối chiếu với source ngày 2026-10-27. Source hiện tại là căn cứ cuối cùng nếu README và code khác nhau.

## 1. Mục tiêu và ý nghĩa

Project xây dựng firmware điều khiển motor theo hướng bottom-up:

1. Đo dòng, điện áp bus và góc rotor đúng thời điểm.
2. Đóng vòng dòng `Id/Iq` bằng FOC và SVPWM.
3. Đặt vòng tốc độ bên ngoài vòng dòng.
4. Đặt vòng vị trí bên ngoài vòng tốc độ.
5. Phối hợp các vòng bằng control mode và system state rõ ràng.
6. Bổ sung bảo vệ, sau đó kết nối slave với master và PC.

Đây là nền tảng điều khiển cho một slave motor. Kiến trúc hệ thống dài hạn được tham khảo theo chuỗi:

```text
PC <-> RS485 <-> master <-> SPI + DMA <-> slave STM32 <-> driver <-> motor/sensors
```

Repository này mới tập trung vào firmware của một slave STM32 điều khiển một motor. Protocol điều khiển với master/PC chưa được triển khai; UART8 hiện chỉ phát telemetry một chiều để log và tune.

## 2. Trạng thái hiện tại

### Đã có trong source

- Clarke, Park, Clarke-Park và inverse Park transform.
- SVPWM sáu sector, duty được giới hạn trong `[0, 1]`.
- PI controller đang dùng trong cascade control; PID tổng quát và LADRC đã có API riêng.
- Vòng dòng `Id/Iq` ở 20 kHz.
- Vòng tốc độ ở 1 kHz, sinh dòng `Iq` đặt.
- Vòng vị trí ở 100 Hz, sinh tốc độ đặt.
- Giới hạn vector điện áp và vector dòng.
- Đọc hai dòng pha bằng ADC injected conversion; tái tạo pha thứ ba.
- Đọc DC bus bằng ADC3.
- Đọc encoder MT6826S qua SPI3 DMA trong lúc control.
- CRC/status validation, góc cơ, góc điện, multi-turn, góc và tốc độ sau hộp số.
- Calibration offset dòng lúc khởi động.
- Tự động calibration offset encoder bằng SPI polling trong startup.
- State machine `SYS_IDLE -> SYS_START_UP -> SYS_READY -> SYS_RUNNING`.
- Startup theo stage: power-up, current offset, chờ encoder, auto-calibration, homing và hoàn tất.
- Homing cơ khí bằng current control và xác nhận hard stop theo vận tốc.
- Runtime dispatcher cho current, speed và position mode.
- Open-loop voltage mode với reference `Vd`, `Vq`, `theta_e`.
- Quy trình dừng control, reset controller và khởi tạo reference khi đổi mode.
- Logger nhị phân UART8 dùng DMA, phát 15 kênh telemetry ở 100 Hz.
- Host tests cho encoder, FOC và calibration.

### Đã có khung nhưng chưa tích hợp hoàn chỉnh

- LADRC đã được khởi tạo/reset theo state nhưng lời gọi điều khiển trong speed mode đang comment; speed loop vẫn dùng PI.
- PID tổng quát đã có implementation nhưng chưa có call site trong application/control flow.
- `sys_error_t` đã khai báo các nhóm lỗi dự kiến nhưng chưa được tạo instance hoặc nối vào fault flow.
- `drv_fault()` đã đọc được chân `nFAULT`, nhưng chưa có fault-handling flow.
- `CALIB_ENCODER_MODE` và `MIT_CTRL_MODE` đã được khai báo nhưng chưa có runtime implementation.
- `SYS_FAULT` đã được khai báo nhưng chưa có logic xử lý và recovery.

### Chưa có

- Bộ bảo vệ hoàn chỉnh cho over-current, under/over-voltage, nhiệt độ, encoder/communication timeout.
- Giao tiếp slave với master bằng SPI + DMA.
- Giao tiếp master với PC bằng RS485.
- Lưu encoder offset vào non-volatile memory.

### Firmware đang chạy ở chế độ nào?

Firmware khởi tạo tại `SYS_IDLE` với `start_req = 1`, nên tự chuyển qua startup. Sau current-offset, frame encoder đầu tiên, auto-calibration và homing, hệ thống dừng tại `SYS_READY` vì `run_req` mặc định bằng `0`. Calibration thất bại sẽ disable driver và chuyển sang `SYS_FAULT`. Khi có `run_req`, `ctrl_mode` được lấy từ `user_command.user_cmd_mode`; mặc định hiện tại là position mode. Current, speed, position và open-loop voltage được dispatch trong ADC ISR. `CALIB_ENCODER_MODE` và `MIT_CTRL_MODE` chưa có runtime implementation nên đưa FOC về off.

## 3. Phần cứng và phạm vi

| Thành phần | Cấu hình hiện tại |
|---|---|
| MCU | STM32H743VIH6, Cortex-M7, CPU 400 MHz |
| Gate driver | DRV8317H, chế độ `3x PWM` |
| Encoder | MT6826S, SPI mode 3 |
| Đo dòng | Hai shunt/feedback `Ia`, `Ib`; `Ic = -(Ia + Ib)` |
| Điện áp bus | ADC3 qua mạch chia áp |
| PWM | TIM1 center-aligned, 20 kHz |
| Control | Current 20 kHz, speed 1 kHz, position 100 Hz |
| Telemetry | UART8 TX DMA, 1,000,000 baud, 100 Hz |
| Motor profile | Chọn bằng `MOTOR_TYPE` trong `Config/config.h` |
| Gear ratio | `29` với `MOTOR_TYPE = 0` hiện tại |
| Pole pairs | `1` với `MOTOR_TYPE = 0` hiện tại |

PCB gốc hướng tới một master và năm slave, nhưng code trong repository này chỉ điều khiển một motor.

Các profile hiện có:

| `MOTOR_TYPE` | Cơ cấu | Gear | Pole pairs | `CURRENT_MAX` | `CURRENT_GAIN` | `SPEED_LIMIT` | Hành trình cấu hình |
|---:|---|---:|---:|---:|---:|---:|---|
| `0` | Motor chưa lắp cơ khí | 29 | 1 | 5.0 A | 0.5 V/A | 18000 RPM | Gần như không giới hạn |
| `1` | Ngón cái | 560 | 2 | 2.5 A | 0.5 V/A | 40 RPM | -115° đến -5° |
| `2` | Đầu ngón cái | 560 | 2 | 2.5 A | 0.5 V/A | 40 RPM | 5° đến 55° |
| `3` | Các ngón còn lại | 580 | 1 | 5.0 A | 0.5 V/A | 30 RPM | 5° đến 115° |

Homing bơm dòng dương cho type `1`, dòng âm cho các type còn lại. `MOTOR_TYPE` là macro compile-time; đổi profile cần build và kiểm tra lại đúng motor/board trước khi chạy.

## 4. Kiến trúc phần mềm

```text
Application coordination (hiện chủ yếu nằm trong main.c)
        |
        +-- system state / control mode / user command
        |
        +-- FOC + cascaded controllers
        |      +-- position PI
        |      +-- speed PI (đang chạy)
        |      +-- current Id/Iq PI
        |      +-- LADRC speed controller (thử nghiệm, chưa active)
        |      +-- generic PID (đã có API, chưa tích hợp)
        |      +-- transforms + SVPWM
        |
        +-- sensors and calibration
        |      +-- current sensor
        |      +-- MT6826S encoder
        |      +-- current/encoder offset calibration
        |
        +-- diagnostic telemetry
        |      +-- TIM7 sample trigger @ 100 Hz
        |      +-- UART8 binary frame + DMA TX
        |
        +-- platform ports
               +-- ADC
               +-- PWM
               +-- GPIO/driver enable-fault
                        |
                    STM32 HAL
```

Nguyên tắc kiến trúc:

- Vòng điều khiển thời gian thực tiếp tục chạy trong luồng ngắt hiện có.
- State/mode, bảo vệ và truyền thông được xây quanh control loop, không đưa tác vụ chậm vào ISR.
- Tách rõ user command, output của controller tầng ngoài và sensor feedback.
- Không tạo thêm tầng abstraction hoặc module nếu chưa có lợi ích cụ thể.
- Không có folder `app/` ở hiện trạng; logic phối hợp vẫn nằm trong `Core/Src/main.c`.

## 5. Cấu trúc repository

```text
AGENTS.md                    Nguyên tắc và hướng phát triển dành cho coding agent
README.md                    Tài liệu tổng quan và trạng thái project
Hand_motor_control.ioc       Cấu hình STM32CubeMX
.project, .cproject          Cấu hình STM32CubeIDE

Core/
  Inc/, Src/                 Code CubeMX/HAL, peripheral init, IRQ và main flow
  Startup/                   Startup assembly cho STM32H743

Config/
  config.h                   Motor, sampling, controller gain và conversion constants
  MotorApp.*                 User command, system state, startup stage, control mode và flags

Platform/
  ADC/                       Start ADC injected và đọc trực tiếp JDR1
  PWM/                       Start TIM1/TIM2 và cập nhật CCR ba pha
  GPIO/                      Enable/disable driver và đọc nFAULT

lib/
  FOC/                       Control loops, transforms và SVPWM
  Controller/                PI đang dùng; generic PID và LADRC thử nghiệm
  Sensors/Current Sensor/    Current offset và ADC-to-current conversion
  Sensors/Encorder/          Driver encoder MT6826S
  Calib/                     Calibration offset encoder
  Log Data/                  UART binary telemetry logger
  drv/                       Placeholder, chưa có implementation DRV8317 riêng
  DSP/                       Phần CMSIS-DSP được project sử dụng/build

Drivers/                     STM32CubeH7 HAL và CMSIS
Middlewares/                 CMSIS-DSP/middleware do CubeMX quản lý
tests/                       Host tests dùng GCC và HAL giả lập
```

Tên folder `Encorder` đang giữ nguyên theo project hiện tại dù chính tả chuẩn là `Encoder`; không nên đổi chỉ để làm đẹp vì sẽ ảnh hưởng include path và project settings.

## 6. Luồng khởi động hiện tại

```text
Reset
  -> MPU + HAL + clock 400 MHz
  -> GPIO, DMA, ADC1/2/3, SPI3, TIM1/2/7 và UART8 init
  -> SYS_IDLE nhận start_req
  -> SYS_START_UP / POWER_UP
       -> encoder_init(), foc_init(), current_sensor_init()
       -> enable driver
       -> start ADC/PWM, đặt duty A = B = C = 50%
  -> CURRENT_OFFSET
       -> ADC ISR tích lũy 4096 mẫu offset dòng
  -> WAIT_ENCODER
       -> chờ frame encoder hợp lệ đầu tiên
  -> AUTO_CALIB
       -> dừng FOC và chờ giao dịch encoder DMA kết thúc
       -> chạy encoder calibration bằng SPI polling, 3 chu kỳ
       -> lỗi: PWM off, disable driver, chuyển SYS_FAULT
  -> HOMING
       -> chạy current control với Iq = CURRENT_HOME
       -> xác nhận hard stop bằng speed deadband và thời gian giữ
       -> chốt mechanical_zero_angle
  -> START_UP_DONE
  -> SYS_READY
```

`while (1)` quản lý system state và startup stage. Các vòng điều khiển thời gian thực vẫn chạy trong ADC injected-complete interrupt đồng bộ với TIM1.

## 7. Luồng thời gian thực

```text
TIM1 center-aligned carrier
  -> TIM1 CH1 OC1REF tạo TRGO gần đỉnh carrier
  -> ADC1/ADC2/ADC3 injected conversion
  -> ADC1 injected-complete ISR
       -> nếu chưa xong current offset: tích lũy mẫu offset rồi return
       -> đọc Vbus
       -> đọc Ia, Ib; tính Ic
       -> nếu đang AUTO_CALIB: return để polling calibration sở hữu PWM/SPI
       -> cập nhật tốc độ từ tổng delta góc
       -> yêu cầu frame encoder SPI DMA mới
       -> SYS_RUNNING: dispatch position/speed/current/open-loop voltage mode
       -> SYS_START_UP + HOMING: chạy current control với dòng homing
       -> các state/stage khác: foc_off()
       -> inverse Park + SVPWM và cập nhật TIM1 CCR khi control đang chạy

SPI3 DMA complete ISR (priority cao hơn ADC)
  -> nhả encoder CS
  -> validate CRC/status
  -> cập nhật góc, multi-turn và delta góc
  -> xóa cờ dma_busy

TIM7 period ISR @ 100 Hz
  -> snapshot feedback/reference theo mode hiện hành
  -> đóng gói frame nhị phân
  -> phát UART8 bằng DMA; bỏ qua mẫu nếu DMA trước vẫn bận
```

Encoder được lấy bất đồng bộ. Control dùng mẫu góc hợp lệ gần nhất và `foc_current_ctrl_run()` chốt `theta_e` vào biến local trước khi tính sin/cos, nên Park và inverse Park trong cùng một chu kỳ dùng cùng một góc ngay cả khi SPI hoàn tất giữa ISR ADC.

## 8. Chuỗi điều khiển cascade

### Vòng vị trí

```text
position reference [rad]
  - mechanical_angle [rad]
  -> PI position @ 100 Hz
  -> omega_ref_rpm, clamp +/- SPEED_LIMIT
```

### Vòng tốc độ

```text
speed reference [RPM after gear]
  - speed_filter_after_gear [RPM]
  -> PI speed @ 1 kHz
  -> iq reference [A]
  -> vector current clamp
```

`Id` được đặt bằng `0` trong speed mode hiện tại.

`LADRC_t` hiện dùng ESO tuyến tính bậc hai và đã được init/reset cùng state machine, nhưng block gọi `ladrc_update()` trong speed mode đang comment. Vì vậy đường điều khiển runtime chính thức vẫn là PI; LADRC mới là nền thử nghiệm cần kiểm chứng và tune trên phần cứng trước khi kích hoạt.

### Vòng dòng và điều chế

```text
Ia, Ib, Ic + theta_e
  -> Clarke-Park
  -> Id/Iq feedback
  -> PI Id, PI Iq @ 20 kHz
  -> vector Vdq clamp: |Vdq| <= Vbus/sqrt(3)
  -> anti-windup
  -> inverse Park
  -> SVPWM
  -> duty A/B/C
  -> TIM1 CCR4/CCR3/CCR2
```

Nếu `v_bus <= 0`, `foc_current_ctrl_run()` gọi `foc_off()` để đưa PWM về 0 và reset các PI/controller state.

## 9. Timing, PWM và ADC

### TIM1

- Timer clock: 200 MHz.
- Center-aligned mode 1.
- Prescaler: `0`.
- ARR: `4999`.
- PWM frequency: `200 MHz / (2 * 5000) = 20 kHz`.
- CH2/CH3/CH4 phát PWM ba pha.
- CH1 không xuất pin, pulse `4995`, dùng `OC1REF` làm TRGO.
- ADC injected conversion kích ở falling edge của TIM1 TRGO.

### TIM2

- Timer clock: 200 MHz.
- Prescaler: `199`, counter tick 1 MHz (`1 tick = 1 us`).
- Dùng đo execution time; `foc_elapsed_time` đang được cập nhật trong ADC ISR.

### TIM7 và UART telemetry

- TIM7 clock: 200 MHz, prescaler `999`, ARR `1999`.
- Tần số callback telemetry: `200 MHz / (1000 * 2000) = 100 Hz`.
- TIM7 IRQ priority `5`; UART8 và DMA1 Stream3 IRQ priority `7`, thấp hơn luồng control.
- UART8 chạy TX-only ở `1,000,000 baud`, `8N1`, không flow control.

### Nhịp các vòng

| Loop | Tần số | Cách chia |
|---|---:|---|
| Current/FOC | 20 kHz | Mỗi ADC ISR, `FOC_TS = 50 us` |
| Speed | 1 kHz | `SPEED_DIV = 20` |
| Position | 100 Hz | `POS_DIV = 200` |

## 10. Mapping phần cứng quan trọng

### PWM ba pha

| Pha | MCU pin | TIM1 | Mapping trong `pwm_set()` |
|---|---|---|---|
| A | PA11 | CH4 | `duty_a` |
| B | PA10 | CH3 | `duty_b` |
| C | PA9 | CH2 | `duty_c` |

Mapping này bám theo PCB hiện tại; không tự đổi thứ tự pha nếu chưa xác minh phần cứng.

### ADC, driver và encoder

| Chức năng | Pin/peripheral | Ghi chú |
|---|---|---|
| Dòng pha A | PC0, ADC1 INP10 | Injected rank 1, tạo ADC interrupt |
| Dòng pha B | PA3, ADC2 INP15 | Injected rank 1 |
| DC bus | PC1, ADC3 INP11 | Injected rank 1 |
| NTC | PA0, ADC1 INP16 | Có pin/config nhưng chưa dùng trong runtime |
| Driver `nSleep` | PB13 | Active high |
| Driver `nFault` | PB11 | Active low |
| Encoder SCK/MISO/MOSI | PB3/PB4/PB5 | SPI3 |
| Encoder CS | PC11 | Software-controlled, active low |
| Telemetry UART8 TX | PE1 | DMA1 Stream3, 1,000,000 baud |
| UART8 RX pin | PE0 | Được CubeMX cấu hình pin nhưng UART hiện chạy TX-only |

SPI3 là master full-duplex, 8 bit, MSB first, mode 3, nguồn clock 160 MHz, prescaler 16, tốc độ khoảng 10 Mbit/s. DMA1 Stream0/Stream1 và SPI3 IRQ có priority `0`; ADC IRQ có priority `1`.

## 11. Encoder MT6826S

- Burst-read command `0xA0`, starting address `0x03`, frame 6 byte.
- Góc 15 bit lấy từ RX byte 2 và 3.
- `angle_rad = raw_angle * 2*pi / 32768`.
- Status lấy từ 3 bit thấp của RX byte 4.
- CRC-8 polynomial `0x07` tính trên RX byte 2..4, so sánh với RX byte 5.
- Frame lỗi không cập nhật góc; `cnt_fault` tăng tới `CNT_ENCODER_ERROR_MAX`.
- `theta_m`: góc cơ rotor trong `[0, 2*pi)`, rad.
- `theta_e`: góc điện đã trừ `offset_encoder`, nhân `POLE_PAIRS`, wrap `[0, 2*pi)`.
- `multi_angle_m`: góc rotor đa vòng, rad.
- `mechanical_zero_angle`: mốc rotor được chốt khi homing cơ khí hoàn tất.
- `mechanical_angle`: vị trí cơ khí tương đối sau gear, rad.
- `speed_after_gear` và `speed_filter_after_gear`: RPM sau gear.

Tốc độ được tính mỗi `SPEED_DIV` chu kỳ từ tổng delta góc, sau đó qua LPF với `SPEED_FILTER_ALPHA`.

## 12. Telemetry UART

Module `lib/Log Data/uart_logger.*` phục vụ quan sát và tuning, độc lập với protocol điều khiển master trong tương lai.

```text
SOF 0xA5 0x5A
  + 15 x float32 little-endian payload
  + CRC16-CCITT-FALSE
= 64 byte/frame
```

- CRC tính trên SOF và payload; hai byte CRC truyền theo thứ tự low byte trước.
- Logger copy dữ liệu vào buffer DMA aligned 32 byte và clean D-cache khi cache đang bật.
- Nếu UART DMA trước chưa hoàn tất, `UART_Log_Send()` trả `HAL_BUSY` và mẫu mới không được gửi.
- Kênh không có reference phù hợp với mode hiện hành được gán `NaN`.

Thứ tự 15 kênh cố định:

| Index | Channel | Đơn vị/ý nghĩa |
|---:|---|---|
| 0 | `LOG_IQ` | `Iq` feedback, A |
| 1 | `LOG_IQ_REF` | `Iq` reference đang áp dụng, A |
| 2 | `LOG_ID` | `Id` feedback, A |
| 3 | `LOG_ID_REF` | `Id` reference đang áp dụng, A |
| 4 | `LOG_VD` | `Vd` sau giới hạn, V |
| 5 | `LOG_VQ` | `Vq` sau giới hạn, V |
| 6 | `LOG_V_BUS` | Điện áp DC bus, V |
| 7 | `LOG_SPEED` | Tốc độ sau gear đã lọc, RPM |
| 8 | `LOG_SPEED_REF` | Speed reference đang áp dụng, RPM |
| 9 | `LOG_POSITION` | Góc cơ khí sau gear, degree |
| 10 | `LOG_POSITION_REF` | Position command, degree |
| 11 | `LOG_TEMPERATURE` | Dự phòng nhiệt độ; hiện là `NaN` |
| 12 | `LOG_SYS_STAGE` | Giá trị `sys_state` hiện tại |
| 13 | `LOG_BOOT_STAGE` | Dự phòng startup stage; hiện luôn bằng `0` |
| 14 | `LOG_CONTROL_MODE` | Dự phòng control mode; hiện là `NaN` |

Ba kênh dự phòng ở trên đã có vị trí ổn định trên wire format nhưng chưa phản ánh đầy đủ runtime. Decoder phía PC phải dùng đúng frame 64 byte và thứ tự index 0..14; decoder 44 byte cũ không còn tương thích.

## 13. Calibration

### Offset dòng

- Driver được enable để current-sense amplifier hoạt động.
- Ba duty đặt 50%, điện áp dây lý tưởng bằng 0.
- ADC tích lũy `4096` mẫu của ADC1/ADC2.
- `offset_a`, `offset_b` là giá trị trung bình; calibration này hiện chạy một lần sau reset.

### Offset encoder

API: `run_calib_encoder(foc, encoder, CALIB_CYCLE)` trong `lib/Calib`; startup hiện gọi API này tự động ở stage `AUTO_CALIB`.

```text
B (+120 electrical degree) -> A (0 degree) -> đọc angle1
C (-120 electrical degree) -> A (0 degree) -> đọc angle2
```

- Gọi ngoài ISR và yêu cầu `cycle_count >= 2`; cấu hình hiện tại dùng `CALIB_CYCLE = 3`.
- Điện áp align gốc hiện là `VD_CALIB = 4 V`.
- Điện áp align giảm theo `VD_CALIB`, `VD_CALIB/2`, `VD_CALIB/4`.
- Bỏ chu kỳ đầu để ổn định; các chu kỳ còn lại được unwrap và tính trung bình online.
- Dùng `encoder_Read_Polling()`, không dùng DMA trong calibration.
- Chỉ ghi offset mới khi toàn bộ quá trình thành công.
- Khi kết thúc, PWM được đưa về 0 cả khi thành công lẫn lỗi.
- Offset mới chỉ nằm trong RAM và chưa được lưu non-volatile.

API không tự chuyển mode, dừng control hay chờ giao dịch DMA. `main.c` đang đảm nhiệm phần phối hợp: tắt FOC, chờ tối đa 10 ms để DMA encoder kết thúc, dành PWM/SPI cho polling calibration, rồi reset bộ tích lũy tốc độ trước khi homing. Nếu DMA vẫn bận hoặc calibration lỗi, firmware tắt PWM, disable driver và chuyển `SYS_FAULT`.

## 14. Quy ước dữ liệu

### User command

| Field | Đơn vị | Ý nghĩa |
|---|---|---|
| `id_ref` | A | Dòng d đặt |
| `iq_ref` | A | Dòng q đặt |
| `rpm_ref` | RPM | Tốc độ đặt tại đầu ra hộp số |
| `pos_ref_deg` | degree | Vị trí đặt tại đầu ra hộp số |
| `vd_ref`, `vq_ref` | V | Vector điện áp đặt cho open-loop mode |
| `theta_e_ref` | rad | Góc điện đặt cho open-loop mode |

Math bên trong control dùng radian. Degree chỉ nên xuất hiện ở biên giao diện/debug và được đổi bằng `DEG_TO_RAD()` trước khi vào position loop.

Trong `foc_t`:

- `id`, `iq`, `vd`, `vq` là reference/output nội bộ của cascade controller.
- Các field hậu tố `_fb` là feedback đo/tính toán.
- `omega_ref_rpm` là output của position PI, không phải feedback encoder.

### Chuyển đổi ADC

```text
ADC_TO_CURRENT = ADC_VREF / (ADC_MAX * GAIN)
Iphase = (ADC_raw - ADC_offset) * ADC_TO_CURRENT
Vbus = ADC3_raw * ADC_VREF * VBUS_DIV_GAIN / ADC_MAX
```

Giá trị hiện tại với `MOTOR_TYPE = 0` trong `Config/config.h`:

- `ADC_MAX = 65535` (16 bit).
- `ADC_VREF = 3.3 V`.
- `CURRENT_GAIN = 0.5 V/A`.
- `VBUS_DIV_GAIN = 5.7`.
- `CURRENT_RATED = 2.0 A`.
- `CURRENT_MAX = 5.0 A`; `foc_init()` dùng giá trị này làm giới hạn vector dòng runtime.
- `CURRENT_LIMIT_FACTOR = 2.5` hiện chưa tham gia tính `CURRENT_MAX`.

Mỗi nhánh `MOTOR_TYPE` cung cấp cùng bộ tên tham số mà source sử dụng: `GEAR_RATIO`, `POLE_PAIRS`, `CURRENT_MAX`, `ENCODER_OFFSET`, `CURRENT_GAIN`, `VOLTAGE_LIMIT`, các tham số homing và `SPEED_LIMIT`. Giới hạn góc của các cơ cấu dùng `DEG_TO_RAD(...)` ngay trong cấu hình.

Các PI gain hiện là giá trị thử nghiệm và phải tune trên đúng motor, nguồn, hộp số và tải.

## 15. State, mode và hướng phát triển tiếp theo

State và mode hiện có trong source:

```text
System state:
SYS_IDLE -> SYS_START_UP -> SYS_READY -> SYS_RUNNING
                                      -> SYS_FAULT (chưa triển khai xử lý)

Startup stage:
POWER_UP -> CURRENT_OFFSET -> WAIT_ENCODER -> AUTO_CALIB -> HOMING -> START_UP_DONE

Control mode:
CALIB_ENCODER_MODE
CURRENT_CTRL_MODE
SPEED_CTRL_MODE
POS_CTRL_MODE
MIT_CTRL_MODE
OPEN_LOOP_CTRL_VOLTAGE
```

Phần đã triển khai:

1. **Startup và system state cơ bản**
   - Tự đi từ `SYS_IDLE` qua startup tới `SYS_READY` khi có `start_req`.
   - Current offset, chờ encoder, auto-calibration và homing là các startup stage riêng.
   - Auto-calibration lỗi chuyển sang `SYS_FAULT`, nhưng fault handling/recovery tổng quát chưa có.
   - `run_req` chuyển từ ready sang running; `stop_req` đưa running về ready.

2. **Control-mode transition**
   - Current, speed, position và open-loop voltage mode được dispatch trong ADC ISR.
   - Khi đổi mode, main đưa hệ thống qua `SYS_READY`; ISR dừng output và controller được reset trước khi chạy mode mới.
   - Position reference bắt đầu tại vị trí hiện tại; speed/current reference được đưa về zero.

3. **Telemetry phục vụ tuning**
   - TIM7 lấy mẫu 100 Hz và UART8 phát frame 15 kênh/64 byte bằng DMA ở 1 Mbaud.
   - Feedback và reference được chọn theo mode; đây chưa phải command/feedback protocol với master.

4. **Controller thử nghiệm**
   - Generic PID có derivative filter, saturation và anti-windup nhưng chưa được gọi.
   - LADRC speed controller có ESO bậc hai và được init/reset, nhưng chưa active trong speed loop.

Phần phát triển tiếp theo:

1. **Protection và `SYS_FAULT`**
   - Tích hợp `nFAULT` trước tiên.
   - Bổ sung ngưỡng Vbus, dòng, encoder error/timeout và NTC theo phần cứng thực tế.
   - Xác định fault latch, fault code và điều kiện clear/restart.

2. **Các mode còn lại**
   - Quyết định có dùng LADRC hay tiếp tục PI, sau đó tune và kiểm thử trên đúng motor nếu kích hoạt LADRC.
   - Hoàn thiện runtime command/transition cho `CALIB_ENCODER_MODE` nếu cần calibration theo yêu cầu ngoài startup.
   - Phát triển MIT control khi có yêu cầu cụ thể.

3. **Communication**
   - Slave protocol với master qua SPI + DMA.
   - Command/feedback frame, CRC, sequence/timeout và fail-safe.
   - Sau đó tích hợp master với PC qua RS485.

Thứ tự trên không có nghĩa trì hoãn các điều kiện dừng và bảo vệ tối thiểu cần cho từng bước.

## 16. Build, test và debug

### STM32CubeIDE

- STM32CubeMX file version hiện ghi `MxCube.Version = 6.12.0`.
- STM32Cube FW H7 `V1.11.2`.
- Target: STM32H743VIHx, Cortex-M7 hard-float `fpv5-d16`.
- Debug C build dùng `-O2`; Release C build dùng `-O2`.

Build firmware:

1. Import project vào STM32CubeIDE.
2. Chọn configuration `Debug` hoặc `Release`.
3. Refresh project nếu vừa thêm source ngoài IDE.
4. Clean Project rồi Build Project.
5. Nếu sửa `.ioc`, generate code với `Keep User Code` và review lại mapping/timing.

### Host tests

Cần GCC trong `PATH`, sau đó chạy từ PowerShell:

```powershell
./tests/encoder/run.ps1
./tests/foc/run.ps1
./tests/calib/run.ps1
```

Ba test suite đã pass khi README này được cập nhật ngày 2026-10-27. Nếu PowerShell chặn script chưa ký, có thể chạy bằng `pwsh -ExecutionPolicy Bypass -File <script>`. Các test dùng HAL giả lập để kiểm tra logic, không thay thế build STM32CubeIDE, đo timing trên board hoặc HIL test.

## 17. Hướng dẫn cho developer và coding agent

Trước khi sửa code:

1. Đọc `AGENTS.md` và README này.
2. Đọc `Config/config.h`, `Core/Src/main.c` và module trực tiếp liên quan.
3. Xác nhận source hiện tại trước khi dựa vào sơ đồ hoặc tài liệu cũ.

Khi phát triển:

- Ưu tiên giải pháp đơn giản, nhanh, clean và dễ mở rộng; tránh overengineering.
- Giữ trách nhiệm rõ giữa system state, control mode, protection và communication.
- Giữ real-time control trong ADC ISR; không đưa delay, polling hoặc protocol parsing nặng vào ISR.
- Giữ `foc.c` tập trung vào control flow; toán transform/SVPWM nằm trong `foc_math.c`.
- Đặt tham số motor/controller/conversion tập trung trong `Config/config.h` khi phù hợp.
- Không tự đổi phase mapping, encoder direction/CS hoặc peripheral timing nếu chưa xác minh board.
- Khi sửa pin/peripheral, đồng bộ `.ioc` và generated code.
- Không để output của controller tầng ngoài ghi đè hoặc bị nhầm với user command.
- Sau thay đổi, chạy host tests liên quan và build firmware nếu toolchain có sẵn.

## 18. Known limitations và rủi ro kỹ thuật

- Auto-calibration đã có startup flow nhưng vẫn là tác vụ blocking ngoài ISR; chưa có timeout tổng cho toàn bộ quá trình align/calibration.
- `CALIB_ENCODER_MODE` và `MIT_CTRL_MODE` hiện đưa FOC về off vì chưa có implementation.
- `SYS_FAULT` chưa có fault entry, latch, clear hoặc recovery flow.
- `sys_error_t` mới chỉ là khai báo dữ liệu; chưa có instance và chưa nối với `SYS_FAULT`.
- `nFAULT` và NTC chưa tham gia quyết định dừng motor.
- Chưa có encoder timeout; frame lỗi chỉ tăng counter và giữ góc cũ.
- Startup/homing chưa có timeout tổng hoặc điều kiện abort sang fault.
- Position mode clamp command bằng `ANGLE_MIN/MAX`; speed/current/open-loop mode chưa có bảo vệ hành trình cơ khí hoàn chỉnh.
- Kiểm tra `pos_ref_deg` trong speed-mode code hiện trộn degree với giới hạn radian và không dùng feedback vị trí để dừng hành trình.
- `VOLTAGE_LIMIT` đã có theo motor profile nhưng chưa tham gia runtime clamp; giới hạn thực tế vẫn là `Vbus/sqrt(3)`.
- `CURRENT_LIMIT_FACTOR` chưa được dùng để tính giới hạn runtime; giới hạn vector dòng lấy trực tiếp từ `CURRENT_MAX`.
- `MOTOR_TYPE = 0` vẫn chạy homing với dòng âm dù profile được mô tả là motor không lắp cơ khí; cần xác minh trước khi cấp công suất.
- UART telemetry là một chiều, không có command parser, sequence number hoặc cơ chế backpressure ngoài việc bỏ mẫu khi DMA bận.
- `LOG_TEMPERATURE` và `LOG_CONTROL_MODE` chưa được cập nhật nên vẫn là `NaN`; `LOG_BOOT_STAGE` hiện hard-code bằng `0` thay vì `start_up_stage`.
- Generic PID chưa có call site; LADRC chưa active vì `ladrc_update()` vẫn đang comment trong speed mode.
- `Hand_motor_control.ioc` vẫn giữ UART8 `921600 baud` và TIM7 `PSC=1999`, `ARR=999`, trong khi generated source đang dùng 1 Mbaud và `PSC=999`, `ARR=1999`. Hai cấu hình TIM7 đều cho 100 Hz, nhưng cần đồng bộ `.ioc` trước lần generate code kế tiếp để tránh ghi đè source.
- Profile `MOTOR_TYPE = 3` hiện không có `ENCODER_OFFSET` active; đổi sang profile này sẽ cần bổ sung/xác nhận offset trước khi build và chạy.
- Encoder offset chưa lưu non-volatile.
- Gain controller cần tune và xác nhận lại trên hệ thật.
- Timing/jitter ADC–SPI DMA–FOC chưa được characterise đầy đủ trên board.
- Chưa có automated hardware-in-the-loop test.
- Schematic và datasheet motor/encoder đang được quản lý ngoài repository.

## 19. Safety

Đây là firmware điều khiển công suất. Trước khi cấp nguồn động lực:

- Dùng nguồn lab có giới hạn dòng và chuẩn bị phương án ngắt nguồn/disable driver ngay lập tức.
- Xác minh Vbus scaling, current offset, phase order, encoder direction và electrical offset.
- Bắt đầu với điện áp/dòng đặt nhỏ.
- Không giữ rotor align/calibration quá lâu vì motor có thể nóng nhanh.
- Theo dõi `nFAULT`, nhiệt độ driver/motor và dòng bus.
- Không coi host test là bằng chứng đủ để motor chạy an toàn.
