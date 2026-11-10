# Giao diện nhà kính MQTT — TH04

## Khởi chạy local
Xem [hướng dẫn chính](../README.md#chạy-local-không-docker). Broker local không dùng tài khoản: để trống tài khoản và mật khẩu trên giao diện.

HTML/CSS/JS thuần; MQTT.js 5.10.4 đi kèm trong vendor, giấy phép MIT. Không cần npm hay CDN khi chạy.
Broker dùng MQTT TCP 1883 cho ESP32 và WebSocket 9001 cho trình duyệt. Cấu hình hiện tại dùng trong LAN; trang HTTPS cần WSS/TLS riêng.
Trang không lưu tài khoản vào localStorage. Mật khẩu chỉ nằm trong phiên trang.

## Chức năng
- LED1 GPIO13 và LED2 GPIO12, HIGH bật, PWM 25/50/90%.
- MOTOR1 GPIO5 PWM, GPIO17 LOW, công suất 25/50/75/100%.
- Tắt từng tải, dừng tất cả; mặc định tất cả tắt khi khởi động.
- Không khởi tạo hoặc ghi GPIO MOTOR2 (23, 22).
- Trạng thái mỗi giây từ ESP32; không tự giả định bật thành công.
- Mất trạng thái quá 3,5 giây: khóa nút bật và hiển thị chưa rõ.
- Firmware giám sát riêng mỗi 20 ms: tối đa 20 giây/lần chạy, thay đổi công suất không gia hạn.
- Dừng khi cảm biến lỗi/cũ quá 6 giây, MQTT mất kết nối được phát hiện, hoặc task MQTT không heartbeat quá 3,5 giây.
  MQTT keepalive 10 giây nên đứt mạng vật lý có thể mất thời gian mới được phát hiện; giới hạn chạy 20 giây vẫn độc lập.
- Đóng trang không đồng nghĩa dừng tức thì; hãy bấm Dừng tất cả và xem phản hồi trước khi đóng.
- Không tự bật lại khi mạng/cảm biến phục hồi.

## Topic và payload
Topic gốc mặc định nhom01/node01, đổi đồng bộ trong config.h và giao diện.
- /telemetry: mỗi 10 giây, device_id, ts (uptime_ms, không phải giờ UTC), temp, humi, units, valid, rain, rain_valid.
- /state: mỗi giây, session, revision, valid và outputs.{led1,led2,motor1}.{power,remaining_ms}.
- /command: JSON {"target":"motor1","power":50,"session":123,"revision":4}.
  Dùng session/revision mới nhất từ /state; target chỉ led1, led2, motor1, all.
  power=0 để tắt; all chỉ cho phép power=0. Gửi QoS0, retain=false.
  Session đổi sau mỗi lần kết nối broker; revision đổi sau lệnh/trạng thái dừng.
  Lệnh bật có session/revision cũ hoặc nằm hàng đợi quá 2 giây sẽ bị bỏ qua.
  UI không xếp hàng lệnh khi mất kết nối. Không publish retained command.
  UI xác nhận bằng trạng thái phản hồi; sau 4 giây không khớp sẽ báo chưa xác nhận.

## Nguồn cảm biến
UNO slave 0x08; ESP32 master SDA33/SCL32 đọc 4 byte mỗi 2 giây.
Giữ cách giải mã đã thống nhất: hai uint16 big-endian chia 10, nhiệt độ rồi độ ẩm.
Đây vẫn là quy ước tạm vì chưa có code UNO. Dữ liệu lỗi trả null, valid=false.
Gói 4 byte chưa có mưa: rain=null, rain_valid=false. Chưa đáp ứng đầy đủ phần cảm biến mưa của bài 4; cần mở rộng giao thức UNO trước khi bổ sung.
Không đọc GPIO34 để giả làm cảm biến mưa.

## Kiểm tra thực tế trước khi nộp
- Kiểm tra bật/tắt và PWM từng tải; chạy đủ 20 giây; thay công suất không làm lại bộ đếm.
- Ngắt cảm biến: tải dừng, không bật được; nối lại không tự chạy.
- Dừng broker 30 giây rồi bật lại: firmware tự kết nối, tải không tự bật.
- Nhận log telemetry liên tục 10 phút trên máy khác, kiểm tra khoảng cách 10 giây và ghi nhận bản tin thiếu.
- Không lưu mật khẩu trong file log/ảnh minh chứng.
Chưa có kiểm thử thiết bị thật cho thay đổi này; build không thay thế kiểm thử phần cứng.

Tài liệu giao thức: https://github.com/mqttjs/MQTT.js và https://mosquitto.org/man/mosquitto-conf-5.html

## Kết quả kiểm tra khi triển khai
- PlatformIO build thành công (RAM 14,9%, Flash 80,2%).
- node --check web/app.js đạt.
- node web/tests/ui.test.cjs đạt: khóa khi offline/stale/invalid, bỏ retained state, gửi đúng payload, chờ phản hồi, STOP, không xếp hàng offline.
- docker compose config --quiet đạt; HTTP trang trả 200; mã hóa UTF-8 đã kiểm tra.
- Docker daemon chưa chạy; chưa kiểm thử MQTT thật hoặc 10 phút telemetry.
- Công cụ trình duyệt không khởi tạo được nên chưa kiểm tra bố cục trực quan.
- Chưa nạp firmware lên ESP32.
