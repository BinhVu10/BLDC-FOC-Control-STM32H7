# BLDC/PMSM FOC Control on STM32H743

Firmware điều khiển một động cơ BLDC/PMSM bằng Field-Oriented Control (FOC) trên STM32H743VIH6. Project hiện đã có nền tảng điều khiển dòng, tốc độ và vị trí chạy theo ngắt ADC đồng bộ PWM; bước phát triển tiếp theo là hoàn thiện chuyển control mode, system state, bảo vệ và truyền thông.

> Trạng thái tài liệu: đối chiếu với source ngày 2026-10-05. Source hiện tại là căn cứ cuối cùng nếu README và code khác nhau.

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

Repository này mới tập trung vào firmware của một slave STM32 điều khiển một motor. Giao tiếp với master/PC chưa được triển khai.

## 2. Trạng thái hiện tại

### Đã có trong source

- Clarke, Park, Clarke-Park và inverse Park transform.
- SVPWM sáu sector, duty được giới hạn trong `[0, 1]`.
- PI controller và anti-windup back-calculation.
- Vòng dòng `Id/Iq` ở 20 kHz.
- Vòng tốc độ ở 1 kHz, sinh dòng `Iq` đặt.
- Vòng vị trí ở 100 Hz, sinh tốc độ đặt.
- Giới hạn vector điện áp và vector dòng.
- Đọc hai dòng pha bằng ADC injected conversion; tái tạo pha thứ ba.
- Đọc DC bus bằng ADC3.
- Đọc encoder MT6826S qua SPI3 DMA trong lúc control.
- CRC/status validation, góc cơ, góc điện, multi-turn, góc và tốc độ sau hộp số.
- Calibration offset dòng lúc khởi động.
- API calibration offset encoder bằng SPI polling.
- Host tests cho encoder, FOC và calibration.

### Đã có khung nhưng chưa tích hợp hoàn chỉnh

- `sys_state_t`, `ctrl_mode_t` và `user_command_t` đã được khai báo.
- API `run_calib_encoder()` đã hoàn thiện độc lập nhưng lệnh gọi trong startup đang comment.
- `drv_fault()` đã đọc được chân `nFAULT`, nhưng chưa có fault-handling flow.
- `foc_off()` có thể zero PWM và reset controller, nhưng chưa được điều phối bởi state machine.

### Chưa có

- State machine vận hành hoàn chỉnh.
- Dispatcher chọn current/speed/position mode trong runtime.
- Quy trình chuyển mode an toàn và chuẩn bị lại controller/encoder state.
- Bộ bảo vệ hoàn chỉnh cho over-current, under/over-voltage, nhiệt độ, encoder/communication timeout.
- Giao tiếp slave với master bằng SPI + DMA.
- Giao tiếp master với PC bằng RS485.
- Lưu encoder offset vào non-volatile memory.

### Firmware đang chạy ở chế độ nào?

`Core/Src/main.c` hiện chưa dùng `state` và `ctrl_mode` để dispatch. Sau khi lấy offset dòng và nhận được mẫu encoder hợp lệ đầu tiên, callback ADC gọi trực tiếp vòng vị trí:

```c
foc_ctrl_pos_run(&foc, &encoder, DEG_TO_RAD(user_command.pos_ref_deg));
```

Vì `user_command.pos_ref_deg` khởi tạo bằng `0`, flow hiện tại giữ vị trí tương đối `0°` tính từ mẫu encoder hợp lệ đầu tiên sau reset. Đây là flow thử nghiệm nền tảng, chưa phải flow vận hành hoàn chỉnh.

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
| Gear ratio | `29` theo `Config/config.h` |
| Pole pairs | `1` theo `Config/config.h` |

PCB gốc hướng tới một master và năm slave, nhưng code trong repository này chỉ điều khiển một motor.

## 4. Kiến trúc phần mềm

