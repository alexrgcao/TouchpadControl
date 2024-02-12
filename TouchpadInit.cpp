#include "TouchpadInit.h"
#include <windows.h>
#include <string>
#include <iostream>
#include <hidusage.h>
#include <fstream>
#include <nlohmann/json.hpp>

TouchpadInit::TouchpadInit() {
    minTouchpadPoint.x = 0;
    minTouchpadPoint.y = 0;
    maxTouchpadPoint.x = 0;
    maxTouchpadPoint.y = 0;
}

TouchpadInit::~TouchpadInit() {
}

/*
 * Save the calibration data to a file 
 */
void TouchpadInit::SaveCalibrationData() {
    nlohmann::json j;
    std::ifstream ifile("calibration.json");
    if (ifile.is_open()) {
        try {
            ifile >> j;
        }
        catch (const std::exception& e) {
            OutputDebugStringA(e.what());
            return;
        }
    }

    j["maxTouchpadPoint.x"] = maxTouchpadPoint.x;
    j["maxTouchpadPoint.y"] = maxTouchpadPoint.y;

    std::ofstream ofile("calibration.json");
    if (!ofile) {
        OutputDebugStringA("Failed to open file\n");
        return;
    }

    try {
        ofile << j;
    }
    catch (const std::exception& e) {
        OutputDebugStringA(e.what());
        return;
    }
}

/*
 * Load the calibration data from a file
 */
bool TouchpadInit::LoadCalibrationData() {
    std::ifstream file("calibration.json");
    if (!file) {
        return false;
    }

    nlohmann::json j;
    try {
        file >> j;
    }
    catch (const std::exception& e) {
        return false;
    }

    if (!j.contains("maxTouchpadPoint.x") || !j.contains("maxTouchpadPoint.y")) {
        return false;
    }

    try {
        maxTouchpadPoint.x = j.at("maxTouchpadPoint.x").get<double>();
        maxTouchpadPoint.y = j.at("maxTouchpadPoint.y").get<double>();
    }
    catch (const std::exception& e) {
        return false;
    }

    return true;
}

/*
 * Register the touchpad as a raw input device
 */
void TouchpadInit::mRegisterRawInput(HWND hwnd) {
    RAWINPUTDEVICE rid;
    clock_t ts = clock();

    rid.usUsagePage = HID_USAGE_PAGE_DIGITIZER;
    rid.usUsage     = HID_USAGE_DIGITIZER_TOUCH_PAD;
    rid.dwFlags     = RIDEV_INPUTSINK;
    rid.hwndTarget  = hwnd;

    if (RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE))) {
        std::string message = "[" + std::to_string(ts) + "] Successfully registered touchpad!\n";
        OutputDebugStringA(message.c_str());
    } else {
        std::string message = "[" + std::to_string(ts) + "] Failed to register touchpad at " + __FILE__ + ":" + std::to_string(__LINE__) + "\n";
        OutputDebugStringA(message.c_str());
        exit(-1);
    }
}

/*
 * Calibrate the touchpad by finding the maximum x and y values
 */
void TouchpadInit::CalibrateTouchpad(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    
    if (message == WM_INPUT) {
        UINT dwSize;

        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
        LPBYTE lpb = new BYTE[dwSize];
        if (lpb == NULL) {
            return;
        }

        if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize ) {
            return;
        }

        RAWINPUT* raw = (RAWINPUT*)lpb;

        if (raw->header.dwType == RIM_TYPEHID) {
            if (raw->data.hid.dwSizeHid >= 6) {
                int x = raw->data.hid.bRawData[2] | (raw->data.hid.bRawData[3] << 8);
                int y = raw->data.hid.bRawData[4] | (raw->data.hid.bRawData[5] << 8);

                if (x > maxTouchpadPoint.x) {
                    maxTouchpadPoint.x = x;
                }
                if (y > maxTouchpadPoint.y) {
                    maxTouchpadPoint.y = y;
                }
                
            } else {
                return;
            }
        }

        delete[] lpb;
    }
}

POINT TouchpadInit::GetMinTouchpadPoint() const {
    return minTouchpadPoint;
}

POINT TouchpadInit::GetMaxTouchpadPoint() const {
    return maxTouchpadPoint;
}

