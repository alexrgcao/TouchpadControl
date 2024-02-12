#pragma once
#include "TouchpadInit.h"
#include <vector>

class TouchpadFunc {
public:
    std::vector<int> x_left, x_right, y;
    int left_corner, right_corner, height_of_x, width_of_y;

    void set_values(TouchpadInit& touchpadInit, double left_right_width, double left_right_height, double top_width, double top_height, double left_corner1, double right_corner1);
    bool isWithinCornerArea(POINT maxPoint, int x, int y);
    bool check_overlap();
    void setBrightness(DWORD brightness);
    void setVolume(int volume);

};