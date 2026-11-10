# TH04 — Điều khiển nhà kính qua MQTT

ESP32 nhận nhiệt độ/độ ẩm từ UNO qua I2C và trao đổi dữ liệu với Mosquitto local. Giao diện HTML điều khiển LED1, LED2, bơm MOTOR1, hiển thị và tải JSON.

## Video minh chứng
[Xem hoặc tải video thực nghiệm](baith4.mp4)

## Cấu trúc
- firmware/: dự án PlatformIO cho ESP32.
- web/: HTML/CSS/JavaScript, thư viện MQTT.js đi kèm.
- broker/: cấu hình Mosquitto chạy trực tiếp Windows; Docker Compose được giữ như phương án riêng.

## Chạy local không Docker
1. Cài Mosquitto và Python, thêm Mosquitto vào PATH.
2. Nếu chưa có broker trên cổng 1883, mở terminal tại thư mục repo:
   mosquitto -c broker/mosquitto-native.conf -v
   Nếu dịch vụ Mosquitto đang chạy cổng 1883, dùng dịch vụ đó, không chạy thêm broker cùng cổng.
3. Mở terminal khác:
   mosquitto -c broker/mosquitto-websocket.conf -v
   WebSocket chỉ lắng nghe tại máy local cổng 9001, nối với broker 1883. Nếu cổng đã được sử dụng, kiểm tra tiến trình hiện có.
4. Sao chép firmware/src/secrets.example.h thành firmware/src/secrets.h. Điền WiFi và IP LAN máy chạy broker. Không dùng localhost làm IP broker trên ESP32. Để trống MQTT_USER/MQTT_PASSWORD với cấu hình local không xác thực.
5. Build/nạp:
   pio run -d firmware
   pio run -d firmware -t upload --upload-port COM7
   Chọn lại cổng nếu máy dùng COM khác.
6. Mở web/index.html trực tiếp hoặc chạy:
   python -m http.server 8084 --bind 127.0.0.1 --directory web
7. Mở http://localhost:8084, URL ws://localhost:9001, topic nhom01/node01, để trống tài khoản/mật khẩu và nhấn Kết nối.

ESP32 và máy chạy broker dùng cùng mạng. Cấu hình không xác thực dành cho mạng lab; không mở cổng broker ra Internet.
Thông tin WiFi và mật khẩu không được đưa lên Git. secrets.h và dữ liệu build đã được bỏ qua.

## Phần cứng và điều khiển
- UNO slave 0x08, ESP32 master SDA GPIO33, SCL GPIO32, đọc 4 byte mỗi 2 giây.
- Giải mã đã thống nhất: hai uint16 big-endian chia 10, nhiệt độ rồi độ ẩm. Chưa có mã UNO để xác nhận giao thức độc lập.
- LED1 GPIO13, LED2 GPIO12, HIGH bật, PWM 25/50/90%.
- MOTOR1 GPIO5 PWM, GPIO17 LOW, mức 25/50/75/100%.
- Tuyệt đối không khởi tạo hoặc điều khiển MOTOR2 GPIO23/GPIO22.
- Khởi động tải tắt; tối đa 20 giây/lần; thay công suất không gia hạn; dừng khi cảm biến lỗi/cũ hoặc mất kết nối được phát hiện.
- Task cảm biến, MQTT, WiFi và điều khiển an toàn tách riêng.
- Mưa chưa có trong gói UNO: rain=null, rain_valid=false.

## MQTT
- nhom01/node01/telemetry: temp, humi, valid, units, ts (uptime_ms), rain.
- nhom01/node01/state: session, revision, valid, outputs.
- nhom01/node01/command: target, power, session, revision; QoS0, retain=false.
- Giao diện chỉ bật khi có trạng thái mới, không xếp hàng lệnh offline, xác nhận bằng phản hồi ESP32.
- Mục JSON cho xem telemetry/state và tải bản tin mới nhất xuống file.

## Kiểm tra
Firmware tương ứng đã build và nạp COM7; nhận telemetry thật 26,2°C, 38% RH tại thời điểm kiểm tra, trạng thái các tải 0%. Đã nhận state qua WebSocket.
Kiểm thử logic UI: node web/tests/ui.test.cjs.
Video minh chứng do người dùng cung cấp; chưa tự đánh giá nội dung video.
Chưa có kết luận về thử nghiệm liên tục 10 phút hoặc ngắt broker 30 giây từ lần kiểm tra này.
Yêu cầu Docker Compose của đề bài là tiêu chí riêng; hướng dẫn trên chạy native theo lựa chọn của người dùng.
