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

/*
-------------------------------------------------------------------------------

    Heap memory management

-------------------------------------------------------------------------------
*/

#define Mem_Alloc(size)                     malloc(size)
#define Mem_ClearedAlloc(size)              calloc(size, 1)
#define Mem_AllocString(str)                strdup(str)
#define Mem_Free(ptr)                       free(ptr)
#ifdef __WIN32__
#define Mem_AlignedAlloc(size, alignment)   _aligned_malloc(size, alignment)
#define Mem_Alloc16(size)                   _aligned_malloc(size, 16)
#define Mem_Alloc32(size)                   _aligned_malloc(size, 32)
#define Mem_AlignedFree(ptr)                _aligned_free(ptr)
#else
#define Mem_AlignedAlloc(size, alignment)   std::aligned_alloc(alignment, size)
#define Mem_Alloc16(size)                   std::aligned_alloc(16, size)
#define Mem_Alloc32(size)                   std::aligned_alloc(32, size)
#define Mem_AlignedFree(ptr)                free(ptr)
#endif

// Not used yet..
#define BE_OVERRIDE_NEW_DELETE \
    BE_INLINE void *operator new (size_t inCount) { return Mem_Alloc(inCount); } \
    BE_INLINE void operator delete (void *inPointer) noexcept { Mem_Free(inPointer); } \
    BE_INLINE void *operator new[] (size_t inCount) { return Mem_Alloc(inCount); } \
    BE_INLINE void operator delete[] (void *inPointer) noexcept { Mem_Free(inPointer); } \
    BE_INLINE void *operator new (size_t inCount, std::align_val_t inAlignment) { return Mem_AlignedAlloc(inCount, static_cast<size_t>(inAlignment)); } \
    BE_INLINE void operator delete (void *inPointer, std::align_val_t inAlignment) noexcept { Mem_AlignedFree(inPointer); } \
    BE_INLINE void *operator new[] (size_t inCount, std::align_val_t inAlignment) { return Mem_AlignedAlloc(inCount, static_cast<size_t>(inAlignment)); } \
    BE_INLINE void operator delete[] (void *inPointer, std::align_val_t inAlignment) noexcept { Mem_AlignedFree(inPointer); }

BE_NAMESPACE_BEGIN
BE_NAMESPACE_END