```text
Application coordination (hiện chủ yếu nằm trong main.c)
        |
        +-- system state / control mode / user command
        |
        +-- FOC + cascaded controllers
        |      +-- position PI
        |      +-- speed PI
        |      +-- current Id/Iq PI
        |      +-- transforms + SVPWM
        |
        +-- sensors and calibration
        |      +-- current sensor
        |      +-- MT6826S encoder
        |      +-- current/encoder offset calibration
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
  app_state.*                Kiểu user command, system state và control mode

Platform/
  ADC/                       Start ADC injected và đọc trực tiếp JDR1
  PWM/                       Start TIM1/TIM2 và cập nhật CCR ba pha
  GPIO/                      Enable/disable driver và đọc nFAULT

lib/
  FOC/                       Control loops, transforms và SVPWM
  Controller/                PI controller và anti-windup
  Sensors/Current Sensor/    Current offset và ADC-to-current conversion
  Sensors/Encorder/          Driver encoder MT6826S
  Calib/                     Calibration offset encoder
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
  -> GPIO, DMA, ADC1/2/3, SPI3, TIM1/2 init
  -> foc_init()
  -> current_sensor_init()
  -> encoder_init()
  -> drv_enable()
  -> chờ current-sense amplifier ổn định 500 ms
  -> adc_start()
  -> pwm_start()
  -> đặt duty A = B = C = 50%
  -> ADC ISR tích lũy 4096 mẫu offset dòng
  -> chờ current_offset_done
  -> while (1) rỗng
```

Control không chạy trong `while (1)`. Toàn bộ chu kỳ thời gian thực bắt đầu từ ADC injected-complete interrupt được đồng bộ với TIM1.

## 7. Luồng thời gian thực

```text
TIM1 center-aligned carrier
  -> TIM1 CH1 OC1REF tạo TRGO gần đỉnh carrier
  -> ADC1/ADC2/ADC3 injected conversion
  -> ADC1 injected-complete ISR
       -> đọc Vbus
       -> đọc Ia, Ib; tính Ic
       -> cập nhật tốc độ từ tổng delta góc
       -> yêu cầu frame encoder SPI DMA mới
       -> nếu đã có góc hợp lệ: chạy position -> speed -> current FOC
       -> inverse Park + SVPWM
       -> cập nhật TIM1 CCR

SPI3 DMA complete ISR (priority cao hơn ADC)
  -> nhả encoder CS
  -> validate CRC/status
  -> cập nhật góc, multi-turn và delta góc
  -> xóa cờ dma_busy
```

Encoder được lấy bất đồng bộ. Control dùng mẫu góc hợp lệ gần nhất và `foc_current_ctrl_run()` chốt `theta_e` vào biến local trước khi tính sin/cos, nên Park và inverse Park trong cùng một chu kỳ dùng cùng một góc ngay cả khi SPI hoàn tất giữa ISR ADC.

## 8. Chuỗi điều khiển cascade

### Vòng vị trí

```text
position reference [rad]
  - angle_after_gear [rad]
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
- `mechanical_zero_angle`: mẫu hợp lệ đầu tiên sau reset.
- `angle_after_gear`: vị trí tương đối sau gear, rad.
- `speed_after_gear` và `speed_filter_after_gear`: RPM sau gear.

Tốc độ được tính mỗi `SPEED_DIV` chu kỳ từ tổng delta góc, sau đó qua LPF với `SPEED_FILTER_ALPHA`.

## 12. Calibration

### Offset dòng

- Driver được enable để current-sense amplifier hoạt động.
- Ba duty đặt 50%, điện áp dây lý tưởng bằng 0.
- ADC tích lũy `4096` mẫu của ADC1/ADC2.
- `offset_a`, `offset_b` là giá trị trung bình; calibration này hiện chạy một lần sau reset.

### Offset encoder

API: `run_calib_encoder(foc, encoder, CALIB_CYCLE)` trong `lib/Calib`.

```text
B (+120 electrical degree) -> A (0 degree) -> đọc angle1
C (-120 electrical degree) -> A (0 degree) -> đọc angle2
```

- Gọi ngoài ISR và yêu cầu `cycle_count >= 2`.
- Điện áp align giảm theo `VD_CALIB`, `VD_CALIB/2`, `VD_CALIB/4`.
- Bỏ chu kỳ đầu để ổn định; các chu kỳ còn lại được unwrap và tính trung bình online.
- Dùng `encoder_Read_Polling()`, không dùng DMA trong calibration.
- Chỉ ghi offset mới khi toàn bộ quá trình thành công.
- Khi kết thúc, PWM được đưa về 0 cả khi thành công lẫn lỗi.
- Offset mới chỉ nằm trong RAM.

API không tự chuyển mode, dừng control hay chờ giao dịch DMA. Phần application/state machine phải đảm bảo không còn control/DMA đang chạy trước khi gọi, rồi reset các state cần thiết trước khi quay lại closed-loop control.

## 13. Quy ước dữ liệu

### User command

| Field | Đơn vị | Ý nghĩa |
|---|---|---|
| `id_ref` | A | Dòng d đặt |
| `iq_ref` | A | Dòng q đặt |
| `rpm_ref` | RPM | Tốc độ đặt tại đầu ra hộp số |
| `pos_ref_deg` | degree | Vị trí đặt tại đầu ra hộp số |

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

Giá trị hiện tại trong `Config/config.h`:

- `ADC_MAX = 65535` (16 bit).
- `ADC_VREF = 3.3 V`.
- `GAIN = 0.5 V/A`.
- `VBUS_DIV_GAIN = 5.7`.
- `CURRENT_RATED = 2.0 A`.
- `CURRENT_LIMIT_FACTOR = 2.5`, nhưng chưa nối vào giới hạn runtime.
- `foc_init()` đang hard-code `current_limit_max = 3.0 A`.

Các PI gain hiện là giá trị thử nghiệm và phải tune trên đúng motor, nguồn, hộp số và tải.

## 14. State, mode và hướng phát triển tiếp theo

Enum hiện có trong source:

```text
System state:
SYS_IDLE -> SYS_START_UP -> SYS_RUNNING -> SYS_FAULT

