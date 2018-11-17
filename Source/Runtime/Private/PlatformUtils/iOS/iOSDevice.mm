// Copyright(c) 2017 POLYGONTEK
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http ://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "Core/Str.h"
#include "PlatformUtils/iOS/iOSDevice.h"
#include <sys/types.h>
#include <sys/sysctl.h>

BE_NAMESPACE_BEGIN

struct IOSDeviceModel {
    const char *name;
    bool iPod, iPhone, iPad;
    int resolutionWidth, resolutionHeight;
};

// Matched with IOSDevice::Type enums
static IOSDeviceModel iOSModels[] = {
    { "iPod Touch 4th generation",  true, false, false, 960, 640 },
    { "iPod Touch 5th generation",  true, false, false, 1136, 640 },
    { "iPod Touch 6th generation",  true, false, false, 1136, 640 },

    { "iPhone 4",                   false, true, false, 960, 640 },
    { "iPhone 4s",                  false, true, false, 960, 640 },
    { "iPhone 5",                   false, true, false, 1136, 640 },
    { "iPhone 5s",                  false, true, false, 1136, 640 },
    { "iPhone 6",                   false, true, false, 1334, 750 },
    { "iPhone 6 Plus",              false, true, false, 1920, 1080 },
    { "iPhone 6s",                  false, true, false, 1334, 750 },
    { "iPhone SE",                  false, true, false, 1136, 640 },
    { "iPhone 7",                   false, true, false, 1334, 750 },
    { "iPhone 7 Plus",              false, true, false, 1920, 1080 },
    { "iPhone 8",                   false, true, false, 1334, 750 },
    { "iPhone 8 Plus",              false, true, false, 1920, 1080 },
    { "iPhone X",                   false, true, false, 2436, 1125 },
    { "iPhone Xs",                  false, true, false, 2436, 1125 },
    { "iPhone Xs Max",              false, true, false, 2688, 1242 },
    { "iPhone Xr",                  false, true, false, 1792, 828 },

    { "iPad 2",                     false, false, true, 1024, 768 },
    { "iPad Mini",                  false, false, true, 1024, 768 },
    { "iPad 3",                     false, false, true, 2048, 1536 },
    { "iPad 4",                     false, false, true, 2048, 1536 },
    { "iPad Air",                   false, false, true, 2048, 1536 },
    { "iPad Mini 2",                false, false, true, 2048, 1536 },    
    { "iPad Mini 3",                false, false, true, 2048, 1536 },
    { "iPad Air 2",                 false, false, true, 2048, 1536 },
    { "iPad Mini 4",                false, false, true, 2048, 1536 },
    { "iPad Pro 9.7",               false, false, true, 2048, 1536 },
    { "iPad Pro 12.9",              false, false, true, 2732, 2048 },
    { "iPad Pro 12.9 (2nd)",        false, false, true, 2732, 2048 },
    { "iPad Pro 10.5",              false, false, true, 2224, 1668 },
    { "iPad Pro 11",                false, false, true, 2388, 1668 },
    { "iPad Pro 12.9 (3rd)",        false, false, true, 2732, 2048 },
};

bool IOSDevice::IsIPhone(IOSDevice::Type deviceType) {
    int index = (int)deviceType;
    assert(index >= 0 && index < IOS_UnknownDevice);

    return iOSModels[index].iPhone;
}

bool IOSDevice::IsIPod(IOSDevice::Type deviceType) {
    int index = (int)deviceType;
    assert(index >= 0 && index < IOS_UnknownDevice);

    return iOSModels[index].iPod;
}

bool IOSDevice::IsIPad(IOSDevice::Type deviceType) {
    int index = (int)deviceType;
    assert(index >= 0 && index < IOS_UnknownDevice);

    return iOSModels[index].iPad;
}

void IOSDevice::GetDeviceResolution(IOSDevice::Type deviceType, int &width, int &height) {
    int index = (int)deviceType;
    assert(index >= 0 && index < IOS_UnknownDevice);

    const auto &model = iOSModels[index];
    width = model.resolutionWidth;
    height = model.resolutionHeight;
}

