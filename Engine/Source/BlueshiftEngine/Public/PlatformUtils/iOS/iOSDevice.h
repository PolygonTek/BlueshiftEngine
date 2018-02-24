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

#pragma once

BE_NAMESPACE_BEGIN

class IOSDevice {
public:
    enum Type {
        IOS_IPhone4,        // (960x640)
        IOS_IPhone4S,
        IOS_IPhone5,        // also the iPhone5c (1136x640)
        IOS_IPhone5S,
        IOS_IPhone6,        // (1334x750)
        IOS_IPhone6Plus,    // (1920x1080)
        IOS_IPhone6S,
        IOS_IPhone6SPlus,
        IOS_IPhoneSE,
        IOS_IPhone7,
        IOS_IPhone7Plus,
        IOS_IPhone8,
        IOS_IPhone8Plus,
        IOS_IPhoneX,        // (2436x1125)
        
        IOS_IPodTouch4,     // (960x640)
        IOS_IPodTouch5,     // (1136x640)
        IOS_IPodTouch6,
        
        IOS_IPad2,          // (1024x768)
        IOS_IPadMini,
        IOS_IPad3,
        IOS_IPad4,
        IOS_IPadAir,        // also the IPad Mini Retina (2048x1536)
        IOS_IPadMini2,
        IOS_IPadMini3,
        IOS_IPadAir2,
        IOS_IPadMini4,

        IOS_IPadPro_9_7,    // iPad Pro 9.7 inch
        IOS_IPadPro_12_9,   // iPad Pro 12.9 inch (2732x2048)
        IOS_IPadPro2_12_9,
        IOS_IPadPro2_10_5,  // iPad Pro 10.5 inch (2224x1668)
        IOS_Unknown,
    };

    static bool IsIPhone(Type deviceType);
    static bool IsIPod(Type deviceType);
    static bool IsIPad(Type deviceType);
    
    static void GetDeviceResolution(Type deviceType, int &width, int &height);
   
    static Type GetIOSDeviceType();
};

BE_NAMESPACE_END
