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

class IOSDevice {
public:
    enum Type {
        IOS_IPhone4,
        IOS_IPhone4S,
        IOS_IPhone5, // also the iPhone5c
        IOS_IPhone5S,
        IOS_IPhone6,
        IOS_IPhone6Plus,
        IOS_IPhone6S,
        IOS_IPhone6SPlus,
        IOS_IPhoneSE,
        IOS_IPhone7,
        IOS_IPhone7Plus,
        IOS_IPhone8,
        IOS_IPhone8Plus,
        IOS_IPhoneX,
        IOS_IPodTouch4,
        IOS_IPodTouch5,
        IOS_IPodTouch6,
        IOS_IPad2,
        IOS_IPadMini,
        IOS_IPad3,
        IOS_IPad4,
        IOS_IPadAir, // also the IPad Mini Retina
        IOS_IPadMini2,
        IOS_IPadMini3,
        IOS_IPadAir2,
        IOS_IPadMini4,
        IOS_IPadPro_9_7, // iPad Pro 9.7 inch
        IOS_IPadPro_12_9, // iPad Pro 12.9 inch
        IOS_IPadPro2_12_9,
        IOS_IPadPro2_10_5,
        IOS_Unknown,
    };

    static bool IsIPhone(Type deviceType);
    static bool IsIPod(Type deviceType);
    static bool IsIPad(Type deviceType);
   
    static Type GetIOSDeviceType();
};

