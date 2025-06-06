#include <stdio.h>
#include "esp_log.h"
#include "esp_err.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "neopixel.h"
#include "uart.h"
#include "spi.h"
#include "rs485CLI.h"
#include "rs485DATA.h"
#define ESP_LOG_COLOR_DISABLED 1

#define PIXEL_COUNT 1
#define NEOPIXEL_PIN GPIO_NUM_48



TaskHandle_t uart_rx_task_handle = NULL;
TaskHandle_t spi_task_handle = NULL;


// Define SPI GPIO pins (adjust these for your target hardware)

#ifdef CONFIG_IDF_TARGET_ESP32C3
#define PIN_NUM_MISO  5   // Master In Slave Out
#define PIN_NUM_MOSI  6   // Master Out Slave In
#define PIN_NUM_CLK   4   // Clock

#define PIN_NUM_CS0    10
#define PIN_NUM_CS1    9
#define PIN_NUM_CS2    8
#define PIN_NUM_CS3    7

#define PIN_NUM_DRDY1  -1
#define PIN_NUM_DRDY2  0
#endif



// #endif

#ifdef CONFIG_IDF_TARGET_ESP32
#define PIN_NUM_MISO  13   // Master In Slave Out
#define PIN_NUM_MOSI  12   // Master Out Slave In
#define PIN_NUM_CLK   11   // Clock

#define PIN_NUM_CS0    10
#define PIN_NUM_CS1    9
#define PIN_NUM_CS2    8
#define PIN_NUM_CS3    7

#endif

static const char *TAG = "SPI_ADS_LINE";




////////////////////////////////////////////

#define NUM_SAMPLES  10 // Set how many samples you want for the average

unsigned long samples[NUM_SAMPLES];
int sampleIndex = 0;
bool bufferFilled = false;

void addSample(unsigned long newResult) {
  samples[sampleIndex] = newResult;
  sampleIndex = (sampleIndex + 1) % NUM_SAMPLES;
  
  if (sampleIndex == 0) {
    bufferFilled = true;
  }
}

unsigned long getAverage() {
  unsigned long sum = 0;
  int count = bufferFilled ? NUM_SAMPLES : sampleIndex;

  for (int i = 0; i < count; i++) {
    sum += samples[i];
  }
  
  if (count == 0) return 0; // prevent division by zero

  return sum / count;
}

////////////////////////////////////////////

void pixel_set(tNeopixelContext *neopixel, int pixel_num, int r, int g, int b){
    tNeopixel pixel[] ={{ pixel_num, NP_RGB(r, g, b) }};
    neopixel_SetPixel(*neopixel, pixel, 1);
}
#define PIN_NUM_MISO  6   // Master In Slave Out
#define PIN_NUM_MOSI  5   // Master Out Slave In
#define PIN_NUM_CLK   13   // Clock


void app_main(void){
  vTaskDelay(pdMS_TO_TICKS(500));

  // gpio_set_direction(PIN_NUM_DRDY1, GPIO_MODE_INPUT);
  // gpio_set_direction(PIN_NUM_CS1, GPIO_MODE_OUTPUT);
  // gpio_set_level(PIN_NUM_CS1, 1);

  // printf("UART initialized\n");
  // init_uart();



  

  // xTaskCreatePinnedToCore(
  //     uart_rx_task,         // Task function
  //     "uart_rx_task",       // Task name
  //     2048,                 // Stack size (bytes)
  //     NULL,                 // Task parameters
  //     5,                    // Task priority (higher numbers = higher priority)
  //     &uart_rx_task_handle, // Task handle
  //     0                     // Core ID (1 is the application core on ESP32)
  // );


  xTaskCreatePinnedToCore(
      spi_task,         // Task function
      "spi_task",       // Task name
      4096,                 // Stack size (bytes)
      NULL,                 // Task parameters
      5,                    // Task priority (higher numbers = higher priority)
      &spi_task_handle, // Task handle
      0                     // Core ID (1 is the application core on ESP32)
  );

  RS485_CLI_Init();
  RS485_DATA_Init();


  // RS485_Send(UART_NUM_1,(uint8_t*)"{gpio:1}",9);s
  

  // char data[16];
  // snprintf(data, sizeof(data), "liczba %d\n", 20); // Convert counter to string
  // RS485_Send_data(data);






  tNeopixelContext neopixel = neopixel_Init(PIXEL_COUNT, NEOPIXEL_PIN);
  pixel_set(&neopixel, 0, 10, 10, 10);
    


  // for(;;){
  //     ESP_LOGI(TAG, "spi0 connected:%d", check_ads(&ads0));
  //     ESP_LOGI(TAG, "spi1 connected:%d", check_ads(&ads1));
  //     ESP_LOGI(TAG, "spi2 connected:%d", check_ads(&ads2));
  //     ESP_LOGI(TAG, "spi3 connected:%d", check_ads(&ads3));
  //     ESP_LOGI(TAG, " ");
  //     vTaskDelay(pdMS_TO_TICKS(500)); // Delay for 100 ms before next transaction
  // }

}
