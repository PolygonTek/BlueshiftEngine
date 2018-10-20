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

class PlatformWinUtils {
public:
                    // Converts string from UCS2 to UTF8.
                    // Returns number of characters in UTF8 string.
    static int      UCS2ToUTF8(const wchar_t *src, char *dest, int destLen);

                    // Converts string from UTF8 to UCS2.
                    // Returns number of characters in UCS2 string.
    static int      UTF8ToUCS2(const char *src, wchar_t *dest, int destLen);
};

BE_NAMESPACE_END
