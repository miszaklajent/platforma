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
#include "files.h"

#define ESP_LOG_COLOR_DISABLED 1

#define PIXEL_COUNT 1
#define NEOPIXEL_PIN GPIO_NUM_48

TaskHandle_t spi_task_handle = NULL;



static const char *TAG = "SPI_ADS_LINE";




////////////////////////////////////////////

#define NUM_SAMPLES  10

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
  
  if (count == 0) return 0;

  return sum / count;
}

////////////////////////////////////////////

void pixel_set(tNeopixelContext *neopixel, int pixel_num, int r, int g, int b){
    tNeopixel pixel[] ={{ pixel_num, NP_RGB(r, g, b) }};
    neopixel_SetPixel(*neopixel, pixel, 1);
}

void app_main(void){
  vTaskDelay(pdMS_TO_TICKS(500));

  // gpio_set_direction(PIN_NUM_DRDY1, GPIO_MODE_INPUT);
  // gpio_set_direction(PIN_NUM_CS1, GPIO_MODE_OUTPUT);
  // gpio_set_level(PIN_NUM_CS1, 1);


  filesystem_config();
  esptool_path();
  test_struct(); 

  Set_calib_point(25, 50.5, 1, 2);

  test_struct(); 

  // xTaskCreatePinnedToCore(
  //   spi_task,         // Task function
  //   "spi_task",       // Task name
  //   4096,                 // Stack size (bytes)
  //   NULL,                 // Task parameters
  //   5,                    // Task priority (higher numbers = higher priority)
  //   &spi_task_handle, // Task handle
  //   0                     // Core ID (1 is the application core on ESP32)
  // );

  CLI_RS485_Init();
  DATA_RS485_Init();


  // char data[16];
  // snprintf(data, sizeof(data), "liczba %d\n", 20); // Convert counter to string
  // DATA_RS485_Send_data(data);






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
