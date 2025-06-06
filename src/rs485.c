#include <stdio.h>
#include <driver/uart.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <esp_timer.h>
#include <string.h>
#include <esp_log.h>
#include "rs485.h"

#define UART_NUM           UART_NUM_1
#define TXD_PIN            39
#define RXD_PIN            6
#define CONV_PIN           7
#define UART_BAUD_RATE     115200
#define BUF_SIZE           1024

#define BLINK_GPIO 8
static const char* TAGRX = "RS485_RX";
static const char* TAGTX = "RS485_TX";
uint8_t tx_buffer[50];
uint8_t rx_buffer[50];
bool gpio_state= 1;

QueueHandle_t uart_event_queue;

void RS485_SetTX(void);
void RS485_SetRX(void);
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

void ehoHelp(void) {
    char data[512];
    snprintf(data, sizeof(data), 
    "\nhelp: av commands: \n"
    "help \n"
    "calibX = Y (where X is calib point, and Y is weight put on platform in Kg) \n"
    "gravity = X (where X is gravity acceleration in your area in m/s^2) \n"
    "get force - prints current force put on platform \n"
    "get weight - prints current weight put on platform \n"
    "stats - display current stats in an interval\n");
    RS485_Send_data(data);
}

bool statsRunning = false;
void stats(void *pvParameters) {
    ESP_LOGI(TAGTX, "Stats task started");
    RS485_Send_data("Stats running! Send any key to exit...\n");

    statsRunning = true;
    int lastTimer = 0;
    while(statsRunning) {
        int timeDelay = 1000; //[ms]

        if(esp_timer_get_time()/1000-lastTimer > timeDelay) {
            lastTimer = esp_timer_get_time()/1000;
            char data[64];
            snprintf(data, sizeof(data), "Stats: %d\n", rand() % 100);
            RS485_Send_data(data);
        } else {
            vTaskDelay(1 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAGTX, "disp statistics...");
        


        // vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    RS485_Send_data("Stats exiting!\n");
    ESP_LOGI(TAGTX, "Stats task finished");
    vTaskDelete(NULL);

}




void process_data(uint8_t *data, int len) {
ESP_LOGI(TAGRX, "Processing received data...");

    // Convert to string for parsing
    char command[BUF_SIZE];
    snprintf(command, sizeof(command), "%s", (char *)data);
    printf("Command: %s\n", command);
    // printf("Command: |%.*s|\n", sizeof(command), command);


    if (statsRunning){
        statsRunning = false;
    }else if (strncmp(command, "help", 4) == 0) {
        ESP_LOGI(TAGTX, "Displaying help menu...");
        ehoHelp();
        
    } else if (strncmp(command, "calib", 5) == 0) {
        int calibPoint;
        float calibValue;
        if (sscanf(command, "calib%d = %f", &calibPoint, &calibValue) == 2) {
            char data[64];
            snprintf(data, sizeof(data), "Calibration Point: %d, Value: %.2f", calibPoint, calibValue);
            RS485_Send_data(data);
            ESP_LOGI(TAGRX, "%s", data);
        } else {
            ESP_LOGW(TAGRX, "Invalid calibration format.");
        }
    } else if (strncmp(command, "gravity", 7) == 0) {
        float gravity;
        if (sscanf(command, "gravity = %f", &gravity) == 1) {
            ESP_LOGI(TAGRX, "Gravity Set to: %.2f", gravity);



        } else {
            ESP_LOGW(TAGRX, "Invalid gravity format.");
        }
    } else if (strncmp(command, "get force", 9) == 0) {
        ESP_LOGI(TAGRX, "Fetching force value...");
        


    } else if (strncmp(command, "get weight", 10) == 0) {
        ESP_LOGI(TAGRX, "Fetching weight value...");



    } else if (strncmp(command, "stats", 5) == 0) {
        xTaskCreate(stats, "stats", 2048 * 4, NULL, 5, NULL);
    } else {
        ESP_LOGW(TAGRX, "Unknown command: %s", command);
    }

}



void RS485_rx_task(void *pvParameters) {

    uint8_t* data = (uint8_t*) malloc(BUF_SIZE);
    
    if (data == NULL) {
        ESP_LOGE(TAGRX, "Failed to allocate memory for UART buffer");
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAGRX, "UART receive task started");
    
    while (1) {
        int len = uart_read_bytes(UART_NUM, data, BUF_SIZE - 1, 20 / portTICK_PERIOD_MS);
        if (len > 0) {
            data[len] = '\0'; // Null-terminate the received data
        
            ESP_LOGI(TAGRX, "Received data (%d bytes): %s", len, data);

            process_data(data, len);
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    assert(false);
}