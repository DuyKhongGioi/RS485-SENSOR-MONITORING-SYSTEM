#include "stm32f4xx.h"
#include <string.h>
#include <stdio.h>

// ==== Khai báo hàm ====
void GPIO_Init(void);
void USART6_Init(void);
void UART2_Init(void);
void UART1_Init(void);
void UART2_Transmit(uint8_t *data, uint32_t size);
void UART1_Transmit(uint8_t *data, uint32_t size);
uint8_t USART6_Receive(uint8_t *success);
void Send_Command(uint8_t address, char command);
void Log_Message(const char *msg);
int Receive_Response(char *buffer, int max_len);
void Reset_RS485_Line(void);
void Delay_Short(void);
void TIM10_Init(void);
// ==== DHT11 ====
int DHT22_Read(float *temperature, float *humidity);
void delay_us(uint32_t us);
void delay_ms(uint32_t ms);
// ==== Bộ đệm ====
uint8_t rx_buffer[100];

// ==== main ====
int main(void) {
    SystemCoreClockUpdate();
    GPIO_Init();
    USART6_Init();
    UART2_Init();   // UART2: Gửi tới LabVIEW (qua USB-UART converter)
    UART1_Init();   // UART1: Gửi tới ESP32
    TIM10_Init();

    GPIOA->BSRR = GPIO_BSRR_BR_0; // DE = LOW

    uint8_t current_addr = 0x01;
    float t1 = 0, h1 = 0, t2 = 0, h2 = 0, t3 = 0, h3 = 0;
    int has_data_1 = 0, has_data_2 = 0, has_data_3 = 0;

    while (1) {
        if (current_addr == 0x03) {
            // Đọc dữ liệu từ DHT22 trực tiếp nối với STM32
            if (DHT22_Read(&t3, &h3)) {
                char status = 'n';
                if (t3 > 30.0f) status = 'a';
                else if (h3 > 70.0f) status = 's';

                char msg[64];
                snprintf(msg, sizeof(msg), "ID:03,T:%.1f H:%.1f,%c\n", t3, h3, status);
                UART1_Transmit((uint8_t*)msg, strlen(msg));
                has_data_3 = 1;
            }
        } else {
            // Gửi lệnh đọc tới Arduino
            GPIOA->BSRR = GPIO_BSRR_BS_0; // DE = HIGH
            Send_Command(current_addr, 'R');
            while (!(USART6->SR & USART_SR_TC));
            GPIOA->BSRR = GPIO_BSRR_BR_0; // DE = LOW

            int len = Receive_Response((char*)rx_buffer, sizeof(rx_buffer));
            if (len > 0) {
                rx_buffer[len] = '\0';

                float temp = 0, humi = 0;
                char *t_ptr = strstr((char*)rx_buffer, "T:");
                char *h_ptr = strstr((char*)rx_buffer, "H:");
                char status = 'n';
                if (len >= 2) status = rx_buffer[len - 2]; // Lấy ký tự cảnh báo trước '\n'

                if (t_ptr && h_ptr) {
                    if (sscanf(t_ptr, "T:%f", &temp) == 1 &&
                        sscanf(h_ptr, "H:%f", &humi) == 1) {
                        char msg[64];
                        if (current_addr == 0x01) {
                            t1 = temp;
                            h1 = humi;
                            has_data_1 = 1;
                            snprintf(msg, sizeof(msg), "ID:01,T:%.1f H:%.1f,%c\n", t1, h1, status);
                        } else if (current_addr == 0x02) {
                            t2 = temp;
                            h2 = humi;
                            has_data_2 = 1;
                            snprintf(msg, sizeof(msg), "ID:02,T:%.1f H:%.1f,%c\n", t2, h2, status);
                        }
                        UART1_Transmit((uint8_t*)msg, strlen(msg));
                    }
                }
            }
        }

        // Đổi thiết bị
        if (current_addr == 0x01) current_addr = 0x02;
        else if (current_addr == 0x02) current_addr = 0x03;
        else current_addr = 0x01;

        // Khi đủ dữ liệu từ 3 thiết bị thì gửi lên LabVIEW
        if (has_data_1 && has_data_2 && has_data_3) {
            char msg[128];
            snprintf(msg, sizeof(msg), "%.1f,%.1f,%.1f,%.1f,%.1f,%.1f\r\n",
                     t3, h3, t1, h1, t2, h2);
            UART2_Transmit((uint8_t*)msg, strlen(msg));
            has_data_1 = has_data_2 = has_data_3 = 0;
        }

        Reset_RS485_Line();
        Delay_Short();
    }
}

// ===================== Các hàm phụ =========================

