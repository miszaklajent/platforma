#pragma once
#include "driver/spi_master.h"

void init_spi(int miso, int mosi, int clk);

int spi_device_setup(spi_device_handle_t device_spi, int CS_pin);

int check_ads(spi_device_handle_t *device_spi);

int ads_cell_setup(spi_device_handle_t *device_spi);void ads_reset(spi_device_handle_t *device_spi);

unsigned long ads_get_cell_val(spi_device_handle_t *device_spi);
