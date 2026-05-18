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

#include "Precompiled.h"
#include "Font.h"
#include "TrueTypeFont.h"
#include "FontFace.h"

static const char * defaultFontFilename = "Data/EngineFonts/consola.ttf";//"Data/EngineFonts/Lucida Console12.font"
FontManager         fontManager;

void FontManager::Init() {
    TrueTypeFont::Init();
    TrueTypeFontFace::InitAtlas();

    fontHashMap.Init(1024, 64, 64);

    expiredFontHashMapCursor = 0;

    defaultFont = AllocFont(defaultFontFilename, 18);
    if (!defaultFont->Load(defaultFontFilename)) {
        BE_FATALERROR("Couldn't load default font!");
    }
}

void FontManager::Shutdown() {
    if (defaultFont) {
        defaultFont.reset();
    }

    fontHashMap.Clear();

    TrueTypeFontFace::FreeAtlas();

    TrueTypeFont::Shutdown();
}

void FontManager::UpdateFrame() {
    CleanExpiredFonts();
}

std::shared_ptr<Font> FontManager::AllocFont(const char *hashName, uint32_t fontSize) {
    if (!hashName || !hashName[0]) {
        BE_FATALERROR("FontManager::AllocFont: empty font name");
    }
    if (const auto *entry = fontHashMap.Get(FontHashKey(hashName, fontSize))) {
        if (!entry->second.expired()) {
            BE_FATALERROR("FontManager::AllocFont: %s (size = %i) font already allocated", hashName, fontSize);
        }
        fontHashMap.Remove(FontHashKey(hashName, fontSize));
    }

    std::shared_ptr<Font> font = std::make_shared<Font>();
    font->hashName = hashName;
    font->name = hashName;
    font->name.StripPath();
    font->name.StripFileExtension();
    font->fontSize = fontSize;
    fontHashMap.Set(FontHashKey(hashName, fontSize), font);

    return font;
}

std::shared_ptr<Font> FontManager::FindFont(const char *hashName, uint32_t fontSize) const {
    if (!hashName || !hashName[0]) {
        return nullptr;
    }

    const auto *entry = fontHashMap.Get(FontHashKey(hashName, fontSize));
    if (!entry) {
        return nullptr;
    }

    return entry->second.lock();
}

std::shared_ptr<Font> FontManager::GetFont(const char *hashName, uint32_t fontSize) {
    if (!hashName || !hashName[0]) {
        return defaultFont;
    }

    std::shared_ptr<Font> font = FindFont(hashName, fontSize);
    if (font) {
        return font;
    }

    font = AllocFont(hashName, fontSize);
    if (!font->Load(hashName)) {
        fontHashMap.Remove(FontHashKey(font->hashName, fontSize));
        return defaultFont;
    }
    return font;
}

void FontManager::CleanExpiredFonts() {
    constexpr int ExpiredFontHashMapScanBudget = 1;
    int scansLeft = ExpiredFontHashMapScanBudget;

    while (scansLeft-- > 0 && fontHashMap.Count() > 0) {
        if (expiredFontHashMapCursor >= fontHashMap.Count()) {
            expiredFontHashMapCursor = 0;
        }

        const auto *entry = fontHashMap.GetByIndex(expiredFontHashMapCursor);
        assert(entry);

        if (entry->second.expired()) {
            fontHashMap.Remove(entry->first);
            continue;
        }

        expiredFontHashMapCursor++;
    }
}
