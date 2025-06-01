#include <stdio.h>
#include <driver/uart.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <string.h>
#include <esp_log.h>
#include "rs485.h"

#define UART_NUM           UART_NUM_1
#define TXD_PIN            8
#define RXD_PIN            7
#define UART_BAUD_RATE     115200
#define BUF_SIZE           1024
#define CONV_PIN 9

#define BLINK_GPIO 8
static const char* TAG = "RS485_TX";
uint8_t tx_buffer[50];
uint8_t rx_buffer[50];
bool gpio_state= 1;

QueueHandle_t uart_event_queue;

void RS485_SetTX(void);
void RS485_SetRX(void);
void uart_event_task(void *pvParameter);
void RS485_rx_task(void *pvParameters);



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
    ESP_LOGI(TAG, "Sent: %s", data);
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
void RS485_Init(void)
{
    uart_init();
    gpio_reset_pin(CONV_PIN);
    gpio_set_direction(CONV_PIN,GPIO_MODE_OUTPUT);
    
    xTaskCreate(RS485_rx_task, "RS485_rx_task", 2048 * 4, NULL, 5, NULL);

}
void RS485_SetTX()
{
    gpio_set_level(CONV_PIN,1);
}
void RS485_SetRX()
{
    gpio_set_level(CONV_PIN,0);
}
void uart_event_task(void *pvParameter)
{

    uart_event_t event;
    uint8_t cnt = 0;
    size_t len = 0;
    RS485_SetRX();
    while (1)
    {
        if (xQueueReceive(uart_event_queue, (void*)&event,portMAX_DELAY) == pdTRUE)
        {
            switch (event.type)
            {
                
                case UART_DATA:
                    uart_read_bytes(UART_NUM_1, rx_buffer, event.size, portMAX_DELAY);
                    
                    printf("Received data: %.*s\n", event.size, rx_buffer);
                    // if(strncmp((char*)rx_buffer,"{gpio:1}",8) == 0)
                    // {
                       

                        
                    // }

                    ESP_LOGI(TAG,"Received : %.*s",event.size,rx_buffer);
                    memset(rx_buffer,0,sizeof(rx_buffer));
                    break;
                case UART_FRAME_ERR:
                    ESP_LOGE(TAG,"UART_FRAME_ERR");
                    break;
                    default:break;
            }     
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }

}


void RS485_rx_task(void *pvParameters) {

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
        
            // ESP_LOGI(TAG, "Received data (%d bytes): %s", len, data);
            printf("Received data (%d bytes): %.*s\n", len, len, data);


        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    assert(false);
}