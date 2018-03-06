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

#include "Core/Str.h"

BE_NAMESPACE_BEGIN

class _FileMapping {
public:
    _FileMapping() {
        size = 0;
        data = NULL;
    }

    void            Open(const void *_data, size_t _size) {
        data = _data;
        size = _size;
    }
    void            Touch();

    const char *    GetData() const { return (const char *)data; }
    size_t          GetSize() const { return size; }

protected:
    size_t          size;
    const void *    data;
};

class FileMapping : public _FileMapping {
public:
    FileMapping();

    FileMapping &   operator=(const FileMapping &value);

    bool            Open(const TCHAR *filename);
    bool            Open(const Str &filename);
    void            Close();

private:
    Str             filename;
#if defined(__WIN32__)
    HANDLE          hFile;
    HANDLE          hMapping;
#elif defined __UNIX__
    int             hFile;
#endif
};

BE_INLINE FileMapping & FileMapping::operator=(const FileMapping &value) {
//	*(_FileMapping *) this = (_FileMapping &)value;
#ifdef __WIN32__
    hFile = value.hFile;
    hMapping = value.hMapping;
#elif defined __UNIX__
    hFile = value.hFile;
#endif
    return *this;
}

BE_NAMESPACE_END
