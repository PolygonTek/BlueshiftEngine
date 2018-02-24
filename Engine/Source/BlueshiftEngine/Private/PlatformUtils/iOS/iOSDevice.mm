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

bool IOSDevice::IsIPhone(IOSDevice::Type deviceType) {
    return deviceType >= IOS_IPhone4 && deviceType <= IOS_IPhoneX;
}

bool IOSDevice::IsIPod(IOSDevice::Type deviceType) {
    return deviceType >= IOS_IPodTouch4 && deviceType <= IOS_IPodTouch6;
}

bool IOSDevice::IsIPad(IOSDevice::Type deviceType) {
    return deviceType >= IOS_IPad2 && deviceType <= IOS_IPadPro2_10_5;
}

void IOSDevice::GetDeviceResolution(IOSDevice::Type deviceType, int &width, int &height) {
    switch (deviceType) {
        case IOS_IPhone4:
        case IOS_IPhone4S:
        case IOS_IPodTouch4:
            width = 960;
            height = 640;
            break;
        case IOS_IPhone5:
        case IOS_IPhone5S:
        case IOS_IPhoneSE:
        case IOS_IPodTouch5:
        case IOS_IPodTouch6:
            width = 1136;
            height = 640;
            break;
        case IOS_IPhone6:
        case IOS_IPhone6S:
        case IOS_IPhone7:
        case IOS_IPhone8:
            width = 1334;
            height = 750;
            break;
        case IOS_IPhone6Plus:
        case IOS_IPhone6SPlus:
        case IOS_IPhone7Plus:
        case IOS_IPhone8Plus:
            width = 1920;
            height = 1080;
            break;
        case IOS_IPhoneX:
            width = 2436;
            height = 1125;
            break;
        case IOS_IPad2:
        case IOS_IPadMini:
            width = 1024;
            height = 768;
            break;
        case IOS_IPad3:
        case IOS_IPad4:
        case IOS_IPadMini2:
        case IOS_IPadMini3:
        case IOS_IPadMini4:
        case IOS_IPadAir:
        case IOS_IPadAir2:
        case IOS_IPadPro_9_7:
            width = 2048;
            height = 1536;
            break;
        case IOS_IPadPro_12_9:
        case IOS_IPadPro2_12_9:
            width = 2732;
            height = 2048;
            break;
        case IOS_IPadPro2_10_5:
            width = 2224;
            height = 1668;
            break;
        default:
            break;
    }
}

IOSDevice::Type IOSDevice::GetIOSDeviceType() {
    // default to unknown
    static IOSDevice::Type deviceType = IOSDevice::Type::IOS_Unknown;
    
    // if we've already figured it out, return it
    if (deviceType != IOSDevice::Type::IOS_Unknown) {
        return deviceType;
    }
    
    // get the device hardware type string length
    size_t deviceIDLen;
    sysctlbyname("hw.machine", NULL, &deviceIDLen, NULL, 0);
    
    // get the device hardware type
    char *deviceID = (char *)malloc(deviceIDLen);
    sysctlbyname("hw.machine", deviceID, &deviceIDLen, NULL, 0);
    
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
        } else if (major >= 10) {
            // for going forward into unknown devices (like 8/8+?), we can't use minor,
            // so treat devices with a scale > 2.5 to be 6SPlus type devices, < 2.5 to be 6S type devices
            if ([UIScreen mainScreen].scale > 2.5f) {
                deviceType = IOS_IPhone8Plus;
            } else {
                deviceType = IOS_IPhone8;
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

