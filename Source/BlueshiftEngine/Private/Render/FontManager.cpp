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
#include "Render/Render.h"
#include "Core/StrColor.h"
#include "Render/Font.h"
#include "FontFace.h"

BE_NAMESPACE_BEGIN

const char *    FontManager::defaultFontFilename = "Data/EngineFonts/consola.ttf";//"Data/EngineFonts/Lucida Console12.font"
Font *          FontManager::defaultFont;
FontManager     fontManager;

void FontManager::Init() {
    FontFaceFreeType::Init();

    fontHashMap.Init(1024, 64, 64);

    defaultFont = AllocFont(defaultFontFilename, 14);
    if (!defaultFont->Load(defaultFontFilename)) {
        BE_FATALERROR(L"Couldn't load default font!");
    }
    defaultFont->permanence = true;
}

void FontManager::Shutdown() {
    fontHashMap.DeleteContents(true);

    FontFaceFreeType::Shutdown();
}

Font *FontManager::AllocFont(const char *hashName, int fontSize) {
    if (fontHashMap.Get(FontHashKey(hashName, fontSize))) {
        BE_FATALERROR(L"%hs font already allocated", hashName);
    }

    Font *font = new Font;
    font->hashName = hashName;
    font->name = hashName;
    font->name.StripPath();
    font->name.StripFileExtension();
    font->refCount = 1;
    font->fontSize = fontSize;
    fontHashMap.Set(FontHashKey(hashName, fontSize), font);

    return font;
}

void FontManager::DestroyFont(Font *font) {
    if (font->refCount > 1) {
        BE_WARNLOG(L"FontManager::DestroyFont: font '%hs' has %i reference count\n", font->name.c_str(), font->refCount);
    }

    fontHashMap.Remove(FontHashKey(font->hashName, font->fontSize));
    delete font;
}

void FontManager::ReleaseFont(Font *font, bool immediateDestroy) {
    if (font->permanence) {
        return;
    }

    // 레퍼런스 카운터가 0 인 font 만 제거한다
    if (font->refCount > 0) {
        font->refCount--;
    }

    if (immediateDestroy && font->refCount == 0) {
        DestroyFont(font);
    }
}

void FontManager::DestroyUnusedFonts() {
    Array<Font *> removeArray;

    for (int i = 0; i < fontHashMap.Count(); i++) {
        const auto *entry = fontHashMap.GetByIndex(i);
        Font *font = entry->second;

        if (font && !font->permanence && font->refCount == 0) {
            removeArray.Append(font);
        }
    }

    for (int i = 0; i < removeArray.Count(); i++) {
        DestroyFont(removeArray[i]);
    }
}

Font *FontManager::FindFont(const char *hashName, int fontSize) const {
    const auto *entry = fontHashMap.Get(FontHashKey(hashName, fontSize));
    if (entry) {
        return entry->second;
    }

    return nullptr;
}

Font *FontManager::GetFont(const char *hashName, int fontSize) {
    if (!hashName || !hashName[0]) {
        return defaultFont;
    }

    Font *font = FindFont(hashName, fontSize);
    if (font) {
        font->refCount++;
        return font;
    }

    font = AllocFont(hashName, fontSize);
    if (!font->Load(hashName)) {
        DestroyFont(font);
        return defaultFont;
    }

    return font;
}
    
BE_NAMESPACE_END
