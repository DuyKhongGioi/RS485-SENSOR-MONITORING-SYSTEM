# Hệ thống Thu thập và Giám sát Dữ liệu Cảm biến Nhiệt độ - Độ ẩm qua RS485 & LabVIEW

Dự án cuối kỳ môn **Cơ sở Đo lường và Điều khiển số** (ELT3270). Hệ thống tập trung vào việc thu thập dữ liệu môi trường, truyền tin qua chuẩn công nghiệp RS485 và giám sát từ xa đa nền tảng.

## Giới thiệu chung

Trong các môi trường công nghiệp như kho lạnh, nhà kính hay phòng máy chủ, việc giám sát nhiệt độ và độ ẩm là cực kỳ quan trọng. Dự án này xây dựng một giải pháp:

* Thu thập dữ liệu từ cảm biến với độ chính xác cao.


* Truyền dữ liệu ổn định ở khoảng cách xa qua chuẩn RS485.


* Giám sát trực quan thời gian thực trên máy tính (LabVIEW) và lưu trữ đám mây để theo dõi từ xa (Firebase/App Android).



## Thành phần hệ thống

### 1. Phần cứng

* 
**Vi điều khiển chính:** STM32 (Xử lý trung tâm tại trạm đo).


* 
**Vi điều khiển truyền tin:** ESP32 (Hỗ trợ kết nối WiFi/Cloud).


* 
**Cảm biến:** DHT11 và DHT22 (Đo nhiệt độ và độ ẩm).


* 
**Giao tiếp:** Module chuyển đổi UART sang RS485 (Truyền xa, chống nhiễu).



### 2. Phần mềm & Giao thức

* 
**LabVIEW:** Giao diện giám sát trên PC (GUI), hiển thị dữ liệu đo, biểu đồ và cảnh báo theo thời gian thực.


* 
**Firebase:** Cơ sở dữ liệu đám mây để lưu trữ và đồng bộ dữ liệu.


* 
**Chuẩn truyền thông:** RS485 và UART.

## Thành viên thực hiện (Nhóm 7)

* 
**Phạm Văn Duy** (22022118) 


* 
**Ngô Đức Hiếu** (22022103) 


* 
**Nguyễn Quốc Toản** (22022175) 


* 
**Tạ Đình Kiên** (22022145) 



**Giảng viên hướng dẫn:** TS. Phạm Duy Hưng **Mentor:** Kiều Tất Thắng 
