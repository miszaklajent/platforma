#pragma once

typedef struct {
    int cellX0;
    int cellX1;
    float cellY0;
    float cellY1;
}CalibData;

void filesystem_config();

void esptool_path();

void test_struct();

void Set_calib_point(int x, float y, int pointNum);