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
    struct Type {
        enum Enum {
            IPodTouch4,         // (960 x 640)
            IPodTouch5,         // (1136 x 640)
            IPodTouch6,

            IPhone4,            // (960 x 640)
            IPhone4S,
            IPhone5,            // also the iPhone5c (1136 x 640)
            IPhone5S,
            IPhone6,            // (1334 x 750)
            IPhone6Plus,        // (1920 x 1080)
            IPhone6S,
            IPhone6SPlus,
            IPhoneSE,
            IPhone7,
            IPhone7Plus,
            IPhone8,
            IPhone8Plus,
            IPhoneX,            // (2436 x 1125)

            IPhoneXR,           // (1792 x 828)
            IPhoneXS,           // (2436 x 1125)
            IPhoneXSMax,        // (2688 x 1242)

            IPhone11,           // (1792 x 828)
            IPhone11Pro,        // (2436 x 1125)
            IPhone11ProMax,     // (2688 x 1242)

            IPad2,              // (1024 x 768)
            IPadMini,
            IPad3,
            IPad4,
            IPadAir,            // also the IPad Mini Retina (2048 x 1536)
            IPadMini2,
            IPadMini3,
            IPadAir2,
            IPadMini4,
            IPadPro_12_9,       // iPad Pro 12.9 inch (2732 x 2048)
            IPadPro_9_7,        // iPad Pro 9.7 inch
            IPad5,
            IPadPro2_12_9,
            IPadPro2_10_5,      // iPad Pro 10.5 inch (2224 x 1668)
            IPad6,
            IPadPro3_12_9,      // iPad Pro 12.9 inch (2732 x 2048)
            IPadPro3_11,        // iPad Pro 11 inch (2388 x 1668)
            IPadMini5,
            IPadAir3,
            UnknownDevice = -1
        };
    };

    static bool IsIPhone(Type::Enum deviceType);
    static bool IsIPod(Type::Enum deviceType);
    static bool IsIPad(Type::Enum deviceType);
    
    static void GetDeviceResolution(Type::Enum deviceType, int &width, int &height);
   
    static Type::Enum GetIOSDeviceType();
};

BE_NAMESPACE_END
