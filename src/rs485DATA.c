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
#define UART_BAUD_RATE     115200
#define BUF_SIZE           1024


static const char* TAGRX = "RS485_RX";
static const char* TAGTX = "RS485_TX";
uint8_t tx_buffer[50];


QueueHandle_t uart_event_queue;

void RS485_SetTX(void);
void RS485_SetRX(void);
void rs485_tx_data(void *pvParameters);


void RS485_Send(uart_port_t uart_port,uint8_t* buf,uint16_t size)
{
    RS485_SetTX();
    uart_write_bytes(uart_port,buf,size);
    uart_wait_tx_done(uart_port,portMAX_DELAY);
    RS485_SetRX();
}

void RS485_Send_data(const char* data) {
    RS485_SetTX();
    uart_write_bytes(UART_NUM, data, strlen(data));
    uart_wait_tx_done(UART_NUM,portMAX_DELAY);
    RS485_SetRX();
    ESP_LOGI(TAGTX, "Sent: %s", data);
}

void uart_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
        };
    uart_param_config(UART_NUM_1, &uart_config);
    // uart_set_pin(UART_NUM_1,5,4,UART_PIN_NO_CHANGE,UART_PIN_NO_CHANGE);
    uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    
    uart_driver_install(UART_NUM_1, BUF_SIZE * 2, BUF_SIZE * 2, 30, &uart_event_queue, 0);
    
}
void RS485_DATA_Init(void)
{
    uart_init();
    gpio_reset_pin(CONV_PIN);
    gpio_set_direction(CONV_PIN,GPIO_MODE_OUTPUT);
    xTaskCreate(rs485_tx_data, "rs485_tx_data", 2048 * 4, NULL, 5, NULL);
    
}
void RS485_SetTX()
{
    gpio_set_level(CONV_PIN,1);
}
void RS485_SetRX()
{
    gpio_set_level(CONV_PIN,0);
}

void rs485_tx_data(void *pvParameters) {
    ESP_LOGI(TAGTX, "RS485 TX task started");
    
    while (1) {
        // Example data to send
        snprintf((char*)tx_buffer, sizeof(tx_buffer), "Hello from RS485!\n");
        
        // Send data
        RS485_Send_data((const char*)tx_buffer);
        
        // Delay for a while before sending next data
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


