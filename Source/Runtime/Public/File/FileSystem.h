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

    File system

-------------------------------------------------------------------------------
*/

#include "Core/Dict.h"
#include "File/File.h"

BE_NAMESPACE_BEGIN

class CmdArgs;
struct ZipArchive;

struct ProgressCallback {
    virtual void        SetText(const char *text) = 0;
    virtual bool        Poll(float fraction) = 0;
};

class BE_API FileArray {
    friend class FileSystem;

public:
    /// Get the absolute path
    const char *        GetPath() const { return path; }
    int                 NumFiles() const { return array.Count(); }
    const char *        GetFilename(int index) const { return array[index].relativePath; }
    Array<FileInfo> &   GetArray() { return array; }

private:
    Str                 path;
    Array<FileInfo>     array;
};

class BE_API FileSystem {
public:
    void                Init(const char *baseDir);
    void                Shutdown();
    void                Restart(const char *baseDir = nullptr);
    
    void                SetSearchPath(const char *path);
 
    const char *        GetBaseDir() const;
    void                SetBaseDir(const char *baseDir);
    
    static bool         IsRelativePath(const char *path);
    static bool         IsAbsolutePath(const char *path);

    const Str           ToRelativePath(const char *absolutePath) const;
    const Str           ToAbsolutePath(const char *relativePath) const;

    int                 MakeFullPath(char *fullpath, int size, const char *base, const char *directory, const char *file);

    bool                FileExists(const char *filename) const;
    size_t              FileSize(const char *filename) const;
    bool                RemoveFile(const char *filename, bool evenReadOnly);
    bool                MoveFile(const char *oldname, const char *newname);
    bool                CopyFile(const char *from, const char *to, ProgressCallback *progress = nullptr);
    bool                CopyDirectoryTree(const char *from, const char *to);
    bool                CopyFileOrDirectoryTree(const char *from, const char *to);
    bool                IsReadOnly(const char *filename);
    bool                SetReadOnly(const char *filename, bool readOnly);

    DateTime            GetTimeStamp(const char *filename) const;
    void                SetTimeStamp(const char *filename, const DateTime &timeStamp);
    
    bool                DirectoryExists(const char *dirname) const;
    bool                CreateDirectory(const char *dirname, bool tree) const;
    bool                RemoveDirectory(const char *dirname, bool tree) const;
        
    File *              OpenFile(const char *filename, File::Mode mode, bool searchDirs = true);
    File *              OpenFileRead(const char *filename, bool useSearchPath, size_t *fileSize = nullptr);
    File *              OpenFileWrite(const char *filename);
    File *              OpenFileAppend(const char *filename);
    void                CloseFile(File *f);

    size_t              LoadFile(const char *filename, bool searchDirs, void **buffer);
    void                FreeFile(void *buffer) const;
    
    void                WriteFile(const char *filename, const void *buffer, int size);

    bool                ReadDict(const char *filename, Dict &dict);
    bool                WriteDict(const char *filename, const Dict &dict);
    
    int                 ListFiles(const char *findPath, const char *nameFilter, FileArray &fileArray, bool searchDirs = true, bool includeSubDir = false, bool sort = false, bool recursive = true);

    Str                 GetDocumentDir() const;
    Str                 GetAppDataDir(const char *org, const char *app) const;
    
private:
    struct SearchPath {
        char *          pathname;
        ZipArchive *    archive;
        SearchPath *    next;
    };

    SearchPath *        searchPath;
    
    void                ClearSearchPath();
    void                AddSearchPath(const char *path);
    void                AddSearchPath_ZIP(const char *path, const char *filename);
    
    static void         Cmd_Dir(const CmdArgs &args);
    static void         Cmd_Path(const CmdArgs &args);
};

extern FileSystem       fileSystem;

BE_NAMESPACE_END
