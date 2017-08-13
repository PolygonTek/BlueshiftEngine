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
#include "Core/Heap.h"

#ifdef __APPLE__
#include "malloc/malloc.h"
#endif

BE_NAMESPACE_BEGIN

//#define SJPARK

#ifndef DEBUG_MEMORY

void *Mem_Alloc(size_t size) {
#ifdef SJPARK
    void *ptr =  malloc(size);
    memset(ptr, 0xfc, size);
    return ptr;
#else
    return malloc(size);
#endif
}

void *Mem_ClearedAlloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        return nullptr;
    }
    memset(ptr, 0, size);
    return ptr;
}

void *Mem_Alloc16(size_t size) {
    byte *ptr = (byte *)Mem_Alloc(size + sizeof(intptr_t) + 15);
    if (!ptr) {
        return nullptr;
    }
    byte *alignedPtr = (byte *)((((intptr_t)ptr) + sizeof(intptr_t) + 15) & ~15);
    *((intptr_t *)(alignedPtr - sizeof(intptr_t))) = (intptr_t)ptr;
    return alignedPtr;
}

void *Mem_Alloc32(size_t size) {
    byte *ptr = (byte *)Mem_Alloc(size + sizeof(intptr_t) + 31);
    if (!ptr) {
        return nullptr;
    }
    byte *alignedPtr = (byte *)((((intptr_t)ptr) + sizeof(intptr_t) + 31) & ~31);
    *((intptr_t *)(alignedPtr - sizeof(intptr_t))) = (intptr_t)ptr;
    return alignedPtr;
}

char *Mem_AllocString(const char *str) {
    char *ptr = (char *)Mem_Alloc(strlen(str) + 1);
    if (!ptr) {
        return nullptr;
    }
    strcpy(ptr, str);
    return ptr;
}

wchar_t *Mem_AllocWideString(const wchar_t *str) {
    wchar_t *ptr = (wchar_t *)Mem_Alloc((wcslen(str) + 1) * sizeof(wchar_t));
    if (!ptr) {
        return nullptr;
    }
    wcscpy(ptr, str);
    return ptr;
}

void Mem_Free(void *ptr) {
    if (ptr) {
#ifdef SJPARK
    #ifdef __WIN32__
        int size = _msize(ptr);
        memset(ptr, 0xfd, size);
    #elif defined __APPLE__
        int size = malloc_size(ptr);
        memset(ptr, 0xfd, size);
    #endif
#endif
        free(ptr);
    }
}

void Mem_AlignedFree(void *ptr) {
    if (ptr) {
        Mem_Free((void *)*((intptr_t *)(((byte *)ptr) - sizeof(intptr_t))));
    }
}

#else

void *DebugMem_Alloc(size_t size, const char *filename, const int lineNumber) {
#ifdef __WIN32__
    return _malloc_dbg(size, _NORMAL_BLOCK, filename, lineNumber);
#else
    return malloc(size);
#endif
}

void *DebugMem_ClearedAlloc(size_t size, const char *filename, const int lineNumber) {
#ifdef __WIN32__
    void *r = _malloc_dbg(size, _NORMAL_BLOCK, filename, lineNumber);
#else
    void *r = malloc(size);
#endif
    memset(r, 0, size);
    return r;
}

void *DebugMem_Alloc16(size_t size, const char *filename, const int lineNumber) {
    byte *ptr = (byte *)DebugMem_Alloc(size + sizeof(intptr_t) + 15, filename, lineNumber);
    if (!ptr) {
        return nullptr;
    }
    byte *alignedPtr = (byte *)((((intptr_t)ptr) + sizeof(intptr_t) + 15) & ~15);
    *((intptr_t *)(alignedPtr - sizeof(intptr_t))) = (intptr_t)ptr;
    return alignedPtr;
}

void *DebugMem_Alloc32(size_t size, const char *filename, const int lineNumber) {
    byte *ptr = (byte *)DebugMem_Alloc(size + sizeof(intptr_t) + 31, filename, lineNumber);
    if (!ptr) {
        return nullptr;
    }
    byte *alignedPtr = (byte *)((((intptr_t)ptr) + sizeof(intptr_t) + 31) & ~31);
    *((intptr_t *)(alignedPtr - sizeof(intptr_t))) = (intptr_t)ptr;
    return alignedPtr;
}

char *DebugMem_AllocString(const char *str, const char *filename, const int lineNumber) {
    char *ptr = (char *)DebugMem_Alloc(strlen(str) + 1, filename, lineNumber);
    if (!ptr) {
        return nullptr;
    }
    strcpy(ptr, str);
    return ptr;
}

wchar_t *DebugMem_AllocWideString(const wchar_t *str, const char *filename, const int lineNumber) {
    wchar_t *ptr = (wchar_t *)DebugMem_Alloc(sizeof(wchar_t) * (wcslen(str) + 1), filename, lineNumber);
    if (!ptr) {
        return nullptr;
    }
    wcscpy(ptr, str);
    return ptr;
}

void DebugMem_Free(void *ptr) {
#ifdef __WIN32__
    _free_dbg(ptr, _NORMAL_BLOCK);
#else
    free(ptr);
#endif
}

void DebugMem_AlignedFree(void *ptr) {
    if (ptr) {
        DebugMem_Free((void *)*((intptr_t *)(((byte *)ptr) - sizeof(intptr_t))));
    }
}

#endif // DEBUG_MEMORY

BE_NAMESPACE_END
