#pragma comment(lib, "dxva2.lib")
#include "Touchpad_Func.h"
#include <iostream>
#include <vector>
#include <windows.h>
#include <highlevelmonitorconfigurationapi.h>
#include <physicalmonitorenumerationapi.h>
#include <string>
#include <setupapi.h>
#include <devguid.h>
#include <stdio.h>
#include <ntddvdeo.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <chrono>

/* 
 * Set the values for the touchpad
 */
void TouchpadFunc::set_values(TouchpadInit& touchpadInit, double left_right_width, double left_right_height, double top_width, double top_height, double left_corner1, double right_corner1) {
    POINT maxPoint = touchpadInit.GetMaxTouchpadPoint();
    POINT minPoint = touchpadInit.GetMinTouchpadPoint();
    
    int left_right_width_calc = static_cast<int>(std::round(maxPoint.x * left_right_width));
    int top_height_calc = static_cast<int>(std::round(maxPoint.y * top_height));

    int left_right_height_bottom = static_cast<int>(std::round((maxPoint.y/2.0) + (maxPoint.y * left_right_height)/2));
    int left_right_height_top = static_cast<int>(std::round((maxPoint.y/2.0) - (maxPoint.y * left_right_height)/2));
    int top_width_left = static_cast<int>(std::round((maxPoint.x/2.0) - (maxPoint.x * top_width)/2));
    int top_width_right = static_cast<int>(std::round((maxPoint.x/2.0) + (maxPoint.x * top_width)/2));

    this->x_left = std::vector<int>{minPoint.x, left_right_width_calc, left_right_height_top, left_right_height_bottom};
    this->x_right = std::vector<int>{maxPoint.x - left_right_width_calc, maxPoint.x, left_right_height_top, left_right_height_bottom};
    this->y = std::vector<int>{minPoint.y, top_height_calc, top_width_left, top_width_right};

    this->left_corner = static_cast<int>(std::round(maxPoint.y * left_corner1));
    this->right_corner = static_cast<int>(std::round(maxPoint.y * right_corner1));

}

/* 
 * Check if the point is within the corner area
 */
bool TouchpadFunc::check_overlap() {
    if (x_left[2] < left_corner || x_right[2] < right_corner) {
        return true;
    }

    return false;
}

/* 
 * This function sets the brightness of the display.
 * It uses the Windows DeviceIoControl function to send a IOCTL_VIDEO_SET_DISPLAY_BRIGHTNESS control code to the display driver.
 */
void TouchpadFunc::setBrightness(DWORD brightness) {
    DISPLAY_BRIGHTNESS displayBrightness;
    displayBrightness.ucDisplayPolicy = DISPLAYPOLICY_BOTH;
    displayBrightness.ucACBrightness = static_cast<UCHAR>(brightness);
    displayBrightness.ucDCBrightness = static_cast<UCHAR>(brightness);

    DWORD bytesReturned;
    HANDLE hMonitor = CreateFile(L"\\\\.\\LCD", GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hMonitor == INVALID_HANDLE_VALUE) {
        printf("CreateFile failed with error: %d\n", GetLastError());
        return;
    }

    BOOL result = DeviceIoControl(hMonitor, IOCTL_VIDEO_SET_DISPLAY_BRIGHTNESS, &displayBrightness, sizeof(displayBrightness), NULL, 0, &bytesReturned, NULL);
    if (!result) {
        printf("DeviceIoControl failed with error: %d\n", GetLastError());
    }

    CloseHandle(hMonitor);
}

/*
 * This function sets the system's master volume level.
 * It uses the Windows Core Audio APIs to interact with the audio endpoint devices.
 */
void TouchpadFunc::setVolume(int volume) {
    float volumeLevel = static_cast<float>(volume) / 100;

    CoInitialize(NULL);

    IMMDeviceEnumerator *deviceEnumerator = NULL;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);

    IMMDevice *defaultDevice = NULL;
    hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
    deviceEnumerator->Release();
    deviceEnumerator = NULL;

    IAudioEndpointVolume *endpointVolume = NULL;
    hr = defaultDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID *)&endpointVolume);
    defaultDevice->Release();
    defaultDevice = NULL;

    hr = endpointVolume->SetMasterVolumeLevelScalar(volumeLevel, NULL);
    endpointVolume->Release();

    CoUninitialize();
    
}
