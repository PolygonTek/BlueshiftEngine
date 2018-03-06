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
#include "PlatformUtils/Android/AndroidJNI.h"
#include <android/asset_manager.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <ftw.h>

BE_NAMESPACE_BEGIN

PlatformAndroidFile::PlatformAndroidFile(FILE *fp) {
    this->fp = fp;
    this->asset = 0;
}

PlatformAndroidFile::PlatformAndroidFile(AAsset *asset) {
    this->fp = 0;
    this->asset = asset;
}

PlatformAndroidFile::~PlatformAndroidFile() {
    if (fp) {
        fclose(fp);
    } 
    if (asset) {
        AAsset_close(asset);
    }
}

int PlatformAndroidFile::Tell() const {
    if (fp) {
        return (int)ftell(fp);
    }
    if (asset) {
        return (int)AAsset_getLength(asset) - (int)AAsset_getRemainingLength(asset);
    }
    return -1;
}

int PlatformAndroidFile::Size() const {
    if (fp) {
        struct stat fileInfo;
        fileInfo.st_size = -1;
        if (fstat(fileno(fp), &fileInfo) < 0) {
            return -1;
        }
        return (int)fileInfo.st_size;
    } 
    if (asset) {
        return (int)AAsset_getLength(asset);
    }
    return -1;
}

int PlatformAndroidFile::Seek(long offset, Origin origin) {
    assert(offset >= 0);

    int whence;
    switch (origin) {
    case Start:
        whence = SEEK_SET;
        break;
    case End:
        whence = SEEK_END;
        break;
    case Current:
        whence = SEEK_CUR;
        break;
    default:
        assert(0);
        break;
    }

    if (fp) {
        return fseek(fp, offset, whence);
    }
    if (asset) {
        return AAsset_seek(asset, offset, whence) >= 0 ? 0 : -1;
    }
    return -1;
}

size_t PlatformAndroidFile::Read(void *buffer, size_t bytesToRead) const {
    assert(bytesToRead > 0);
    if (fp) {
        size_t readBytes = fread(buffer, 1, bytesToRead, fp);
        if (readBytes == -1) {
            BE_FATALERROR(L"PlatformAndroidFile::Read: -1 bytes read");
            return 0;
        }
        return readBytes;
    }
    if (asset) {
        size_t avail = (size_t)AAsset_getRemainingLength(asset);
        if (avail == 0) {
            return 0;
        }
        if (bytesToRead > avail) {
            bytesToRead = avail;
        }
        jint readBytes = AAsset_read(asset, buffer, bytesToRead);
        return (size_t)readBytes;
    }
    return 0;
}

bool PlatformAndroidFile::Write(const void *buffer, size_t bytesToWrite) {
    if (!fp) {
        // Can't write to asset.
        return false;
    }

    byte *ptr = (byte *)buffer;
    size_t writeSize = bytesToWrite;
    bool failedOnce = false;

    while (writeSize) {
        size_t written = fwrite(ptr, 1, writeSize, fp);
        if (!written) {
            if (!failedOnce) {
                failedOnce = true;
            }
            else {
                BE_WARNLOG(L"PlatformAndroidFile::Write: 0 bytes written");
                return false;
            }
        }
        else if (written == -1) {
            BE_WARNLOG(L"PlatformAndroidFile::Write: -1 bytes written");
            return false;
        }

        writeSize -= written;
        ptr += written;
    }

    fflush(fp);
    return true;
}

Str PlatformAndroidFile::NormalizeFilename(const char *filename) {
    Str normalizedFilename;
    if (FileSystem::IsAbsolutePath(filename)) {
        normalizedFilename = filename;
    } else {
        normalizedFilename = GetBasePath();
        normalizedFilename.AppendPath(filename);
    }
    normalizedFilename.CleanPath(PATHSEPERATOR_CHAR);

    return normalizedFilename;
}

