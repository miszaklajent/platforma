#include "uart.h"
#include <driver/uart.h>
#include <esp_log.h>
#include <esp_err.h>

#define UART_NUM           UART_NUM_1
#define TXD_PIN            7
#define RXD_PIN            8
#define UART_BAUD_RATE     9600
#define BUF_SIZE           1024

static const char *TAG = "UART";

void init_uart(void) {

    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_APB,
    };
    
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 0, NULL, 0));
    
    ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));

    ESP_ERROR_CHECK(uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    
    ESP_LOGI(TAG, "Initialized successfully");
}


void uart_rx_task(void *pvParameters) {
    uint8_t* data = (uint8_t*) malloc(BUF_SIZE);
    
    if (data == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for UART buffer");
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "UART receive task started");
    
    while (1) {
        int len = uart_read_bytes(UART_NUM, data, BUF_SIZE - 1, 20 / portTICK_PERIOD_MS);
        if (len > 0) {
        
            ESP_LOGI(TAG, "Received data (%d bytes): %s", len, data);
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    assert(false);
}