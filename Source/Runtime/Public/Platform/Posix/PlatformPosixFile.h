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

// This class uses ANSI-C fopen rather than POSIX open.
class BE_API PlatformPosixFile : public PlatformBaseFile {
    friend class PlatformPosixFileMapping;
    
public:
    PlatformPosixFile() {}
    PlatformPosixFile(FILE *fp);
    virtual ~PlatformPosixFile();
                                // Return offset in file.
    virtual int                 Tell() const;
    virtual int                 Size() const;
                                // Seek from the start on a file.
    virtual int                 Seek(long offset, Origin::Enum origin);
    
                                // Read data from the file to the buffer.
    virtual size_t              Read(void *buffer, size_t bytesToRead) const;
                                // Write data from the buffer to the file.
    virtual bool                Write(const void *buffer, size_t bytesToWrite);
    
    static PlatformBaseFile *   OpenFileRead(const char *filename);
    static PlatformBaseFile *   OpenFileWrite(const char *filename);
    static PlatformBaseFile *   OpenFileAppend(const char *filename);

    static bool                 FileExists(const char *filename);
    static size_t               FileSize(const char *filename);
    static bool                 IsFileWritable(const char *filename);
    static bool                 IsReadOnly(const char *filename);
    static bool                 SetReadOnly(const char *filename, bool readOnly);
    static bool                 RemoveFile(const char *filename);
    static bool                 MoveFile(const char *srcFilename, const char *dstFilename);
    static int                  GetFileMode(const char *filename);
    static void                 SetFileMode(const char *filename, int mode);
    
    static DateTime             GetTimeStamp(const char *filename);
    static void                 SetTimeStamp(const char *filename, const DateTime &timeStamp);
    
    static bool                 DirectoryExists(const char *dirname);
    static bool                 CreateDirectory(const char *dirname);
    static bool                 RemoveDirectory(const char *dirname);
    static bool                 RemoveDirectoryTree(const char *dirname);

    static const char *         Cwd();
    static bool                 SetCwd(const char *dirname);
    static const char *         ExecutablePath();
    
    static int                  ListFiles(const char *directory, const char *nameFilter, bool recursive, bool includeSubDir, Array<FileInfo> &files);
    
protected:
    static Str                  NormalizeFilename(const char *filename);
    static Str                  NormalizeDirectoryName(const char *dirname);

    FILE *                      fp;
};

class BE_API PlatformPosixFileMapping : public PlatformBaseFileMapping {
public:
    PlatformPosixFileMapping(int fileHandle, size_t size, const void *data);
    virtual ~PlatformPosixFileMapping();

    virtual void                Touch();

    static PlatformPosixFileMapping *OpenFileRead(const char *filename);
    static PlatformPosixFileMapping *OpenFileReadWrite(const char *filename, int newSize = 0);

protected:
    int                         fileHandle = -1;
};

#ifndef USE_BASE_PLATFORM_POSIX_FILE
typedef PlatformPosixFile       PlatformFile;
typedef PlatformPosixFileMapping PlatformFileMapping;
#endif

BE_NAMESPACE_END