Str PlatformAndroidFile::NormalizeDirectoryName(const char *dirname) {
    Str normalizedDirname;
    if (FileSystem::IsAbsolutePath(dirname)) {
        normalizedDirname = dirname;
    } else {
        normalizedDirname = GetBasePath();
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

PlatformAndroidFile *PlatformAndroidFile::OpenFileRead(const char *filename) {
    // FIXME: Read newest file
    // Read from external data directory
    Str normalizedFilename = NormalizeFilename(filename);
    FILE *fp = fopen(normalizedFilename.c_str(), "rb");
    if (fp) {
        return new PlatformAndroidFile(fp);
    }
    // Read by asset manager
    AAsset *asset = AAssetManager_open(AndroidJNI::activity->assetManager, filename, AASSET_MODE_RANDOM);
    if (asset) {
        return new PlatformAndroidFile(asset);
    }        
    return nullptr;
}

PlatformAndroidFile *PlatformAndroidFile::OpenFileWrite(const char *filename) {
    Str normalizedFilename = NormalizeFilename(filename);
    FILE *fp = fopen(normalizedFilename, "wb");
    if (!fp) {
        return nullptr;
    }
    return new PlatformAndroidFile(fp);
}

PlatformAndroidFile *PlatformAndroidFile::OpenFileAppend(const char *filename) {
    Str normalizedFilename = NormalizeFilename(filename);
    FILE *fp = fopen(normalizedFilename, "ab");
    if (!fp) {
        return nullptr;
    }
    return new PlatformAndroidFile(fp);
}

bool PlatformAndroidFile::FileExists(const char *filename) {
    struct stat fileInfo;
    Str normalizedFilename = NormalizeFilename(filename);
    if (stat(normalizedFilename, &fileInfo) == 0 && S_ISREG(fileInfo.st_mode)) {
        return true;
    }
    AAsset *asset = AAssetManager_open(AndroidJNI::activity->assetManager, filename, AASSET_MODE_RANDOM);
    if (asset) {
        AAsset_close(asset);
        return true;
    }
    return false;
}

size_t PlatformAndroidFile::FileSize(const char *filename) {
    struct stat fileInfo;
    fileInfo.st_size = -1;
    Str normalizedFilename = NormalizeFilename(filename);
    if (stat(normalizedFilename, &fileInfo) == 0 && S_ISREG(fileInfo.st_mode)) {
        return fileInfo.st_size;
    }
    AAsset *asset = AAssetManager_open(AndroidJNI::activity->assetManager, filename, AASSET_MODE_RANDOM);
    if (asset) {
        size_t size = (size_t)AAsset_getLength(asset);
        AAsset_close(asset);
        return size;
    }
    return 0;
}

bool PlatformAndroidFile::IsFileWritable(const char *filename) {
    struct stat fileInfo;
    Str normalizedFilename = NormalizeFilename(filename);
    if (stat(normalizedFilename, &fileInfo) == -1) {
        return true;
    }
    return (fileInfo.st_mode & S_IWUSR) != 0;
}

bool PlatformAndroidFile::IsReadOnly(const char *filename) {
    Str normalizedFilename = NormalizeFilename(filename);
    if (access(normalizedFilename, F_OK) == -1) {
        return false; // file doesn't exist
    }
    if (access(normalizedFilename, W_OK) == -1) {
        return errno == EACCES;
    }
    return false;
}

bool PlatformAndroidFile::SetReadOnly(const char *filename, bool readOnly) {
    struct stat fileInfo;
    Str normalizedFilename = NormalizeFilename(filename);
    if (stat(normalizedFilename, &fileInfo) == -1) {
        return false;
    }
    
    if (readOnly) {
        fileInfo.st_mode &= ~S_IWUSR;
    } else {
        fileInfo.st_mode |= S_IWUSR;
    }
    return chmod(normalizedFilename, fileInfo.st_mode) == 0;
}

bool PlatformAndroidFile::RemoveFile(const char *filename) {
    Str normalizedFilename = NormalizeFilename(filename);
    if (remove(normalizedFilename)) {
        BE_LOG(L"failed to remove file '%hs'\n", normalizedFilename.c_str());
        return false;
    }

    return true;
}

bool PlatformAndroidFile::MoveFile(const char *srcFilename, const char *dstFilename) {
    Str normalizedSrcFilename = NormalizeFilename(srcFilename);
    Str normalizedDstFilename = NormalizeFilename(dstFilename);
    return rename(normalizedSrcFilename, normalizedDstFilename) == 0;
}

int PlatformAndroidFile::GetFileMode(const char *filename) {
    struct stat fileInfo;
    if (stat(NormalizeFilename(filename), &fileInfo) != 0) {
        return -1;
    }
    int fileMode = 0;
    if (fileInfo.st_mode & S_IRUSR) {
        fileMode |= Readable;
    }
    if (fileInfo.st_mode & S_IWUSR) {
        fileMode |= Writable;
    }
    if (fileInfo.st_mode & S_IXUSR) {
        fileMode |= Executable;
    }
    return fileMode;
}

void PlatformAndroidFile::SetFileMode(const char *filename, int fileMode) {
    mode_t mode = 0;
    if (fileMode & Readable) {
        mode |= S_IRUSR;
    }
    if (fileMode & Writable) {
        mode |= S_IWUSR;
    }
    if (fileMode & Executable) {
        mode |= S_IXUSR;
    }
    chmod(NormalizeFilename(filename), mode);
}

DateTime PlatformAndroidFile::GetTimeStamp(const char *filename) {
    static const DateTime epoch(1970, 1, 1);
    struct stat fileInfo;

    Str normalizedFilename = NormalizeFilename(filename);
    if (stat(normalizedFilename, &fileInfo) == -1) {
        return DateTime::MinValue();
    }
    
    Timespan timeSinceEpoch = Timespan::FromSeconds(fileInfo.st_mtime);
    return epoch + timeSinceEpoch;
}

void PlatformAndroidFile::SetTimeStamp(const char *filename, const DateTime &timeStamp) {
}

bool PlatformAndroidFile::DirectoryExists(const char *dirname) {
    struct stat fileInfo;
    Str normalizedDirname = NormalizeDirectoryName(dirname);
    if (stat(normalizedDirname, &fileInfo) == 0 && S_ISDIR(fileInfo.st_mode)) {
        return true;
    }
    return false;
}

bool PlatformAndroidFile::CreateDirectory(const char *dirname) {
    if (DirectoryExists(dirname)) {
        return true;
    }
    return mkdir(NormalizeDirectoryName(dirname), S_IRWXU | S_IRWXG | S_IRWXO) == 0;
}

bool PlatformAndroidFile::RemoveDirectory(const char *dirname) {
    Str normalizedDirname = NormalizeDirectoryName(dirname);
    return rmdir(normalizedDirname) == 0;
}

static int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    int rv = remove(fpath);
    if (rv) {
        perror(fpath);
    }

    return rv;
}

bool PlatformAndroidFile::RemoveDirectoryTree(const char *dirname) {
    Str normalizedDirname = NormalizeDirectoryName(dirname);
    return nftw(normalizedDirname, unlink_cb, 64, FTW_DEPTH | FTW_PHYS) == 0;
}

const char *PlatformAndroidFile::Cwd() {
    static char cwd[MaxAbsolutePath];

    getcwd(cwd, sizeof(cwd) - 1);
    cwd[sizeof(cwd) - 1] = 0;
    return cwd;
}

bool PlatformAndroidFile::SetCwd(const char *dirname) {
    return chdir(dirname) == 0;
}

const char *PlatformAndroidFile::ExecutablePath() {
    return AndroidJNI::activity->externalDataPath;
}

void PlatformAndroidFile::ListFilesRecursive(const char *directory, const char *subdir, const char *nameFilter, bool includeSubDir, Array<FileInfo> &files) {
    FileInfo    fileInfo;
    char		path[MaxAbsolutePath];
    char		subpath[MaxAbsolutePath];
    char		filename[MaxAbsolutePath];

    if (subdir[0]) {
        Str::snPrintf(path, sizeof(path), "%s/%s", directory, subdir);
    } else {
        Str::snPrintf(path, sizeof(path), "%s", directory);
    }

    Str normalizedPath = NormalizeFilename(path);
    DIR *dp = opendir(normalizedPath);
    if (dp) {
        while (dirent *dent = readdir(dp)) {
            if (!Str::Cmp(dent->d_name, ".") || !Str::Cmp(dent->d_name, "..")) {
                continue;
            }

            if (dent->d_type & DT_DIR) {
                if (subdir[0]) {
                    Str::snPrintf(subpath, sizeof(subpath), "%s/%s", subdir, dent->d_name);
                } else {
                    Str::snPrintf(subpath, sizeof(subpath), "%s", dent->d_name);
                }

                if (includeSubDir) {
                    fileInfo.isSubDir = true;
                    fileInfo.relativePath = subpath;
                    files.Append(fileInfo);
                }

                ListFilesRecursive(directory, subpath, nameFilter, includeSubDir, files);
            } else if (Str::Filter(nameFilter, dent->d_name, false)) {
                if (subdir[0]) {
                    Str::snPrintf(filename, sizeof(filename), "%s/%s", subdir, dent->d_name);
                } else {
                    Str::snPrintf(filename, sizeof(filename), "%s", dent->d_name);
                }

                fileInfo.isSubDir = false;
                fileInfo.relativePath = filename;
                files.Append(fileInfo);
            }
        }

        closedir(dp);
    }

    AAssetDir *assetDir = AAssetManager_openDir(AndroidJNI::activity->assetManager, path);
    if (assetDir) {
        FileInfo fileInfo;
        while (const char *filename = AAssetDir_getNextFileName(assetDir)) {
            if (!Str::Filter(nameFilter, filename)) {
                continue;
            }

            fileInfo.isSubDir = false;
            fileInfo.relativePath = filename;
            files.Append(fileInfo);
        }

        AAssetDir_close(assetDir);
    }
}

int PlatformAndroidFile::ListFiles(const char *directory, const char *nameFilter, bool recursive, bool includeSubDir, Array<FileInfo> &files) {
    if (!nameFilter) {
        nameFilter = "*";
    }

    files.Clear();

    if (recursive) {
        ListFilesRecursive(directory, "", nameFilter, includeSubDir, files);
    } else {
        Str normalizedDirectory = NormalizeFilename(directory);
        DIR *dp = opendir(normalizedDirectory);
        if (dp) {
            FileInfo fileInfo;
            while (dirent *dent = readdir(dp)) {
                if (!Str::Filter(nameFilter, dent->d_name)) {
                    continue;
                }

                if (includeSubDir) {
                    fileInfo.isSubDir = (dent->d_type & DT_DIR) != 0;
                } else {
                    if (dent->d_type & DT_DIR) {
                        continue;
                    }
                }

                fileInfo.relativePath = dent->d_name;
                files.Append(fileInfo);
            }
            closedir(dp);
        }

        AAssetDir *assetDir = AAssetManager_openDir(AndroidJNI::activity->assetManager, directory);
        if (assetDir) {
            FileInfo fileInfo;
            while (const char *filename = AAssetDir_getNextFileName(assetDir)) {
                if (!Str::Filter(nameFilter, filename)) {
                    continue;
                }

                fileInfo.isSubDir = false;
                fileInfo.relativePath = filename;
                files.Append(fileInfo);
            }

            AAssetDir_close(assetDir);
        }
    }

    return files.Count();
}

BE_NAMESPACE_END
