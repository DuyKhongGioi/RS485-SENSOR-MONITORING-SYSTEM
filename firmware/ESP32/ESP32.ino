#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <time.h>

// WiFi
#define WIFI_SSID "P702"
#define WIFI_PASSWORD "12345678"

// Firebase
#define API_KEY ""
#define DATABASE_URL "temperaturehumidity-4f9f0-default-rtdb.asia-southeast1.firebasedatabase.app"
#define USER_EMAIL "hieu@gmail.com"
#define USER_PASSWORD "123456"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

String rx_buffer = "";
bool data_received = false;
unsigned long last_ready_time = 0;
const unsigned long ready_timeout = 5000;
bool ready_sent = false;

void logEvent(String event) {
    if (Firebase.ready()) {
        time_t now = time(nullptr);
        struct tm *timeinfo = localtime(&now);
        char timestamp[20];
        strftime(timestamp, sizeof(timestamp), "%d/%m/%Y %H:%M:%S", timeinfo);

        FirebaseJson json;
        json.set("event", event);
        json.set("timestamp", timestamp);

        Firebase.RTDB.pushJSON(&fbdo, "/logs", &json);
    }
}

void sendReady() {
    Serial1.print("READY\n");
    Serial.println(">> READY");
    logEvent("Sent READY");
    last_ready_time = millis();
    ready_sent = true;
}

String getSensorPathByID(String id) {
    if (id == "01") return "/sensors/sensor1/readings";
    else if (id == "02") return "/sensors/sensor2/readings";
    else if (id == "03") return "/sensors/sensor3/readings"; // Thêm đường dẫn cho cảm biến 3
    else return "/sensors/unknown/readings";
}

void setup() {
    Serial.begin(115200);
    Serial1.begin(9600, SERIAL_8N1, 16, 17); // RX = GPIO16, TX = GPIO17

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.println("\nWiFi connected");
    logEvent("WiFi Connected");

    configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    while (time(nullptr) < 100000) {
        delay(500);
    }
    logEvent("Time Synced");

    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;
    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;
    Firebase.reconnectWiFi(true);
    fbdo.setBSSLBufferSize(4096, 1024);
    Firebase.begin(&config, &auth);

    while (!Firebase.ready()) {
        delay(500);
    }
    logEvent("Firebase Connected");

    delay(1000);
    sendReady(); // Send READY on startup
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        WiFi.reconnect();
        delay(5000);
        return;
    }

    while (Serial1.available()) {
        char c = Serial1.read();
        rx_buffer += c;
        if (c == '\n') {
            data_received = true;
            break;
        }
    }

    if (data_received && Firebase.ready()) {
        rx_buffer.trim();
        Serial.print("<< Nhận: ");
        Serial.println(rx_buffer);
        logEvent("Received: " + rx_buffer);

        if (rx_buffer.startsWith("ID:")) {
            String id = rx_buffer.substring(3, 5);
            String content = rx_buffer.substring(6); // Bỏ "ID:xx,"

            // Kiểm tra ký tự trạng thái cuối
            char status = content.charAt(content.length() - 1); // 'a', 's', 'n'
            content = content.substring(0, content.length() - 1); // Loại bỏ trạng thái để parse T, H

            float temp = 0.0, hum = 0.0;
            if (sscanf(content.c_str(), "T:%f H:%f", &temp, &hum) == 2) {
                time_t now = time(nullptr);
                struct tm *timeinfo = localtime(&now);
                char timestamp[20];
                strftime(timestamp, sizeof(timestamp), "%d/%m/%Y %H:%M:%S", timeinfo);

                // Gửi dữ liệu lên /sensors
                FirebaseJson json;
                json.set("temperature", temp);
                json.set("humidity", hum);
                json.set("timestamp", timestamp);

                String path = getSensorPathByID(id);
                if (Firebase.RTDB.pushJSON(&fbdo, path, &json)) {
                    Serial.printf(">> Gửi Firebase thành công: %s\n", path.c_str());
                    Serial1.print("OK\n");
                } else {
                    Serial.println("Firebase thất bại: " + fbdo.errorReason());
                    Serial1.print("ERROR\n");
                }

                // Ghi log trạng thái
                String statusMessage;
                if (status == 'a') statusMessage = "BÁO ĐỘNG";
                else if (status == 's') statusMessage = "DỪNG CẢNH BÁO";
                else if (status == 'n') statusMessage = "TRẠNG THÁI BÌNH THƯỜNG";
                else statusMessage = "TRẠNG THÁI KHÔNG XÁC ĐỊNH";

                logEvent("ID:" + id + " - " + statusMessage);
            }
        }

        rx_buffer = "";
        data_received = false;
        delay(200);
        sendReady();
    }

    if (ready_sent && !data_received && millis() - last_ready_time > ready_timeout) {
        Serial.println("Timeout - gửi lại READY");
        sendReady();
    }
}