// Refernce: https://www.theiphonewiki.com/wiki/Models
IOSDevice::Type IOSDevice::GetIOSDeviceType() {
    // default to unknown
    static IOSDevice::Type deviceType = IOSDevice::Type::IOS_UnknownDevice;
    
    // if we've already figured it out, return it
    if (deviceType != IOSDevice::Type::IOS_UnknownDevice) {
        return deviceType;
    }
    
    // get the device hardware type string length
    size_t deviceIDLen;
    sysctlbyname("hw.machine", nullptr, &deviceIDLen, nullptr, 0);
    
    // get the device hardware type
    char *deviceID = (char *)malloc(deviceIDLen);
    sysctlbyname("hw.machine", deviceID, &deviceIDLen, nullptr, 0);
    
    // convert to NSString
    BE1::Str deviceIDString([NSString stringWithCString:deviceID encoding:NSUTF8StringEncoding]);
    free(deviceID);
    
    if (!deviceIDString.Cmpn("iPod", 4)) {
        // get major revision number
        int major = deviceIDString[4] - '0';
        
        if (major == 4) {
            deviceType = IOS_IPodTouch4;
        } else if (major == 5) {
            deviceType = IOS_IPodTouch5;
        } else if (major >= 7) {
            deviceType = IOS_IPodTouch6;
        }
    } else if (!deviceIDString.Cmpn("iPad", 4)) {
        // get major revision number
        int major = deviceIDString[4] - '0';
        int minor = deviceIDString[6] - '0';
        
        if (major == 2) {
            if (minor >= 5) {
                deviceType = IOS_IPadMini;
            } else {
                deviceType = IOS_IPad2;
            }
        } else if (major == 3) {
            if (minor <= 3) {
                deviceType = IOS_IPad3;
            } else if (minor >= 4) {
                deviceType = IOS_IPad4;
            }
        } else if (major == 4) {
            if (minor >= 8) {
                deviceType = IOS_IPadMini3;
            } else if (minor >= 4) {
                deviceType = IOS_IPadMini2;
            } else {
                deviceType = IOS_IPadAir;
            }
        } else if (major == 5) {
            if (minor >= 3) {
                deviceType = IOS_IPadAir2;
            } else {
                deviceType = IOS_IPadMini4;
            }
        } else if (major == 6) {
            if (minor >= 7) {
                deviceType = IOS_IPadPro_12_9;
            } else {
                deviceType = IOS_IPadPro_9_7;
            }
        } else if (major >= 7) { // Default to highest settings currently available for any future device
            if (minor >= 3) {
                deviceType = IOS_IPadPro2_10_5;
            } else {
                deviceType = IOS_IPadPro2_12_9;
            }
        } else if (major >= 8) {
            if (minor >= 5) {
                deviceType = IOS_IPadPro3_12_9;
            } else {
                deviceType = IOS_IPadPro3_11;
            }
        }
    } else if (!deviceIDString.Cmpn("iPhone", 6)) {
        int major = atoi(&deviceIDString[6]);
        int commaIndex = deviceIDString.Find(',');
        int minor = deviceIDString[commaIndex + 1] - '0';
        
        if (major == 3) {
            deviceType = IOS_IPhone4;
        } else if (major == 4) {
            deviceType = IOS_IPhone4S;
        } else if (major == 5) {
            deviceType = IOS_IPhone5;
        } else if (major == 6) {
            deviceType = IOS_IPhone5S;
        } else if (major == 7) {
            if (minor == 1) {
                deviceType = IOS_IPhone6Plus;
            } else if (minor == 2) {
                deviceType = IOS_IPhone6;
            }
        } else if (major == 8) {
            if (minor == 1) {
                deviceType = IOS_IPhone6S;
            } else if (minor == 2) {
                deviceType = IOS_IPhone6SPlus;
            } else if (minor == 4) {
                deviceType = IOS_IPhoneSE;
            }
        } else if (major == 9) {
            if (minor == 1 || minor == 3) {
                deviceType = IOS_IPhone7;
            } else if (minor == 2 || minor == 4) {
                deviceType = IOS_IPhone7Plus;
            }
        } else if (major == 10) {
            if (minor == 1 || minor == 4) {
                deviceType = IOS_IPhone8;
            } else if (minor == 2 || minor == 5) {
                deviceType = IOS_IPhone8Plus;
            } else if (minor == 3 || minor == 6) {
                deviceType = IOS_IPhoneX;
            }
        } else if (major == 11) {
            if (minor == 2) {
                deviceType = IOS_IPhoneXS;
            } else if (minor == 4 || minor == 6) {
                deviceType = IOS_IPhoneXSMax;
            } else if (minor == 8) {
                deviceType = IOS_IPhoneXR;
            }
        } else if (major >= 11) {
            // for going forward into unknown devices (like 8/8+?), we can't use minor,
            // so treat devices with a scale > 2.5 to be 6SPlus type devices, < 2.5 to be 6S type devices
            if ([UIScreen mainScreen].scale > 2.5f) {
                deviceType = IOS_IPhoneXSMax;
            } else {
                deviceType = IOS_IPhoneXS;
            }
        }
    } else if (!deviceIDString.Cmpn("x86", 3)) { // simulator
        if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone) {
            CGSize result = [[UIScreen mainScreen] bounds].size;
            if (result.height >= 586) {
                deviceType = IOS_IPhone5;
            } else {
                deviceType = IOS_IPhone4S;
            }
        } else {
            if ([[UIScreen mainScreen] scale] > 1.0f) {
                deviceType = IOS_IPad3;
            } else {
                deviceType = IOS_IPad2;
            }
        }
    }
    
    return deviceType;
}

BE_NAMESPACE_END

