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

struct AAsset;

BE_NAMESPACE_BEGIN

class BE_API PlatformAndroidFile : public PlatformBaseFile {
public:
    PlatformAndroidFile(FILE *fp);
    PlatformAndroidFile(AAsset *fp);

    virtual ~PlatformAndroidFile();

                            /// Returns offset in file.
    virtual int             Tell() const;
                            /// Returns file size.
    virtual int             Size() const;
                            /// Seek from the start on a file.
    virtual int             Seek(long offset, Origin origin);

                            // Reads data from the file to the buffer.
    virtual size_t          Read(void *buffer, size_t bytesToRead) const;
                            // Writes data from the buffer to the file.
    virtual bool            Write(const void *buffer, size_t bytesToWrite);

    static PlatformAndroidFile *OpenFileRead(const char *filename);
    static PlatformAndroidFile *OpenFileWrite(const char *filename);
    static PlatformAndroidFile *OpenFileAppend(const char *filename);

    static bool             FileExists(const char *filename);
    static size_t           FileSize(const char *filename);
    static bool             IsFileWritable(const char *filename);
    static bool             IsReadOnly(const char *filename);
    static bool             SetReadOnly(const char *filename, bool readOnly);
    static bool             RemoveFile(const char *filename);
    static bool             MoveFile(const char *srcFilename, const char *dstFilename);
    static int              GetFileMode(const char *filename);
    static void             SetFileMode(const char *filename, int mode);

    static DateTime         GetTimeStamp(const char *filename);
    static void             SetTimeStamp(const char *filename, const DateTime &timeStamp);

    static bool             DirectoryExists(const char *dirname);
    static bool             CreateDirectory(const char *dirname);
    static bool             RemoveDirectory(const char *dirname);
    static bool             RemoveDirectoryTree(const char *dirname);

    static const char *     Cwd();
    static bool             SetCwd(const char *dirname);
    
    static const char *     ExecutablePath();

    static int              ListFiles(const char *directory, const char *nameFilter, bool recursive, bool includeSubDir, Array<FileInfo> &files);

protected:
    static Str              NormalizeFilename(const char *filename);
    static Str              NormalizeDirectoryName(const char *dirname);

    static void             ListFilesRecursive(const char *directory, const char *subdir, const char *nameFilter, bool includeSubDir, Array<FileInfo> &files);

    FILE *                  fp;
    AAsset *                asset;
};

typedef PlatformAndroidFile PlatformFile;

BE_NAMESPACE_END