void GPIO_Init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIOBEN;
    // USART6: PC6 (TX), PC7 (RX)
    GPIOC->MODER |= (2 << (6 * 2)) | (2 << (7 * 2));
    GPIOC->AFR[0] |= (8 << (6 * 4)) | (8 << (7 * 4));
    // UART2: PA2 (TX)
    GPIOA->MODER |= (2 << (2 * 2));
    GPIOA->AFR[0] |= (7 << (2 * 4));
    // UART1: PA9 (TX)
    GPIOA->MODER |= (2 << (9 * 2));
    GPIOA->AFR[1] |= (7 << ((9 - 8) * 4));
    // DE: PA0
    GPIOA->MODER |= (1 << (0 * 2));
    GPIOA->OTYPER &= ~(1 << 0);
    GPIOA->PUPDR &= ~(3 << 0);
    // DHT: PA0 dùng input pull-up
    GPIOA->MODER &= ~(3 << 0);
    GPIOA->PUPDR |= (1 << 0);
}

void USART6_Init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_USART6EN;
    USART6->BRR = (104 << 4) | 3; // 9600 baud @16MHz
    USART6->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

void UART2_Init(void) {
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    USART2->BRR = (104 << 4) | 3;
    USART2->CR1 = USART_CR1_UE | USART_CR1_TE;
}

void UART1_Init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    USART1->BRR = (104 << 4) | 3;
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE;
}

void UART2_Transmit(uint8_t *data, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        while (!(USART2->SR & USART_SR_TXE));
        USART2->DR = data[i];
    }
    while (!(USART2->SR & USART_SR_TC));
}

void UART1_Transmit(uint8_t *data, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        while (!(USART1->SR & USART_SR_TXE));
        USART1->DR = data[i];
    }
    while (!(USART1->SR & USART_SR_TC));
}

void Log_Message(const char *msg) {
    UART2_Transmit((uint8_t*)msg, strlen(msg));
}

void Send_Command(uint8_t address, char command) {
    uint8_t packet[2] = {address, command};
    for (int i = 0; i < 2; i++) {
        while (!(USART6->SR & USART_SR_TXE));
        USART6->DR = packet[i];
    }
}

uint8_t USART6_Receive(uint8_t *success) {
    uint32_t timeout = 300000;
    while (!(USART6->SR & USART_SR_RXNE) && timeout--) {
        if (timeout == 0) {
            *success = 0;
            return 0;
        }
    }
    *success = 1;
    return (uint8_t)(USART6->DR & 0xFF);
}

int Receive_Response(char *buffer, int max_len) {
    int i = 0;
    uint8_t success, c;
    for (; i < max_len - 1; i++) {
        c = USART6_Receive(&success);
        if (!success) break;
        buffer[i] = c;
        if (c == '\n') break;
    }
    buffer[i] = '\0';
    return i;
}

void Reset_RS485_Line(void) {
    GPIOA->BSRR = GPIO_BSRR_BR_0;
    while (USART6->SR & USART_SR_RXNE) {
        volatile uint8_t dump = USART6->DR;
        (void)dump;
    }
}

void Delay_Short(void) {
    for (volatile int i = 0; i < 2000000; i++);
}

// ===================== DHT22 ========================
void TIM10_Init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_TIM10EN;
    TIM10->PSC = 16 - 1;
    TIM10->CR1 = TIM_CR1_CEN;
}

void delay_us(uint32_t us) {
    TIM10->CNT = 0;
    while (TIM10->CNT < us);
}

void delay_ms(uint32_t ms) {
    while (ms--) delay_us(1000);
}

int DHT22_Read(float *temperature, float *humidity) {
    uint8_t data[5] = {0};
    uint32_t timeout;

    // Start signal
    GPIOA->MODER |= (1 << 0);       // PA0 output
    GPIOA->BSRR = (1 << 16);        // Low
    delay_ms(1);                    // 1ms
    GPIOA->BSRR = (1 << 0);         // High
    delay_us(30);
    GPIOA->MODER &= ~(3 << 0);      // Input
    timeout = 100;
    while (GPIOA->IDR & 1) if (!--timeout) return 0;
    timeout = 100;
    while (!(GPIOA->IDR & 1)) if (!--timeout) return 0;
    timeout = 100;
    while (GPIOA->IDR & 1) if (!--timeout) return 0;

    for (int i = 0; i < 40; i++) {
        while (!(GPIOA->IDR & 1));
        delay_us(40);
        if (GPIOA->IDR & 1)
            data[i / 8] |= (1 << (7 - (i % 8)));
        while (GPIOA->IDR & 1);
    }

    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) return 0;

    uint16_t raw_humi = (data[0] << 8) | data[1];
    uint16_t raw_temp = (data[2] << 8) | data[3];
    *humidity = raw_humi / 10.0;
    if (raw_temp & 0x8000) {
        raw_temp &= 0x7FFF;
        *temperature = -1.0 * (raw_temp / 10.0);
    } else {
        *temperature = raw_temp / 10.0;
    }
    return 1;
}
