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

class AAsset;
class AAssetManager;

BE_NAMESPACE_BEGIN

class BE_API _PlatformAndroidFile : public PlatformBaseFile {
public:

    _PlatformAndroidFile() {}
    _PlatformAndroidFile(FILE *fp);
    _PlatformAndroidFile(AAsset *fp);
    virtual                 ~_PlatformAndroidFile();
    // Return offset in file.
    virtual int             Tell() const;
    virtual int				Size() const;
    // Seek from the start on a file.
    virtual int             Seek(long offset, Origin origin);

    // Read data from the file to the buffer.
    virtual size_t          Read(void *buffer, size_t bytesToRead) const;
    // Write data from the buffer to the file.
    virtual bool            Write(const void *buffer, size_t bytesToWrite);

    static Str              NormalizeFilename(const char *filename);
    static Str              NormalizeDirectoryName(const char *dirname);

    static _PlatformAndroidFile *OpenFileRead(const char *filename);
    static _PlatformAndroidFile *OpenFileWrite(const char *filename);
    static _PlatformAndroidFile *OpenFileAppend(const char *filename);

    static bool             FileExists(const char *filename);
    static size_t           FileSize(const char *filename);
    static bool             IsFileWritable(const char *filename);
    static bool             IsReadOnly(const char *filename);
    static bool             SetReadOnly(const char *filename, bool readOnly);
    static bool             RemoveFile(const char *filename);
    static bool             MoveFile(const char *srcFilename, const char *dstFilename);

    static DateTime         GetTimeStamp(const char *filename);
    static void             SetTimeStamp(const char *filename, const DateTime &timeStamp);

    static bool             DirectoryExists(const char *dirname);
    static bool             CreateDirectory(const char *dirname);
    static bool             RemoveDirectory(const char *dirname);
    static bool             RemoveDirectoryTree(const char *dirname);

    static const char *     Cwd();
    static const char *     ExecutablePath();
    static const char *     HomePath();

    static int              ListFiles(const char *directory, const char *nameFilter, bool recursive, bool includeSubDir, Array<FileInfo> &files);

protected:
    FILE *m_fp;
    AAsset *m_asset;
};




class BE_API PlatformAndroidFile : public _PlatformAndroidFile {
public:
    PlatformAndroidFile(FILE *fp) : _PlatformAndroidFile(fp) {}
    PlatformAndroidFile(AAsset *asset) : _PlatformAndroidFile(asset) {}
    virtual                 ~PlatformAndroidFile();
    
    static Str              NormalizeFilename(const char *filename);
    static Str              NormalizeDirectoryName(const char *dirname);
    
    static PlatformAndroidFile *OpenFileRead(const char *filename);
    static PlatformAndroidFile *OpenFileWrite(const char *filename);
    static PlatformAndroidFile *OpenFileAppend(const char *filename);
    
    static bool             FileExists(const char *filename);
    static size_t           FileSize(const char *filename);
    static bool             IsFileWritable(const char *filename);
    static bool             IsReadOnly(const char *filename);
    static bool             RemoveFile(const char *filename);
    static bool             MoveFile(const char *srcFilename, const char *dstFilename);

    static const char *     ExecutablePath();
    static void					SetExecutablePath(const Str &path);
    static const char *     HomePath();
    
    static Str              ConvertToAndroidPath(const Str &filename, bool forWrite);
    static AAssetManager*	s_manager;
    static void		SetManager(AAssetManager* manager) { s_manager = manager;  }
    static AAssetManager* GetManager() { return s_manager;  }
};

typedef PlatformAndroidFile     PlatformFile;

BE_NAMESPACE_END