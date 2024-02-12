#pragma once

#include <windows.h>

class TouchpadInit
{
public:
    TouchpadInit();
    ~TouchpadInit();
    void mRegisterRawInput(HWND hwnd);
    void CalibrateTouchpad(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    POINT GetMinTouchpadPoint() const;
    POINT GetMaxTouchpadPoint() const;
    void SaveCalibrationData();
    bool LoadCalibrationData();

private:
    POINT minTouchpadPoint;
    POINT maxTouchpadPoint;
};