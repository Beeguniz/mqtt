# MQTT Broker (Eclipse Mosquitto)

## 1. Tạo file mật khẩu

Trước khi khởi động broker, cần tạo file mật khẩu cho người dùng MQTT:

```bash
docker run --rm -it -v ./mosquitto/config:/mosquitto/config eclipse-mosquitto:2 mosquitto_passwd -c /mosquitto/config/password_file <tên_user>
```

Thay `<tên_user>` bằng tên người dùng mong muốn. Hệ thống sẽ yêu cầu nhập mật khẩu.

Để thêm người dùng mới (không ghi đè file cũ), bỏ flag `-c`:

```bash
docker run --rm -it -v ./mosquitto/config:/mosquitto/config eclipse-mosquitto:2 mosquitto_passwd /mosquitto/config/password_file <tên_user_mới>
```

## 2. Khởi động / Dừng broker

**Khởi động:**

```bash
docker compose up -d
```

**Dừng:**

```bash
docker compose down
```

**Xem log:**

```bash
docker logs mqtt-broker
```

## 3. Kiểm tra kết nối

**Subscribe (nhận tin nhắn):**

```bash
mosquitto_sub -h localhost -p 1883 -t "test/topic" -u <tên_user> -P <mật_khẩu>
```

**Publish (gửi tin nhắn):**

```bash
mosquitto_pub -h localhost -p 1883 -t "test/topic" -m "Hello MQTT" -u <tên_user> -P <mật_khẩu>
```
