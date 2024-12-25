#ifdef __SWITCH__
#include <errno.h>
#include <cstdio>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include "common.h"
#include "pthread.h"
#include <math.h>

#define MAX_DELTA 1 / 60.0f

HidVibrationDeviceHandle VibrationDeviceHandles[2][2];
HidVibrationValue VibrationValue;
HidVibrationValue VibrationTargetStart;
HidVibrationValue VibrationTargetStop;
HidVibrationValue VibrationTargetMid;

float lerp = 0;
u32 target_device=0;
Result rc = 0;
Uint32 lastTime;
float currentValue = 0.0f;

// Lerp function
float NSX_lerp(float start, float end, float t) {
    return (1 - t) * start + t * end;
}

void NSX_RumbleLow() {
    currentValue += 0.3f;
    if(currentValue > 1.0f)
        currentValue = 1.0f;
}

void NSX_RumbleMed() {
    currentValue += 0.5f;
    if(currentValue > 1.0f)
        currentValue = 1.0f;
}

void NSX_RumbleHigh() {
    currentValue += 1.0f;
    if(currentValue > 1.0f)
        currentValue = 1.0f;
}

void NSX_RumbleCustom(float r) {
    currentValue = r;
    if(currentValue > 1.0f)
        currentValue = 1.0f;
}


void NSX_RumbleUpdate()
{
    Uint32 currentTime = SDL_GetTicks();
    // Calculate delta time
    float deltaTime = (currentTime - lastTime) / 1000.0f;
    if (deltaTime > MAX_DELTA)
        deltaTime = MAX_DELTA;

    currentValue -= deltaTime * 3.5f;
    if (currentValue < 0)
        currentValue = 0;

    VibrationValue.amp_low = NSX_lerp(VibrationTargetMid.amp_low, VibrationTargetStart.amp_low, currentValue);
    VibrationValue.amp_high = NSX_lerp(VibrationTargetMid.amp_high, VibrationTargetStart.amp_high, currentValue);
    VibrationValue.freq_low = NSX_lerp(VibrationTargetMid.freq_low, VibrationTargetStart.freq_low, currentValue);
    VibrationValue.freq_high = NSX_lerp(VibrationTargetMid.freq_high, VibrationTargetStart.freq_high, currentValue);

    VibrationValue.amp_low = NSX_lerp(VibrationTargetStop.amp_low, VibrationValue.amp_low, currentValue);
    VibrationValue.amp_high = NSX_lerp(VibrationTargetStop.amp_high, VibrationValue.amp_high, currentValue);
    VibrationValue.freq_low = NSX_lerp(VibrationTargetStop.freq_low, VibrationValue.freq_low, currentValue);
    VibrationValue.freq_high = NSX_lerp(VibrationTargetStop.freq_high, VibrationValue.freq_high, currentValue);

    HidVibrationValue VibrationValues[2];
    if (R_SUCCEEDED(rc))
    {
        //Calling hidSendVibrationValue/hidSendVibrationValues is really only needed when sending new VibrationValue(s).
        //If you just want to vibrate 1 device, you can also use hidSendVibrationValue.

        memcpy(&VibrationValues[0], &VibrationValue, sizeof(HidVibrationValue));
        memcpy(&VibrationValues[1], &VibrationValue, sizeof(HidVibrationValue));

        hidSendVibrationValues(VibrationDeviceHandles[target_device], VibrationValues, 2);
    }

    lastTime = currentTime;
}

void NSX_RumbleInit()
{
    // Two VibrationDeviceHandles are returned: first one for left-joycon, second one for right-joycon.
    // Change the total_handles param to 1, and update the hidSendVibrationValues calls, if you only want 1 VibrationDeviceHandle.
    rc = hidInitializeVibrationDevices(VibrationDeviceHandles[0], 2, HidNpadIdType_Handheld, HidNpadStyleTag_NpadHandheld);

    // Setup VibrationDeviceHandles for HidNpadIdType_No1 too, since we want to support both HidNpadIdType_Handheld and HidNpadIdType_No1.
    if (R_SUCCEEDED(rc)) rc = hidInitializeVibrationDevices(VibrationDeviceHandles[1], 2, HidNpadIdType_No1, HidNpadStyleTag_NpadJoyDual);

    VibrationTargetStart.amp_low = 6.0f;
    VibrationTargetStart.amp_high = 6.0f;
    VibrationTargetStart.freq_low = 160.0f;
    VibrationTargetStart.freq_high = 150.0f;

    VibrationTargetMid.amp_low = 2.0f;
    VibrationTargetMid.amp_high = 2.0f;
    VibrationTargetStop.freq_low = 100.0f;
    VibrationTargetStop.freq_high = 120.0f;

    VibrationTargetStop.amp_low = 0.0f;
    VibrationTargetStop.amp_high = 0.0f;
    VibrationTargetStop.freq_low = 80.0f;
    VibrationTargetStop.freq_high = 90.0f;

    // Configure our supported input layout: a single player with standard controller styles
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    // Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
    PadState pad;
    padInitializeDefault(&pad);

    // Scan the gamepad. This should be done once for each frame
    padUpdate(&pad);

    //Select which devices to vibrate.
    target_device = padIsHandheld(&pad) ? 0 : 1;

    lastTime = SDL_GetTicks();
}

#endif