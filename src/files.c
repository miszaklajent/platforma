#include <stdio.h>
#include <cJSON.h>
#include <string.h>
#include <esp_spiffs.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "files.h"
#include "rs485CLI.h"

static const char *TAG = "filesystem";

void esptool_path(){
    FILE *f = fopen("/storage/file.json", "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return;
    } 

    char buffer[1024];
    int len = fread(buffer, 1, sizeof(buffer), f);
    if (len < 0) {
        ESP_LOGE(TAG, "Failed to read file");
        fclose(f);
        return;
    }
    size_t size = ftell(f);
    printf("read file:\nesptool.py read_flash 0x110205 0x%x %s\n\n", (int)size, "reading.json");
    // printf("File size: %zu bytes\n", size);
    // printf("Read %d bytes from file\n", len);
    // printf("esptool.py read_flash 0x110205 0x%x reading.json\n", len);
    fclose(f);
}

void filesystem_config(){
    esp_vfs_spiffs_conf_t config = {
        .base_path = "/storage",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true,
    };
    esp_err_t result = esp_vfs_spiffs_register(&config);

    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SPIFFS (%s)", esp_err_to_name(result));
        return;
    }

    size_t total = 0, used = 0;
    result = esp_spiffs_info(config.partition_label, &total, &used);
    if(result == ESP_OK) {
        ESP_LOGI(TAG, "SPIFFS total: %d, used: %d", total, used);
    } else {
        ESP_LOGE(TAG, "Failed to get SPIFFS info (%s)", esp_err_to_name(result));
    }
}

CalibData *get_calib_data() {
    FILE *f = fopen("/storage/file.json", "r");
    if (f == NULL) {
        printf("Failed to open file for reading\n");
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    rewind(f);

    char *json_content = malloc(fsize + 1);
    fread(json_content, 1, fsize, f);
    fclose(f);

    json_content[fsize] = '\0';

    cJSON *json = cJSON_Parse(json_content);
    free(json_content);

    if (json == NULL) {
        printf("Failed to parse JSON\n");
        return NULL;
    }

    CalibData *data = malloc(sizeof(CalibData));

    cJSON *cells = cJSON_GetObjectItem(json, "cells");
    if (cells) {
        data->cellX0 = cJSON_GetObjectItem(cells, "x0")->valueint;
        data->cellY0 = (float)cJSON_GetObjectItem(cells, "y0")->valuedouble;
        data->cellX1 = cJSON_GetObjectItem(cells, "x1")->valueint;
        data->cellY1 = (float)cJSON_GetObjectItem(cells, "y1")->valuedouble;
    }

    cJSON_Delete(json);
    return data;
}

void Set_calib_point(int x, float y, int pointNum) {
    CalibData *calData = get_calib_data();
    if (calData == NULL) {
        printf("Error: calData structure is NULL\n");
        return;
    }

    if (pointNum < 0 || pointNum >= 2) {
        printf("Error: Invalid pointNum\n");
        return;
    }

    if (pointNum == 0) {
        calData->cellX0 = x;
        calData->cellY0 = y;
    } else {
        calData->cellX1 = x;
        calData->cellY1 = y;
    }

    cJSON *json = cJSON_CreateObject();
    cJSON *cells = cJSON_CreateObject();

    cJSON_AddNumberToObject(cells, "x0", calData->cellX0);
    cJSON_AddNumberToObject(cells, "y0", calData->cellY0);
    cJSON_AddNumberToObject(cells, "x1", calData->cellX1);
    cJSON_AddNumberToObject(cells, "y1", calData->cellY1);

    cJSON_AddItemToObject(json, "cells", cells);

    char *json_string = cJSON_Print(json);
    FILE *f = fopen("/storage/file.json", "w");
    if (f == NULL) {
        printf("Error: Failed to open file for writing\n");
        cJSON_Delete(json);
        free(json_string);
        return;
    }

    fprintf(f, "%s", json_string);
    fclose(f);

    cJSON_Delete(json);
    free(json_string);
}

void test_struct() {
    CalibData *calData = get_calib_data();
    if (calData == NULL) {
        printf("No calData available\n");
        return;
    }

    char buffer[150];
    snprintf(buffer, sizeof(buffer), "\nCell: X0: %d, X1: %d, Y0: %.2f, Y1: %.2f", 
                calData->cellX0, calData->cellX1, calData->cellY0, calData->cellY1);
    printf("%s", buffer);
    CLI_RS485_Send_data(buffer);
}

void read_data(){
    FILE *f = fopen("/storage/file.json", "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return;
    } 

    char buffer[1024];
    int len = fread(buffer, 1, sizeof(buffer), f);
    if (len < 0) {
        ESP_LOGE(TAG, "Failed to read file");
        fclose(f);
        return;
    }

    fclose(f);

        // parse the JSON calData
    cJSON *json = cJSON_Parse(buffer);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            printf("Error: %s\n", error_ptr);
        }
        cJSON_Delete(json);
        return;
    }


    // access the JSON calData
    cJSON *val = cJSON_GetObjectItemCaseSensitive(json, "val");
    printf("val: %d\n", val->valueint);
    cJSON *val2 = cJSON_GetObjectItemCaseSensitive(json, "val2");
    printf("val2: %s\n", val2->valuestring);



    // delete the JSON object
    cJSON_Delete(json);


}


