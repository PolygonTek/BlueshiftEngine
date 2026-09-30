// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "Containers/HashMap.h"
#include "Font.h"

/*
---------------------------------------------------------------------------------------------------

    Font Manager

---------------------------------------------------------------------------------------------------
*/

class FontManager {
public:
    void                    Init();
    void                    Shutdown();

    void                    UpdateFrame();

    SharedPtr<Font>         AllocFont(const char *name, uint32_t fontSize);
    SharedPtr<Font>         FindFont(const char *name, uint32_t fontSize) const;
    SharedPtr<Font>         GetFont(const char *name, uint32_t fontSize);

    SharedPtr<Font>         defaultFont;

private:
    void                    CleanExpiredFonts();

    struct FontHashKey {
        FontHashKey() : fontSize(0) {}
        FontHashKey(const BE1::Str &name, uint32_t fontSize) : name(name), fontSize(fontSize) {}

        BE1::Str            name;
        uint32_t            fontSize;
    };

    struct FontHashCompare {
        template <typename Type1, typename Type2>
        static bool Compare(const Type1 &lhs, const Type2 &rhs) {
            return (lhs.fontSize == rhs.fontSize && BE1::Str::Icmp(lhs.name, rhs.name) == 0);
        }
    };

    struct FontHashGenerator {
        template <typename Type>
        static int Hash(const BE1::HashIndex &hasher, const Type &value) {
            return hasher.GenerateHash(value.name, false) * value.fontSize;
        }
    };

    using FontHashMap       = BE1::HashMap<FontHashKey, WeakPtr<Font>, FontHashCompare, FontHashGenerator>;
    FontHashMap             fontHashMap;
    int                     expiredFontHashMapCursor = 0;
};

extern FontManager          fontManager;