Control mode:
CTRL_NONE
CALIB_ENCODER_MODE
CURRENT_CTRL_MODE
SPEED_CTRL_MODE
POS_CTRL_MODE
```

State machine chưa được triển khai. Thứ tự phát triển đã thống nhất:

1. **Control-mode transition**
   - Dispatch current/speed/position mode trong ADC ISR.
   - Quy định ownership của command/reference ở từng mode.
   - Reset PI, divider counter, speed accumulator và output khi đổi mode.
   - Tách calibration mode khỏi closed-loop ISR một cách an toàn.

2. **System state**
   - `SYS_START_UP`: kiểm tra tối thiểu, offset dòng, lấy mẫu encoder đầu tiên và calibration khi được yêu cầu.
   - `SYS_IDLE`: không phát torque, nhận lệnh/chờ enable.
   - `SYS_RUNNING`: cho phép mode dispatcher chạy.
   - `SYS_FAULT`: zero PWM, disable driver khi cần và chỉ cho phép recovery có điều kiện.

3. **Protection**
   - Tích hợp `nFAULT` trước tiên.
   - Bổ sung ngưỡng Vbus, dòng, encoder error/timeout và NTC theo phần cứng thực tế.
   - Xác định fault latch, fault code và điều kiện clear/restart.

4. **Communication**
   - Slave protocol với master qua SPI + DMA.
   - Command/feedback frame, CRC, sequence/timeout và fail-safe.
   - Sau đó tích hợp master với PC qua RS485.

Thứ tự trên không có nghĩa trì hoãn các điều kiện dừng và bảo vệ tối thiểu cần cho từng bước.

## 15. Build, test và debug

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

Ba test suite đã pass khi README này được cập nhật ngày 2026-10-05. Chúng dùng HAL giả lập để kiểm tra logic, không thay thế build STM32CubeIDE, đo timing trên board hoặc HIL test.

## 16. Hướng dẫn cho developer và coding agent

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

## 17. Known limitations và rủi ro kỹ thuật

- Main flow hiện luôn gọi position control sau khi encoder initialize; state/mode variables chưa có tác dụng.
- Khởi tạo `user_command` trong `main.c` ghi `.id_ref` hai lần; `iq_ref` chưa được khởi tạo tường minh.
- Encoder calibration chưa được nối vào startup/mode flow.
- Chưa có cơ chế dừng yêu cầu DMA mới và chờ DMA hoàn tất khi chuyển sang polling calibration.
- `nFAULT` và NTC chưa tham gia quyết định dừng motor.
- Chưa có encoder timeout; frame lỗi chỉ tăng counter và giữ góc cũ.
- `CURRENT_LIMIT_FACTOR` chưa được dùng; current vector limit đang hard-code 3 A.
- Encoder offset chưa lưu non-volatile.
- Gain controller cần tune và xác nhận lại trên hệ thật.
- Timing/jitter ADC–SPI DMA–FOC chưa được characterise đầy đủ trên board.
- Chưa có automated hardware-in-the-loop test.
- Schematic và datasheet motor/encoder đang được quản lý ngoài repository.

## 18. Safety

Đây là firmware điều khiển công suất. Trước khi cấp nguồn động lực:

- Dùng nguồn lab có giới hạn dòng và chuẩn bị phương án ngắt nguồn/disable driver ngay lập tức.
- Xác minh Vbus scaling, current offset, phase order, encoder direction và electrical offset.
- Bắt đầu với điện áp/dòng đặt nhỏ.
- Không giữ rotor align/calibration quá lâu vì motor có thể nóng nhanh.
- Theo dõi `nFAULT`, nhiệt độ driver/motor và dòng bus.
- Không coi host test là bằng chứng đủ để motor chạy an toàn.
