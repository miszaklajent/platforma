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

#define PIXEL_COUNT 1
#define NEOPIXEL_PIN GPIO_NUM_48



TaskHandle_t uart_rx_task_handle = NULL;


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

// #ifdef CONFIG_IDF_TARGET_ESP32S3
#define PIN_NUM_MISO  6   // Master In Slave Out
#define PIN_NUM_MOSI  5   // Master Out Slave In
#define PIN_NUM_CLK   13   // Clock

#define PIN_NUM_CS0    4
#define PIN_NUM_CS1    3
#define PIN_NUM_CS2    2
#define PIN_NUM_CS3    1

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
long conv_to_negative(unsigned long uval){
    long sval;
    if (uval & (1 << 23)) {
        // Sign bit is set, value is negative
        sval = uval - (1 << 24);
    } else {
        // Positive value
        sval = uval;
    }
    return sval;
}

void init_spi(int miso, int mosi, int clk){
    esp_err_t ret;

    spi_bus_config_t buscfg = {
        .miso_io_num = miso,  // MISO
        .mosi_io_num = mosi,  // MOSI
        .sclk_io_num = clk,   // Clock
        .quadwp_io_num = -1,          // Not used
        .quadhd_io_num = -1,          // Not used
        .max_transfer_sz = 4096,      // Maximum transfer size in bytes
    };

    // For ESP32-C3, we typically use SPI2_HOST.
    ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "SPI bus initialized");
}

int spi_device_setup(spi_device_handle_t device_spi, int CS_pin){
    esp_err_t ret;
    spi_device_interface_config_t config = {
        .clock_speed_hz = 10 * 1000 * 1000, // 10 MHz clock
        .mode = 0,                          // SPI mode 0
        .spics_io_num = CS_pin,         // Chip select pin
        .queue_size = 3,                    // Transaction queue size
    };
    ret = spi_bus_add_device(SPI2_HOST, &config, device_spi);
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "SPI device at CS_pin: %d, added to bus", CS_pin);
    return 1;
}

int chequeADS(spi_device_handle_t *device_spi){
    esp_err_t ret;

    uint8_t set_rest_rgs[2] = {0b01000000, 0b00010011};
    spi_transaction_t tx_chq = {
        .length = sizeof(set_rest_rgs) * 8,             // 4 bytes = 32 bits
        .tx_buffer = set_rest_rgs,
        .rx_buffer = NULL,
    };
    ret = spi_device_transmit(*device_spi, &tx_chq);
    ESP_ERROR_CHECK(ret);

    // chq 1 register 
    uint8_t ask_rest_rgs[1] = {0b00100000};
    uint8_t get_rest_rgs[1] = {0};
    spi_transaction_t rx_chq = {
        .length = 4 * 8,             // 4 bytes = 32 bits
        .tx_buffer = ask_rest_rgs,
        .rx_buffer = get_rest_rgs,
    };
    ret = spi_device_transmit(*device_spi, &rx_chq);
    ESP_ERROR_CHECK(ret);
    // ESP_LOGI(TAG, "chq: 0x%02X",ads2_test_rx[1]);

    if (get_rest_rgs[1] == set_rest_rgs[1]){
        // ESP_LOGI(TAG, "ADS is connected");
        return 1;
    }
    else{
        // ESP_LOGI(TAG, "ADS is not connected");
        return 0;
    }
    // return 1;
}

int ADS_cell_setup(spi_device_handle_t *device_spi){
    esp_err_t ret;

    uint8_t set_setup_data[5] = {0x43, 0x3E, 0xC4, 0x88, 0x00};
    spi_transaction_t set_cell_setup = {
        .length = sizeof(set_setup_data) * 8,  // Length in bits (5 bytes * 8 bits)
        .tx_buffer = set_setup_data,
        .rx_buffer = NULL,
    };
    ret = spi_device_transmit(*device_spi, &set_cell_setup);
    ESP_ERROR_CHECK(ret);


    uint8_t ask_setup_data[5] = {0x23, 0x00, 0x00, 0x00, 0x00};
    uint8_t get_setup_data[5] = {0};
    spi_transaction_t chq_cell_setup = {
        .length = sizeof(ask_setup_data) * 8,  // Length in bits (5 bytes * 8 bits)
        .tx_buffer = ask_setup_data,
        .rx_buffer = get_setup_data,
    };
    ret = spi_device_transmit(*device_spi, &chq_cell_setup);
    ESP_ERROR_CHECK(ret);

    for(int i=0; i<4; i++){
        if(get_setup_data[i+1] != set_setup_data[i+1]){
            // ESP_LOGI(TAG, "ADS is not connected");
            return 0;
        }
    }
    // ESP_LOGI(TAG, "ADS is connected");
    return 1;


    // ESP_LOGI(TAG, "Setup transaction completed");
}

void ADS_reset(spi_device_handle_t *device_spi){
    esp_err_t ret;

    uint8_t reset[1] = {0b00000110};
    spi_transaction_t rest_cmd = {
        .length = sizeof(reset) * 8,             // 4 bytes = 32 bits
        .tx_buffer = reset,
        .rx_buffer = NULL,
    };
    ret = spi_device_transmit(*device_spi, &rest_cmd);
    ESP_ERROR_CHECK(ret);
}

