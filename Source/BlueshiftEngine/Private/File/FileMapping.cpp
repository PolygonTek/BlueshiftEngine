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
#include "Core/Str.h"
#include "Core/WStr.h"
#include "Platform/PlatformFile.h"
#include "Platform/PlatformProcess.h"
#include "File/FileMapping.h"

#ifdef __UNIX__
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#endif

#ifdef __ANDROID__
#include "PlatformUtils/Android/AndroidJNI.h"
#include <android/asset_manager.h>
#endif

BE_NAMESPACE_BEGIN

#ifdef __ANDROID__
static off_t g_nPageMask;
#endif

void _FileMapping::Touch() {
#if defined(__UNIX__)
    size_t page_size = (size_t)sysconf(_SC_PAGESIZE);
#elif defined(__WIN32__)
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    size_t page_size = sysinfo.dwPageSize;
#endif
    uint32_t checksum = 0;
    for (byte *ptr = (byte *)data; ptr < (byte *)data + size; ptr += page_size) {
        checksum += *(uint32_t *)ptr;
    }
}

bool FileMapping::Open(const TCHAR *filename) {
#if defined __ANDROID__
    g_nPageMask = getpagesize() - 1;
    off_t nStart;
    hFile = open(filename, O_RDONLY);
    if (hFile >= 0) {
        struct stat fs;
        fstat(hFile, &fs);
        size = fs.st_size;
        nStart = 0;
    } else {
        AAsset* asset = AAssetManager_open(AndroidJNI::activity->assetManager, filename, AASSET_MODE_UNKNOWN);
        if (asset) {
            off_t nLength;
            hFile = AAsset_openFileDescriptor(asset, &nStart, &nLength);
            assert(hFile > 0);
            size = nLength;
            AAsset_close(asset);
        }
    }
    off_t nDelta = nStart & g_nPageMask;
    void *map = mmap(NULL, size + nDelta, PROT_READ, MAP_FILE | MAP_SHARED, hFile, nStart - nDelta);
    if (map == NULL) {
        BE_ERRLOG(L"_FileMapping::Open: Could not map %ls to memory\n", towcs(filename));
        assert(0);
        close(hFile);
        hFile = -1;
        return false;
    }
    assert(((size_t)map & g_nPageMask) == 0);
    data = (char *) map + nDelta;
#elif defined(__UNIX__)
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        BE_ERRLOG(L"_FileMapping::Open: Could not open %ls\n", towcs(filename));
        return false;
    }
    struct stat fs;
    fstat(fd, &fs);
    size = fs.st_size;

    data = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    if (data == NULL) {
        BE_ERRLOG(L"_FileMapping::Open: Could not map %ls to memory\n", towcs(filename));
        return false;
    }

    // close fd is error 
    //if (close(fd) != 0) {
    //	BE_ERRLOG(L"_FileMapping::Open: unable to close file\n");
    //	return false;
    //}

#elif defined(__WIN32__)
    hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        BE_ERRLOG(L"_FileMapping::Open: Could not open %ls: %ls\n", towcs(filename), PlatformProcess::GetLastErrorText().c_str());
        return false;
    }
    size = GetFileSize(hFile, 0);

    hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!hMapping) {
        BE_ERRLOG(L"_FileMapping::Open: Create_FileMapping %ls: %ls\n", towcs(filename), PlatformProcess::GetLastErrorText().c_str());
        return false;
    }

    data = (void *)MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
    if (!data) {
        BE_ERRLOG(L"_FileMapping::Open: Could not map %ls to memory: %ls\n", towcs(filename), PlatformProcess::GetLastErrorText().c_str());
        CloseHandle(hMapping);
        CloseHandle(hFile);
        hMapping = NULL;
        hFile = INVALID_HANDLE_VALUE;
        return false;
    }
#endif

    return true;
}

FileMapping::FileMapping() {
#ifdef __WIN32__
    hFile = INVALID_HANDLE_VALUE;
    hMapping = NULL;
#elif defined __UNIX__
    hFile = -1;
#endif
}

bool FileMapping::Open(const Str &filename) {
    this->filename = filename;
    return Open(totcs(filename.c_str()));
}

void FileMapping::Close() {
    if (!data) {
        return;
    }
#ifdef __ANDROID__
    data = (void *)((off_t)data & ~g_nPageMask);
#endif

#if defined(__UNIX__)
    int retval = munmap((void *) data, size);
    if (retval != 0) {
        BE_ERRLOG(L"_FileMapping::Close: unable to unmap memory\n");
    }
    close(hFile);
#elif defined(__WIN32__)
    UnmapViewOfFile(data);
    CloseHandle(hMapping);
    CloseHandle(hFile);
#endif
}

BE_NAMESPACE_END
