#include <DHT.h>

#define DHTPIN 2
#define DHTTYPE DHT11
#define MY_ADDRESS 0x01 //0x02 cho arduino 2
#define BUZZER_PIN 3

DHT dht(DHTPIN, DHTTYPE);
bool alerting = false;
void setup() {
  Serial.begin(9600);
  dht.begin();
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
}

void loop() {
  if (Serial.available() >= 2) {
    uint8_t addr = Serial.read();
    char cmd = Serial.read();

    while (Serial.available()) Serial.read();  // Clear remaining

    if (addr == MY_ADDRESS && cmd == 'R') {
      float temp = dht.readTemperature();
      float humi = dht.readHumidity();

      if (isnan(temp) || isnan(humi)) {
        Serial.println("Sensor Error");
      } else {
        char status;

        if (temp > 35.0 || humi > 90.0) {
          status = 'a';
          if (!alerting) {
            digitalWrite(BUZZER_PIN, HIGH);
            alerting = true;
          }
        } else {
          if (alerting) {
            digitalWrite(BUZZER_PIN, LOW);
            alerting = false;
            status = 's';
          } else {
            status = 'n';
          }
        }

        Serial.print("T:");
        Serial.print(temp, 1);
        Serial.print(" H:");
        Serial.print(humi, 1);
        Serial.println(status); // 'a' nếu vượt ngưỡng, 's' nếu vừa trở lại bình thường, 'n' nếu luôn bình thường
      }
    }
  }
}