unsigned long ADS_get_cell_val(spi_device_handle_t *device_spi){
    esp_err_t ret;

    uint8_t request_tx[4] = {0x10, 0x00, 0x00, 0x00};  // Command + 3 dummy bytes
    uint8_t request_rx[4] = {0};                        // Buffer to hold response data

    spi_transaction_t trans_request = {
        .length = 4 * 8,             // 4 bytes = 32 bits
        .tx_buffer = request_tx,
        .rx_buffer = request_rx,
    };

    ret = spi_device_transmit(*device_spi, &trans_request);
    ESP_ERROR_CHECK(ret);

    // The first byte in request_rx corresponds to the transmitted command byte.
    // The next three bytes are the actual response data.
    // ESP_LOGI(TAG, "Data request transmitted; received bytes: 0x%02X, 0x%02X, 0x%02X",
    //         request_rx[1], request_rx[2], request_rx[3]);

    unsigned long results = (request_rx[1] << 16) | (request_rx[2] << 8) | request_rx[3];
    return results;
}

void pixel_set(tNeopixelContext *neopixel, int pixel_num, int r, int g, int b){
    tNeopixel pixel[] ={{ pixel_num, NP_RGB(r, g, b) }};
    neopixel_SetPixel(*neopixel, pixel, 1);
}



void app_main(void){
    // gpio_set_direction(PIN_NUM_DRDY1, GPIO_MODE_INPUT);
    // gpio_set_direction(PIN_NUM_CS1, GPIO_MODE_OUTPUT);
    // gpio_set_level(PIN_NUM_CS1, 1);


    init_uart();

    xTaskCreatePinnedToCore(
        uart_rx_task,         // Task function
        "uart_rx_task",       // Task name
        2048,                 // Stack size (bytes)
        NULL,                 // Task parameters
        5,                    // Task priority (higher numbers = higher priority)
        &uart_rx_task_handle, // Task handle
        0                     // Core ID (1 is the application core on ESP32)
    );

    vTaskDelay(pdMS_TO_TICKS(200));

    tNeopixelContext neopixel = neopixel_Init(PIXEL_COUNT, NEOPIXEL_PIN);
    pixel_set(&neopixel, 0, 10, 10, 10);
      
    init_spi(PIN_NUM_MISO, PIN_NUM_MOSI, PIN_NUM_CLK);

    spi_device_handle_t ads0;
    spi_device_handle_t ads1;
    spi_device_handle_t ads2;
    spi_device_handle_t ads3;

    spi_device_setup(&ads0, PIN_NUM_CS0);
    spi_device_setup(&ads1, PIN_NUM_CS1);
    spi_device_setup(&ads2, PIN_NUM_CS2);
    spi_device_setup(&ads3, PIN_NUM_CS3);

    int ads0_connected = chequeADS(&ads0);
    int ads1_connected = chequeADS(&ads1);
    int ads2_connected = chequeADS(&ads2);
    int ads3_connected = chequeADS(&ads3);

    ESP_LOGI(TAG, "spi0 connected:%d", ads0_connected);
    ESP_LOGI(TAG, "spi1 connected:%d", ads1_connected);
    ESP_LOGI(TAG, "spi2 connected:%d", ads2_connected);
    ESP_LOGI(TAG, "spi3 connected:%d", ads3_connected);

    ESP_LOGI(TAG, "spi0 setup done:%d", ADS_cell_setup(&ads0));
    ESP_LOGI(TAG, "spi1 setup done:%d", ADS_cell_setup(&ads1));
    ESP_LOGI(TAG, "spi2 setup done:%d", ADS_cell_setup(&ads2));
    ESP_LOGI(TAG, "spi3 setup done:%d", ADS_cell_setup(&ads3));

    long result = 0;

    for(;;){
        long results0 = conv_to_negative(ADS_get_cell_val(&ads0));
        long results1 = conv_to_negative(ADS_get_cell_val(&ads1));
        long results2 = conv_to_negative(ADS_get_cell_val(&ads2));
        long results3 = conv_to_negative(ADS_get_cell_val(&ads3));

        result = ads0_connected ? result + results0 : result;
        result = ads1_connected ? result + results1 : result;
        result = ads2_connected ? result + results2 : result;
        result = ads3_connected ? result + results3 : result;
        result = result / (ads0_connected + ads1_connected + ads2_connected + ads3_connected);

        printf("result: %f\n", result*0.001203152);
        if (ads0_connected) printf("Received data 0: %f\n", results0*0.001203152);
        if (ads1_connected) printf("Received data 1: %f\n", results1*0.001203152);
        if (ads2_connected) printf("Received data 2: %f\n", results2*0.001203152);
        if (ads3_connected) printf("Received data 3: %f\n\n", results3*0.001203152);
        vTaskDelay(pdMS_TO_TICKS(100)); // Delay for 100 ms before next transaction
    }

    // for(;;){
    //     ESP_LOGI(TAG, "spi0 connected:%d", chequeADS(&ads0));
    //     ESP_LOGI(TAG, "spi1 connected:%d", chequeADS(&ads1));
    //     ESP_LOGI(TAG, "spi2 connected:%d", chequeADS(&ads2));
    //     ESP_LOGI(TAG, "spi3 connected:%d", chequeADS(&ads3));
    //     ESP_LOGI(TAG, " ");
    //     vTaskDelay(pdMS_TO_TICKS(500)); // Delay for 100 ms before next transaction
    // }

}
