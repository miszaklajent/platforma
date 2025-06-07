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

// struct CalibData data;

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
    if (json == NULL) {
        printf("Failed to parse JSON\n");
        free(json_content);
        return NULL;
    }

    CalibData *data = malloc(sizeof(CalibData));

    for (int i = 0; i < 4; i++) {
        char key[10];
        sprintf(key, "cell%d", i);
        cJSON *cell = cJSON_GetObjectItem(json, key);

        if (cell) {
            data->cellX0[i] = cJSON_GetObjectItem(cell, "x0")->valueint;
            data->cellX1[i] = cJSON_GetObjectItem(cell, "x1")->valueint;
            data->cellY0[i] = (float)cJSON_GetObjectItem(cell, "y0")->valuedouble;
            data->cellY1[i] = (float)cJSON_GetObjectItem(cell, "y1")->valuedouble;
        }
    }

    cJSON_Delete(json);
    free(json_content);
    return data;
}

// void Set_calib_point(int x, float y, int pointNum, int cellNum) {
//     CalibData *data = get_calib_data();
//     if (data == NULL) {
//         printf("Error: Data structure is NULL\n");
//         return;
//     }
    
//     if (cellNum < 0 || cellNum >= 4 || pointNum < 0 || pointNum >= 2) {
//         printf("Error: Invalid cellNum or pointNum\n");
//         return;
//     }

//     if (pointNum == 0) {
//         data->cellX0[cellNum] = x;
//         data->cellY0[cellNum] = y;
//     } else {
//         data->cellX1[cellNum] = x;
//         data->cellY1[cellNum] = y;
//     }
// }

void Set_calib_point(int x, float y, int pointNum, int cellNum) {
    CalibData *data = get_calib_data();
    if (data == NULL) {
        printf("Error: Data structure is NULL\n");
        return;
    }

    if (cellNum < 0 || cellNum >= 4 || pointNum < 0 || pointNum >= 2) {
        printf("Error: Invalid cellNum or pointNum\n");
        return;
    }

    if (pointNum == 0) {
        data->cellX0[cellNum] = x;
        data->cellY0[cellNum] = y;
    } else {
        data->cellX1[cellNum] = x;
        data->cellY1[cellNum] = y;
    }

    cJSON *json = cJSON_CreateObject();
    for (int i = 0; i < 4; i++) {
        cJSON *cell = cJSON_CreateObject();
        cJSON_AddNumberToObject(cell, "x0", data->cellX0[i]);
        cJSON_AddNumberToObject(cell, "y0", data->cellY0[i]);
        cJSON_AddNumberToObject(cell, "x1", data->cellX1[i]);
        cJSON_AddNumberToObject(cell, "y1", data->cellY1[i]);

        char key[10];
        sprintf(key, "cell%d", i);
        cJSON_AddItemToObject(json, key, cell);
    }

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

    // printf("Calibration data updated and saved to %s\n", filename);
}



void test_struct() {
    CalibData *data = get_calib_data();
    if (data == NULL) {
        printf("No data available\n");
        return;
    }

    for (int i = 0; i < 4; i++) {
        char buffer[150];
        snprintf(buffer, sizeof(buffer), "\nCell %d: X0: %d, X1: %d, Y0: %.2f, Y1: %.2f", 
                 i, data->cellX0[i], data->cellX1[i], data->cellY0[i], data->cellY1[i]);
        printf("%s", buffer);
        CLI_RS485_Send_data(buffer);

    }
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

        // parse the JSON data
    cJSON *json = cJSON_Parse(buffer);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            printf("Error: %s\n", error_ptr);
        }
        cJSON_Delete(json);
        return;
    }


    // access the JSON data
    cJSON *val = cJSON_GetObjectItemCaseSensitive(json, "val");
    printf("val: %d\n", val->valueint);
    cJSON *val2 = cJSON_GetObjectItemCaseSensitive(json, "val2");
    printf("val2: %s\n", val2->valuestring);



    // delete the JSON object
    cJSON_Delete(json);


}






// void app_main() {

    // char line[64];
    // while (fgets(line, sizeof(line), f) != NULL) {
    //     ESP_LOGI(TAG, "Read from file: %s", line);
    // }
    // fclose(f);

    // char line[64];
    // fgets(line, sizeof(line), f);
    // fclose(f);
    // printf("\nRead from file: %s\n\n", line);

    // Read the entire file into a buffer

    // size_t size = ftell(f);
    // printf("read file:\nesptool.py read_flash 0x110205 0x%x %s\n\n", (int)size, "reading.json");
    // printf("File size: %zu bytes\n", size);
    // printf("Read %d bytes from file\n", len);
    // printf("esptool.py read_flash 0x110205 0x%x reading.json\n", len);


    // size_t size = ftell(f);
    // printf("read file:\nesptool.py read_flash 0x110205 0x%x reading.json\n\n", (int)size);

// }