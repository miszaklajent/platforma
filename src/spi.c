#include "spi.h"
#include <esp_log.h>
#include <esp_err.h>

static const char *TAG = "SPI_ADS_LINE";

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

    // The first byte in request_rx corresponds to the transmitted command byte.
    // The next three bytes are the actual response data.
    // ESP_LOGI(TAG, "Data request transmitted; received bytes: 0x%02X, 0x%02X, 0x%02X",
    //         request_rx[1], request_rx[2], request_rx[3]);

    unsigned long results = (request_rx[1] << 16) | (request_rx[2] << 8) | request_rx[3];
    return results;
}
