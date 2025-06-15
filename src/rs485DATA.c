#include <stdio.h>
#include <driver/uart.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <esp_timer.h>
#include <string.h>
#include <esp_log.h>
#include "rs485DATA.h"

#define UART_NUM           UART_NUM_2
#define TXD_PIN            5
#define RXD_PIN            3
#define CONV_PIN           4
#define UART_BAUD_RATE     921600
#define BUF_SIZE           1024

static const char* TAGRX = "DATA_RS485_RX";
static const char* TAGTX = "DATA_RS485_TX";


QueueHandle_t DATA_uart_event_queue;

// void DATA_rs485_tx_data(void *pvParameters);
void DATA_RS485_SetTX(){ gpio_set_level(CONV_PIN,1); }
void DATA_RS485_SetRX(){ gpio_set_level(CONV_PIN,0); }
void periodic_callback(void* arg);


void start_microsecond_timer() {
    const esp_timer_create_args_t timer_args = {
        .callback = &periodic_callback,
        .name = "900us_timer"
    };

    esp_timer_handle_t timer;
    esp_timer_create(&timer_args, &timer);
    esp_timer_start_periodic(timer, 1200);
}


void DATA_uart_init(void){
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
        };
    uart_param_config(UART_NUM, &uart_config);
    // uart_set_pin(UART_NUM_1,5,4,UART_PIN_NO_CHANGE,UART_PIN_NO_CHANGE);
    uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    
    uart_driver_install(UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 30, &DATA_uart_event_queue, 0);
}

void DATA_RS485_Init(void){
    DATA_uart_init();
    gpio_reset_pin(CONV_PIN);
    gpio_set_direction(CONV_PIN,GPIO_MODE_OUTPUT);

    
    // start_microsecond_timer();
    // xTaskCreate(DATA_rs485_tx_data, "DATA_rs485_tx_data", 2048 * 4, NULL, 5, NULL);
}

void DATA_RS485_Send_data(const char* data) {
    DATA_RS485_SetTX();
    uart_write_bytes(UART_NUM, data, strlen(data));
    uart_wait_tx_done(UART_NUM,portMAX_DELAY);
    DATA_RS485_SetRX();
    // ESP_LOGI(TAGTX, "Sent: %s", data);
}



void periodic_callback(void* arg) {
    // char tx_buffer[128];
    // snprintf((char*)tx_buffer, sizeof(tx_buffer), "lc,%d,%.4f\n", (int)esp_timer_get_time(), 75.5);
    // DATA_RS485_Send_data((const char*)tx_buffer);

    // snprintf((char*)tx_buffer, sizeof(tx_buffer), "ps,%d,%.4f\n", (int)esp_timer_get_time(), 1.2);
    // DATA_RS485_Send_data((const char*)tx_buffer);
}

void callback(const float pressure, const float weight) {
    char tx_buffer[128];
    snprintf((char*)tx_buffer, sizeof(tx_buffer), "lc,%d,%.4f\n", (int)esp_timer_get_time(), weight);
    DATA_RS485_Send_data((const char*)tx_buffer);

    snprintf((char*)tx_buffer, sizeof(tx_buffer), "ps,%d,%.4f\n", (int)esp_timer_get_time(), pressure);
    DATA_RS485_Send_data((const char*)tx_buffer);
}


// char tx_buffer[128]; 
// void DATA_rs485_tx_data(void *pvParameters) {
//     ESP_LOGI(TAGTX, "DATA RS485 TX task started");
    
//     for(;;) { // format będzie taki: "lc,czas[us],kg\n",   "ps,czas[us],bar(czy cośtam innego)\n"

//         snprintf((char*)tx_buffer, sizeof(tx_buffer), "lc,%d,%.4f\n", (int)esp_timer_get_time(), 75.5);
//         DATA_RS485_Send_data((const char*)tx_buffer);

//         snprintf((char*)tx_buffer, sizeof(tx_buffer), "ps,%d,%.4f\n", (int)esp_timer_get_time(), 1.2);
//         DATA_RS485_Send_data((const char*)tx_buffer);
//         // snprintf((char*)tx_buffer, sizeof(tx_buffer), "\n");
        
        
//         vTaskDelay(pdMS_TO_TICKS(1000));
//     }
// }