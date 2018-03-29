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

#define USE_BASE_PLATFORM_POSIX_FILE
#include "../Posix/PlatformPosixFile.h"

BE_NAMESPACE_BEGIN

class BE_API PlatformIOSFile : public PlatformPosixFile {
public:
    PlatformIOSFile(FILE *fp);
    virtual ~PlatformIOSFile();   
    
    static PlatformIOSFile *OpenFileRead(const char *filename);
    static PlatformIOSFile *OpenFileWrite(const char *filename);
    static PlatformIOSFile *OpenFileAppend(const char *filename);
    
    static bool             FileExists(const char *filename);
    static size_t           FileSize(const char *filename);
    static bool             IsFileWritable(const char *filename);
    static bool             IsReadOnly(const char *filename);
    static bool             RemoveFile(const char *filename);
    static bool             MoveFile(const char *srcFilename, const char *dstFilename);
    
    static bool             DirectoryExists(const char *dirname);
    static bool             CreateDirectory(const char *dirname);
    static bool             RemoveDirectory(const char *dirname);

    static const char *     ExecutablePath();

    static const char *     UserDir();
    static const char *     UserDocumentDir();
    static const char *     UserAppDataDir();
    static const char *     UserTempDir();
    
    static Str              ConvertToIOSPath(const Str &filename, bool forWrite);

protected:
    static Str              NormalizeFilename(const char *filename);
    static Str              NormalizeDirectoryName(const char *dirname);
};

typedef PlatformIOSFile     PlatformFile;

BE_NAMESPACE_END
