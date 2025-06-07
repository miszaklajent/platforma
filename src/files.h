#pragma once

typedef struct {
    int cellX0[4];
    int cellX1[4];
    float cellY0[4];
    float cellY1[4];
}CalibData;

void filesystem_config();

void esptool_path();

void test_struct();

void Set_calib_point(int x, float y, int pointNum, int cellNum);