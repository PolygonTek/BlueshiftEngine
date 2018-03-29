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
#include "File/FileSystem.h"
#include "Platform/PlatformFile.h"
#include "Platform/Windows/PlatformWinFile.h"
#include "Platform/PlatformProcess.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <errno.h>
#include <float.h>
#include <fcntl.h>
#include <direct.h>
#include <io.h>
#include <sys/utime.h>

#ifndef __MRC__
#include <sys/types.h>
#include <sys/stat.h>
#endif

BE_NAMESPACE_BEGIN

static const size_t READWRITE_SIZE = 1024 * 1024;

PlatformWinFile::PlatformWinFile(HANDLE fileHandle) {
    this->fileHandle = fileHandle;
}

PlatformWinFile::~PlatformWinFile() {
    CloseHandle(fileHandle);
}

static int64_t Win_FileSeek(HANDLE fileHandle, int64_t distance, DWORD moveMethod) {
    LARGE_INTEGER li;
    li.QuadPart = distance;
    li.LowPart = SetFilePointer(fileHandle, li.LowPart, &li.HighPart, moveMethod);
    if (li.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR) {
        li.QuadPart = -1;
    }
    return li.QuadPart;
}

int PlatformWinFile::Tell() const {
    return (int)Win_FileSeek(fileHandle, 0, FILE_CURRENT);
}

int PlatformWinFile::Size() const {
    return GetFileSize(fileHandle, nullptr);
}

int PlatformWinFile::Seek(long offset, Origin origin) {
    assert(offset >= 0);

    DWORD moveMethod;
    switch (origin) {
    case Start:
        moveMethod = FILE_BEGIN;
        break;
    case End:
        moveMethod = FILE_END;
        break;
    case Current:
        moveMethod = FILE_CURRENT;
        break;
    default:
        assert(0);
        break;
    }
    
    return Win_FileSeek(fileHandle, offset, moveMethod) != INVALID_SET_FILE_POINTER ? 0 : -1;
}

size_t PlatformWinFile::Read(void *buffer, size_t bytesToRead) const {
    byte *ptr = (byte *)buffer;
    DWORD readBytes;
    
    while (bytesToRead) {
        size_t thisSize = Min(READWRITE_SIZE, bytesToRead);
        if (!ReadFile(fileHandle, ptr, thisSize, &readBytes, nullptr)) {
            return 0;
        }        
        
        ptr += readBytes;
        bytesToRead -= readBytes;

        if (readBytes != (DWORD)thisSize) {
            break;
        }
    }
    
    return (size_t)((ptr - (byte *)buffer));
}

bool PlatformWinFile::Write(const void *buffer, size_t bytesToWrite) {
    byte *ptr = (byte *)buffer;
    DWORD writeBytes;

    while (bytesToWrite) {
        size_t thisSize = Min(READWRITE_SIZE, bytesToWrite);
        if (!WriteFile(fileHandle, ptr, thisSize, &writeBytes, nullptr) || writeBytes != (DWORD)thisSize) {
            return false;
        }

        ptr += thisSize;
        bytesToWrite -= thisSize;
    }

    return true;
}

//-------------------------------------------------------------------------------------------

Str PlatformWinFile::NormalizeFilename(const char *filename) {
    Str normalizedFilename;
    if (FileSystem::IsAbsolutePath(filename)) {
        normalizedFilename = filename;
    } else {
        normalizedFilename = basePath;
        normalizedFilename.AppendPath(filename);
    }
    normalizedFilename.CleanPath(PATHSEPERATOR_CHAR);
    
    return normalizedFilename;
}

Str PlatformWinFile::NormalizeDirectory(const char *dirname) {
    Str normalizedDirname;
    if (FileSystem::IsAbsolutePath(dirname)) {
        normalizedDirname = dirname;
    } else {
        normalizedDirname = basePath;
        normalizedDirname.AppendPath(dirname);
    }
    normalizedDirname.CleanPath(PATHSEPERATOR_CHAR);

    int length = normalizedDirname.Length();
    if (length > 0) {
        if (normalizedDirname[length - 1] != PATHSEPERATOR_CHAR) {
            normalizedDirname.Append(PATHSEPERATOR_CHAR);
        }
    }
    
    return normalizedDirname;
}

