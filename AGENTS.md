# Nguyên tắc xuyên suốt project

Các nguyên tắc dưới đây được người dùng thống nhất ngày 2026-10-04.

- Ưu tiên giải pháp đơn giản, hiệu quả về tốc độ thực thi, clean và dễ mở rộng.
- Không overengineering; không chia thêm folder, module, tầng abstraction, biến hoặc API nếu chưa có lợi ích cụ thể.
- Giữ trách nhiệm rõ ràng, nhất là giữa system state, control mode, bảo vệ và truyền thông; không bắt buộc mỗi trách nhiệm phải có một folder riêng.
- Đọc README.md và source liên quan trước khi đề xuất thay đổi. Source hiện tại là căn cứ xác định implementation; README có thể chưa cập nhật hoàn toàn.

# Hướng phát triển đã thống nhất

- Đã có các vòng điều khiển dòng, vận tốc và vị trí. Phát triển tiếp theo hướng bottom-up: chuyển control mode -> chuyển system state (init/startup, chờ, chạy, fault) -> hoàn thiện cơ chế bảo vệ -> truyền thông với master và PC.
- Đích kiến trúc tham khảo: PC <-> RS485 <-> master <-> SPI + DMA <-> slave STM32 <-> driver, motor và sensors.
- Hai sơ đồ người dùng cung cấp là tài liệu tham khảo, không phải đặc tả bắt buộc. Có thể đơn giản hóa; không tự mặc định mọi state, bước calibration/homing, peripheral hay cờ trong sơ đồ đều cần triển khai.
- Giữ các vòng điều khiển thời gian thực ở luồng ngắt hiện có; thiết kế phối hợp state/mode và truyền thông quanh nền tảng này.
- Thứ tự phát triển không có nghĩa trì hoãn các điều kiện dừng và bảo vệ tối thiểu cần cho từng bước.
- Làm từng bước theo phạm vi người dùng yêu cầu. Giai đoạn đọc và thống nhất kiến trúc không phải là ủy quyền tự triển khai firmware.
