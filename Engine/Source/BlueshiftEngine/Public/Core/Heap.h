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

BE_NAMESPACE_BEGIN

//#define DEBUG_MEMORY

#ifndef DEBUG_MEMORY

void * BE_API       Mem_Alloc(size_t size);
void * BE_API       Mem_ClearedAlloc(size_t size);
void * BE_API       Mem_Alloc16(size_t size);
void * BE_API       Mem_Alloc32(size_t size);
char * BE_API       Mem_AllocString(const char *str);
wchar_t * BE_API    Mem_AllocWideString(const wchar_t *str);
void BE_API         Mem_Free(void *ptr);
void BE_API         Mem_AlignedFree(void *ptr);

#else // DEBUG_MEMORY

void * BE_API       DebugMem_Alloc(size_t size, const char *filename, const int lineNumber);
void * BE_API       DebugMem_ClearedAlloc(size_t size, const char *filename, const int lineNumber);
void * BE_API       DebugMem_Alloc16(size_t size, const char *filename, const int lineNumber);
void * BE_API       DebugMem_Alloc32(size_t size, const char *filename, const int lineNumber);
char * BE_API       DebugMem_AllocString(const char *str, const char *filename, const int lineNumber);
wchar_t * BE_API    DebugMem_AllocWideString(const wchar_t *str, const char *filename, const int lineNumber);
void BE_API         DebugMem_Free(void *ptr);
void BE_API         DebugMem_AlignedFree(void *ptr);

#define Mem_Alloc(size)         DebugMem_Alloc(size, __FILE__, __LINE__)
#define Mem_ClearedAlloc(size)  DebugMem_ClearedAlloc(size, __FILE__, __LINE__)
#define Mem_Alloc16(size)       DebugMem_Alloc16(size, __FILE__, __LINE__)
#define Mem_Alloc32(size)       DebugMem_Alloc32(size, __FILE__, __LINE__)
#define Mem_AllocString(s)      DebugMem_AllocString(s, __FILE__, __LINE__)
#define Mem_AllocWideString(s)  DebugMem_AllocWideString(s, __FILE__, __LINE__)
#define Mem_Free(ptr)           DebugMem_Free(ptr)
#define Mem_AlignedFree(ptr)    DebugMem_AlignedFree(ptr)

#endif // DEBUG_MEMORY

BE_NAMESPACE_END