PlatformWinFile *PlatformWinFile::OpenFileRead(const char *filename) {
    DWORD access = GENERIC_READ;
    DWORD shareMode = FILE_SHARE_READ;
    DWORD creation = OPEN_EXISTING;
    HANDLE handle = CreateFileA(NormalizeFilename(filename), access, shareMode, nullptr, creation, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        return nullptr;
    }
    return new PlatformWinFile(handle);
}

PlatformWinFile *PlatformWinFile::OpenFileWrite(const char *filename) {
    DWORD access = GENERIC_WRITE;
    DWORD shareMode = 0;
    DWORD creation = CREATE_ALWAYS;
    HANDLE handle = CreateFileA(NormalizeFilename(filename), access, shareMode, nullptr, creation, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        WStr lastErrorText = PlatformWinProcess::GetLastErrorText();
        BE_WARNLOG(L"Failed to CreateFile : %ls", lastErrorText.c_str());
        return nullptr;
    }
    return new PlatformWinFile(handle);
}

PlatformWinFile *PlatformWinFile::OpenFileAppend(const char *filename) {
    DWORD access = GENERIC_WRITE;
    DWORD shareMode = 0;
    DWORD creation = OPEN_ALWAYS;
    HANDLE handle = CreateFileA(NormalizeFilename(filename), access, shareMode, nullptr, creation, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        return nullptr;
    }
    return new PlatformWinFile(handle);
}

bool PlatformWinFile::FileExists(const char *filename) {
    struct _stat fileInfo;
    if (_stat(NormalizeFilename(filename), &fileInfo) == 0 && (fileInfo.st_mode & S_IFREG)) {
        return true;
    }
    return false;
}

