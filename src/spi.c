#include "spi.h"
#include <esp_log.h>
#include <esp_err.h>
#include <driver/gpio.h>
#include <freertos/semphr.h>
#include "files.h"

// Declare the get_calib_data function if not already declared
CalibData* get_calib_data();


#define SPI_HOST       SPI2_HOST

#define PIN_NUM_MISO  12 
#define PIN_NUM_MOSI  11 
#define PIN_NUM_CLK   10 

#define PIN_NUM_CS0    9
#define PIN_NUM_CS1    8
#define PIN_NUM_CS2    14
#define PIN_NUM_CS3    2

#define PIN_DATA_REDY 1

SemaphoreHandle_t AvgWeightMutex;
SemaphoreHandle_t rawValuesMutex;

CalibData data;


static const char *TAG = "SPI_ADS_LINE";

static long conv_to_negative(unsigned long uval){
    long sval;
    if (uval & (1 << 23)) {
        sval = uval - (1 << 24);
    } else {
        sval = uval;
    }
    return sval;
}

void init_spi(int miso, int mosi, int clk){
    CalibData *data = get_calib_data();


    AvgWeightMutex = xSemaphoreCreateMutex();
    rawValuesMutex = xSemaphoreCreateMutex();

    esp_err_t ret;

    spi_bus_config_t buscfg = {
        .miso_io_num = miso,
        .mosi_io_num = mosi,
        .sclk_io_num = clk, 
        .quadwp_io_num = -1,        
        .quadhd_io_num = -1,        
        .max_transfer_sz = 4096,    
    };

    ret = spi_bus_initialize(SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "SPI bus initialized");
}

int spi_device_setup(spi_device_handle_t device_spi, int CS_pin){
    esp_err_t ret;
    spi_device_interface_config_t config = {
        .clock_speed_hz = 10 * 1000 * 1000,
        .mode = 0,                       
        .spics_io_num = CS_pin,      
        .queue_size = 3,                 
    };
    ret = spi_bus_add_device(SPI_HOST, &config, device_spi);
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "SPI device at CS_pin: %d, added to bus", CS_pin);
    return 1;
}

int check_ads(spi_device_handle_t *device_spi){
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

int ads_cell_setup(spi_device_handle_t *device_spi){
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

void ads_reset(spi_device_handle_t *device_spi){
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

unsigned long ads_get_cell_val(spi_device_handle_t *device_spi){
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

    unsigned long results = (request_rx[1] << 16) | (request_rx[2] << 8) | request_rx[3];
    return results;
}



float Lineary_transformed_cell(int x0, int x1, int y0, int y1, int x) {
    // Linear transformation formula: y = y0 + (y1 - y0) * (x - x0) / (x1 - x0)
    return y0 + (float)(y1 - y0) * (x - x0) / (x1 - x0);
}


float result = 0;
// int results0 = 0;
// int results1 = 0;
// int results2 = 0;
// int results3 = 0;
int results[4] = {0, 0, 0, 0};
float avrage_weight = 0;
void spi_task(void *pvParameters) {
    printf("SPI task started\n");
    gpio_set_direction(PIN_DATA_REDY, GPIO_MODE_INPUT);

    init_spi(PIN_NUM_MISO, PIN_NUM_MOSI, PIN_NUM_CLK);

    spi_device_handle_t ads0;
    spi_device_handle_t ads1;
    spi_device_handle_t ads2;
    spi_device_handle_t ads3;

    spi_device_setup(&ads0, PIN_NUM_CS0);
    spi_device_setup(&ads1, PIN_NUM_CS1);
    spi_device_setup(&ads2, PIN_NUM_CS2);
    spi_device_setup(&ads3, PIN_NUM_CS3);

    int ads0_connected = check_ads(&ads0);
    int ads1_connected = check_ads(&ads1);
    int ads2_connected = check_ads(&ads2);
    int ads3_connected = check_ads(&ads3);

    ESP_LOGI(TAG, "spi0 connected:%d", ads0_connected);
    ESP_LOGI(TAG, "spi1 connected:%d", ads1_connected);
    ESP_LOGI(TAG, "spi2 connected:%d", ads2_connected);
    ESP_LOGI(TAG, "spi3 connected:%d", ads3_connected);

    ESP_LOGI(TAG, "spi0 setup done:%d", ads_cell_setup(&ads0));
    ESP_LOGI(TAG, "spi1 setup done:%d", ads_cell_setup(&ads1));
    ESP_LOGI(TAG, "spi2 setup done:%d", ads_cell_setup(&ads2));
    ESP_LOGI(TAG, "spi3 setup done:%d", ads_cell_setup(&ads3));



    for(;;){
        if (gpio_get_level(PIN_DATA_REDY) == 0) {
            

            results[0] = conv_to_negative(ads_get_cell_val(&ads0));
            results[1] = conv_to_negative(ads_get_cell_val(&ads1));
            results[2] = conv_to_negative(ads_get_cell_val(&ads2));
            results[3] = conv_to_negative(ads_get_cell_val(&ads3));


            result = ads0_connected ? result + results[0] : result;
            result = ads1_connected ? result + results[1] : result;
            result = ads2_connected ? result + results[2] : result;
            result = ads3_connected ? result + results[3] : result;
            
        } else {
            
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }




        int ads_connected = ads0_connected + ads1_connected + ads2_connected + ads3_connected;
        if (ads_connected) {
            result = result / (ads0_connected + ads1_connected + ads2_connected + ads3_connected);
        }else {
            result = 0;
        }

        if (xSemaphoreTake(AvgWeightMutex, portMAX_DELAY)) {
            avrage_weight = result * 0.001203152; // Convert to kg
            xSemaphoreGive(AvgWeightMutex);
        }

        printf("result: %f\n", result*0.001203152);
        if (ads0_connected) printf("Received data 0: %f\n", results[0]*0.001203152);
        if (ads1_connected) printf("Received data 1: %f\n", results[1]*0.001203152);
        if (ads2_connected) printf("Received data 2: %f\n", results[2]*0.001203152);
        if (ads3_connected) printf("Received data 3: %f\n\n", results[3]*0.001203152);
        vTaskDelay(pdMS_TO_TICKS(100));
        result = 0;
    }

}

int get_raw_cell_val(int *CellNum){

    for (int i = 0; i < 4; i++) {  // potential race condition!!!
        CellNum[i] = results[i];
    }
    return 0;
}

float get_cell_avrage() {
    if (xSemaphoreTake(AvgWeightMutex, portMAX_DELAY)) {
        float avg_weight = avrage_weight;
        xSemaphoreGive(AvgWeightMutex);
        return avg_weight;
    }
    return 0.0f;
}