size_t PlatformWinFile::FileSize(const char *filename) {
    WIN32_FILE_ATTRIBUTE_DATA info;
    if (!!GetFileAttributesExA(NormalizeFilename(filename), GetFileExInfoStandard, &info)) {
        if ((info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
            LARGE_INTEGER li;
            li.HighPart = info.nFileSizeHigh;
            li.LowPart = info.nFileSizeLow;
            return li.QuadPart;
        }
    }
    return -1;
}

bool PlatformWinFile::IsFileWritable(const char *filename) {
    struct _stat fileInfo;
    if (_stat(NormalizeFilename(filename), &fileInfo) == -1) {
        return true;
    }
    return (fileInfo.st_mode & S_IWRITE) != 0;
}

bool PlatformWinFile::IsReadOnly(const char *filename) {
    DWORD result = GetFileAttributesA(NormalizeFilename(filename));
    if (result == 0xFFFFFFFF) {
        return false;
    }
    return !!(result & FILE_ATTRIBUTE_READONLY);
}

bool PlatformWinFile::SetReadOnly(const char *filename, bool readOnly) {
    return !!SetFileAttributesA(NormalizeFilename(filename), readOnly ? FILE_ATTRIBUTE_READONLY : FILE_ATTRIBUTE_NORMAL);
}

bool PlatformWinFile::RemoveFile(const char *filename) {
    return !!DeleteFileA(NormalizeFilename(filename));
}

bool PlatformWinFile::MoveFile(const char *from, const char *to) {
    Str normalizedFrom = NormalizeFilename(from);
    Str normalizedTo = NormalizeFilename(to);
    return !!MoveFileA(normalizedFrom, normalizedTo);
}

int PlatformWinFile::GetFileMode(const char *filename) {
    struct _stat fileInfo;
    if (_stat(NormalizeFilename(filename), &fileInfo) != 0) {
        return -1;
    }
    int fileMode = 0;
    if (fileInfo.st_mode & _S_IREAD) {
        fileMode |= Readable;
    }
    if (fileInfo.st_mode & _S_IWRITE) {
        fileMode |= Writable;
    }
    if (fileInfo.st_mode & _S_IEXEC) {
        fileMode |= Executable;
    }
    return fileMode;
}

void PlatformWinFile::SetFileMode(const char *filename, int fileMode) {
    int mode = 0;
    if (fileMode & Readable) {
        mode |= _S_IREAD;
    }
    if (fileMode & Writable) {
        mode |= _S_IWRITE;
    }
    if (fileMode & Executable) {
        mode |= _S_IEXEC;
    }
    _chmod(NormalizeFilename(filename), mode);
}

DateTime PlatformWinFile::GetTimeStamp(const char *filename) {
    static const DateTime epoch(1970, 1, 1);
    struct _stat fileInfo;
    if (_stat(NormalizeFilename(filename), &fileInfo) == -1) {
        return DateTime::MinValue();
    }
    
    Timespan timeSinceEpoch = Timespan::FromSeconds(fileInfo.st_mtime);
    return epoch + timeSinceEpoch;
}

void PlatformWinFile::SetTimeStamp(const char *filename, const DateTime &timeStamp) {
    static const DateTime epoch(1970, 1, 1);
    struct stat fileInfo;
    if (stat(NormalizeFilename(filename), &fileInfo) == -1) {
        return;
    }
    
    struct _utimbuf times;
    times.actime = fileInfo.st_atime;
    times.modtime = (timeStamp - epoch).TotalSeconds();
    _utime(filename, &times);
}

bool PlatformWinFile::DirectoryExists(const char *dirname) {
    struct _stat fileInfo;
    if (_stat(NormalizeFilename(dirname), &fileInfo) == 0 && (fileInfo.st_mode & S_IFDIR)) {
        return true;
    }
    return false;
}

bool PlatformWinFile::CreateDirectory(const char *dirname) {
    if (DirectoryExists(dirname)) {
        return true;
    }
    return _mkdir(NormalizeFilename(dirname)) == 0;
}

static bool RemoveDirRecursive(const char *path) {
    char findPath[MAX_PATH];
    strcpy(findPath, path);
    strcat(findPath, "\\*");
    
    char filename[MAX_PATH];
    strcpy(filename, path);
    strcat(filename, "\\");
    
    _finddata_t finddata;
    intptr_t handle = _findfirst(findPath, &finddata);
    if (handle == -1) {
        return false;
    }
    strcpy(findPath, filename);
    
    while (1) {
        if (_findnext(handle, &finddata) == -1) {
            if (GetLastError() == ERROR_NO_MORE_FILES) {
                break;
            } else {
                // some error occured, close the handle and return FALSE
                _findclose(handle);
                return false;
            }
        }
        
        if (!strcmp(finddata.name, ".") || !strcmp(finddata.name, "..")) {
            continue;
        }
        
        strcat(filename, finddata.name);
        
        if (finddata.attrib & _A_SUBDIR) {
            if (!RemoveDirRecursive(filename)) {
                // directory couldn't be deleted
                _findclose(handle);
                return false;
            }
            
            // remove the empty directory
            RemoveDirectoryA(filename);
            strcpy(filename, findPath);
        } else {
            if (finddata.attrib & _A_RDONLY) {
                // change read-only file mode
                _chmod(filename, _S_IWRITE);
            }
            
            if (!DeleteFileA(filename)) {
                _findclose(handle);
                return false;
            }
            
            strcpy(filename, findPath);
        }
    }
    
    _findclose(handle);
    
    // remove the empty directory
    if (!RemoveDirectoryA(path)) {
        WStr lastErrorText = PlatformWinProcess::GetLastErrorText();
        BE_WARNLOG(L"Failed to RemoveDirectory : %ls", lastErrorText.c_str());
        return false;
    }
    
    return true;
}

bool PlatformWinFile::RemoveDirectoryTree(const char *dirname) {
    Str normalizedDirname = NormalizeFilename(dirname);
    return RemoveDirRecursive(normalizedDirname);
}

bool PlatformWinFile::RemoveDirectory(const char *dirname) {
    Str normalizedDirname = NormalizeFilename(dirname);
    return _rmdir(normalizedDirname) == 0;
}

const char *PlatformWinFile::Cwd() {
    static char cwd[MaxAbsolutePath];
    
    _getcwd(cwd, sizeof(cwd) - 1);
    cwd[sizeof(cwd) - 1] = 0;
    return cwd;
}

bool PlatformWinFile::SetCwd(const char *dirname) {
    return _chdir(dirname) == 0 ? true : false;
}

const char *PlatformWinFile::ExecutablePath() {
    static char path[MaxAbsolutePath];
    GetModuleFileNameA(nullptr, path, COUNT_OF(path));

    for (int i = strlen(path) - 1; i >= 0; i--) {
        if (path[i] == '\\') {
            path[i + 1] = '\0';
            break;
        }
    }
    return path;
}

void PlatformWinFile::ListFilesRecursive(const char *directory, const char *subdir, const char *nameFilter, bool includeSubDir, Array<FileInfo> &files) {
    FileInfo    fileInfo;
    _finddata_t finddata;
    char        pattern[MaxAbsolutePath];
    char        subpath[MaxAbsolutePath];
    char        filename[MaxAbsolutePath];

    if (subdir[0]) {
        Str::snPrintf(pattern, sizeof(pattern), "%s%s\\*", directory, subdir);
    } else {
        Str::snPrintf(pattern, sizeof(pattern), "%s*", directory);
    }

    intptr_t handle = _findfirst(pattern, &finddata);
    if (handle == -1) {
        return;
    }

    do {
        if (!Str::Cmp(finddata.name, ".") || !Str::Cmp(finddata.name, "..")) {
            continue;
        }

        //fileInfo.size = finddata.size;

        if (finddata.attrib & _A_SUBDIR) {
            if (subdir[0]) {
                Str::snPrintf(subpath, sizeof(subpath), "%s\\%s", subdir, finddata.name);
            } else {
                Str::snPrintf(subpath, sizeof(subpath), "%s", finddata.name);
            }

            fileInfo.isSubDir = true;
            if (includeSubDir) {
                fileInfo.relativePath = subpath;
                files.Append(fileInfo);
            }

            ListFilesRecursive(directory, subpath, nameFilter, includeSubDir, files);
        } else {
            fileInfo.isSubDir = false;
        }

        if (Str::Filter(nameFilter, finddata.name, false)) {
            if (subdir[0]) {
                Str::snPrintf(filename, sizeof(filename), "%s\\%s", subdir, finddata.name);
            } else {
                Str::snPrintf(filename, sizeof(filename), "%s", finddata.name);
            }

            fileInfo.relativePath = filename;
            files.Append(fileInfo);
        }
    } while (_findnext(handle, &finddata) != -1);

    _findclose(handle);
}

int PlatformWinFile::ListFiles(const char *directory, const char *nameFilter, bool recursive, bool includeSubDir, Array<FileInfo> &files) {
    if (!nameFilter) {
        nameFilter = "*";
    }

    files.Clear();
    
    Str normalizedDirectory = NormalizeDirectory(directory);

    if (recursive) {
        ListFilesRecursive(normalizedDirectory, "", nameFilter, includeSubDir, files);
    } else {
        char pattern[MaxAbsolutePath];
        Str::snPrintf(pattern, sizeof(pattern), "%s\\%s", normalizedDirectory.c_str(), nameFilter);

        _finddata_t finddata;
        intptr_t handle = _findfirst(pattern, &finddata);
        if (handle == -1) {
            return 0;
        }

        FileInfo fileInfo;
        do {
            if (includeSubDir) {
                fileInfo.isSubDir = (finddata.attrib & _A_SUBDIR) ? true : false;
            } else {
                if (finddata.attrib & _A_SUBDIR) {
                    continue;
                }
                fileInfo.isSubDir = false;
            }

            //fileInfo.size = finddata.size;
            fileInfo.relativePath = finddata.name;

            files.Append(fileInfo);
        } while (_findnext(handle, &finddata) != -1);

        _findclose(handle);
    }

    return files.Count();
}

BE_NAMESPACE_END
