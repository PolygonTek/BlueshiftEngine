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
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <utime.h>
#include <dirent.h>
#include <pwd.h>
#include <ftw.h>
#include <fcntl.h>

BE_NAMESPACE_BEGIN

PlatformPosixFile::PlatformPosixFile(FILE *fp) {
    assert(fp);
    this->fp = fp;
}

PlatformPosixFile::~PlatformPosixFile() {
    fclose(fp);
}

int PlatformPosixFile::Tell() const {
    return (int)ftell(fp);
}

int PlatformPosixFile::Size() const {
    struct stat fileInfo;
    fileInfo.st_size = -1;
    if (fstat(fileno(fp), &fileInfo) < 0) {
        return -1;
    }
    return (int)fileInfo.st_size;
}

int PlatformPosixFile::Seek(long offset, Origin origin) {
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
    
    return fseek(fp, offset, whence);
}

size_t PlatformPosixFile::Read(void *buffer, size_t bytesToRead) const {
    size_t readBytes = fread(buffer, 1, bytesToRead, fp);
    if (readBytes == -1) {
        BE_FATALERROR("PlatformPosixFile::Read: -1 bytes read");
        return 0;
    }
    
    return readBytes;
}

bool PlatformPosixFile::Write(const void *buffer, size_t bytesToWrite) {
    byte *ptr = (byte *)buffer;
    size_t writeSize = bytesToWrite;
    bool failedOnce = false;
    
    while (writeSize) {
        size_t written = fwrite(ptr, 1, writeSize, fp);
        if (!written) {
            if (!failedOnce) {
                failedOnce = true;
            } else {
                BE_WARNLOG("PlatformPosixFile::Write: 0 bytes written");
                return false;
            }
        } else if (written == -1) {
            BE_WARNLOG("PlatformPosixFile::Write: -1 bytes written");
            return false;
        }
        
        writeSize -= written;
        ptr += written;
    }
    
    fflush(fp);
    return true;
}

//-------------------------------------------------------------------------------------------

Str PlatformPosixFile::NormalizeFilename(const char *filename) {
    Str normalizedFilename;
    if (FileSystem::IsAbsolutePath(filename)) {
        normalizedFilename = filename;
    } else {
        normalizedFilename = basePath;
        normalizedFilename.AppendPath(filename);
    }
    normalizedFilename.CleanPath('/');
    //normalizedFilename.BackSlashesToSlashes();

    return normalizedFilename;
}

Str PlatformPosixFile::NormalizeDirectoryName(const char *dirname) {
    Str normalizedDirname;
    if (FileSystem::IsAbsolutePath(dirname)) {
        normalizedDirname = dirname;
    } else {
        normalizedDirname = basePath;
        normalizedDirname.AppendPath(dirname);
    }
    normalizedDirname.CleanPath('/');
    //normalizedDirname.BackSlashesToSlashes();

    int length = normalizedDirname.Length();
    if (length > 0) {
        if (normalizedDirname[length - 1] != '/') {
            normalizedDirname.Append('/');
        }
    }

    return normalizedDirname;
}

PlatformPosixFile *PlatformPosixFile::OpenFileRead(const char *filename) {
    Str normalizedFilename = NormalizeFilename(filename);
    FILE *fp = fopen(normalizedFilename, "rb");
    if (!fp) {
        return nullptr;
    }
    return new PlatformPosixFile(fp);
}

PlatformPosixFile *PlatformPosixFile::OpenFileWrite(const char *filename) {
    Str normalizedFilename = NormalizeFilename(filename);
    FILE *fp = fopen(normalizedFilename, "wb");
    if (!fp) {
        return nullptr;
    }    
    return new PlatformPosixFile(fp);
}

PlatformPosixFile *PlatformPosixFile::OpenFileAppend(const char *filename) {
    Str normalizedFilename = NormalizeFilename(filename);
    FILE *fp = fopen(normalizedFilename, "ab");
    if (!fp) {
        return nullptr;
    }    
    return new PlatformPosixFile(fp);
}

bool PlatformPosixFile::FileExists(const char *filename) {
    struct stat fileInfo;
    Str normalizedFilename = NormalizeFilename(filename);
    if (stat(normalizedFilename, &fileInfo) == 0 && S_ISREG(fileInfo.st_mode)) {
        return true;
    }
    return false;
}

size_t PlatformPosixFile::FileSize(const char *filename) {
    struct stat fileInfo;
    fileInfo.st_size = -1;
    Str normalizedFilename = NormalizeFilename(filename);
    stat(normalizedFilename, &fileInfo);
    
    // make sure to return -1 for directories
    if (S_ISDIR(fileInfo.st_mode)) {
        fileInfo.st_size = -1;
    }
    
    return fileInfo.st_size;
}

bool PlatformPosixFile::IsFileWritable(const char *filename) {
    struct stat fileInfo;
    Str normalizedFilename = NormalizeFilename(filename);
    if (stat(normalizedFilename, &fileInfo) == -1) {
        return true;
    }
    return (fileInfo.st_mode & S_IWUSR) != 0;
}

bool PlatformPosixFile::IsReadOnly(const char *filename) {
    Str normalizedFilename = NormalizeFilename(filename);
    if (access(normalizedFilename, F_OK) == -1) {
        return false; // file doesn't exist
    }
    if (access(normalizedFilename, W_OK) == -1) {
        return errno == EACCES;
    }
    return false;
}

bool PlatformPosixFile::SetReadOnly(const char *filename, bool readOnly) {
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

bool PlatformPosixFile::RemoveFile(const char *filename) {
    Str normalizedFilename = NormalizeFilename(filename);
    if (remove(normalizedFilename)) {
        BE_LOG("failed to remove file '%s'\n", normalizedFilename.c_str());
        return false;
    }

    return true;
}

bool PlatformPosixFile::MoveFile(const char *srcFilename, const char *dstFilename) {
    Str normalizedSrcFilename = NormalizeFilename(srcFilename);
    Str normalizedDstFilename = NormalizeFilename(dstFilename);
    return rename(normalizedSrcFilename, normalizedDstFilename) == 0;
}

int PlatformPosixFile::GetFileMode(const char *filename) {
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

void PlatformPosixFile::SetFileMode(const char *filename, int fileMode) {
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

DateTime PlatformPosixFile::GetTimeStamp(const char *filename) {
    static const DateTime epoch(1970, 1, 1);
    struct stat fileInfo;

    Str normalizedFilename = NormalizeFilename(filename);
    if (stat(normalizedFilename, &fileInfo) == -1) {
        return DateTime::MinValue();
    }
    
    Timespan timeSinceEpoch = Timespan::FromSeconds(fileInfo.st_mtime);
    return epoch + timeSinceEpoch;
}

void PlatformPosixFile::SetTimeStamp(const char *filename, const DateTime &timeStamp) {
    static const DateTime epoch(1970, 1, 1);
    struct stat fileInfo;

    Str normalizedFilename = NormalizeFilename(filename);
    if (stat(normalizedFilename, &fileInfo) == -1) {
        return;
    }
    
    struct utimbuf times;
    times.actime = fileInfo.st_atime;
    times.modtime = (timeStamp - epoch).TotalSeconds();
    utime(filename, &times);
}

bool PlatformPosixFile::DirectoryExists(const char *dirname) {
    struct stat fileInfo;
    Str normalizedDirname = NormalizeDirectoryName(dirname);
    if (stat(normalizedDirname, &fileInfo) == 0 && S_ISDIR(fileInfo.st_mode)) {
        return true;
    }
    return false;
}

bool PlatformPosixFile::CreateDirectory(const char *dirname) {
    if (DirectoryExists(dirname)) {
        return true;
    }
    return mkdir(NormalizeDirectoryName(dirname), S_IRWXU | S_IRWXG | S_IRWXO) == 0;
}

bool PlatformPosixFile::RemoveDirectory(const char *dirname) {
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

bool PlatformPosixFile::RemoveDirectoryTree(const char *dirname) {
    Str normalizedDirname = NormalizeDirectoryName(dirname);
    return nftw(normalizedDirname, unlink_cb, 64, FTW_DEPTH | FTW_PHYS) == 0;
}

const char *PlatformPosixFile::Cwd() {
    static char cwd[MaxAbsolutePath];

    getcwd(cwd, sizeof(cwd) - 1);
    cwd[sizeof(cwd) - 1] = 0;
    return cwd;
}

bool PlatformPosixFile::SetCwd(const char *dirname) {
    return chdir(dirname) == 0 ? true : false;
}

const char *PlatformPosixFile::ExecutablePath() {
    return Cwd();
}

static void ListFilesRecursive(const char *directory, const char *subdir, const char *nameFilter, bool includeSubDir, Array<FileInfo> &files) {
    FileInfo    fileInfo;
    char        path[MaxAbsolutePath];
    char        subpath[MaxAbsolutePath];
    char        filename[MaxAbsolutePath];
    
    if (subdir[0]) {
        Str::snPrintf(path, sizeof(path), "%s/%s", directory, subdir);
    } else {
        Str::snPrintf(path, sizeof(path), "%s", directory);
    }
    
    DIR *dp = opendir(path);
    if (!dp) {
        return;
    }
    
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

int PlatformPosixFile::ListFiles(const char *directory, const char *nameFilter, bool recursive, bool includeSubDir, Array<FileInfo> &files) {
    if (!nameFilter) {
        nameFilter = "*";
    }
    
    files.Clear();
    
    Str normalizedDirectory = NormalizeFilename(directory);
    
    if (recursive) {
        ListFilesRecursive(normalizedDirectory, "", nameFilter, includeSubDir, files);
    } else {
        DIR *dp = opendir(normalizedDirectory);
        if (!dp) {
            return 0;
        }
        
        FileInfo fileInfo;
        while (dirent *dent = readdir(dp)) {
            if (!Str::Filter(nameFilter, dent->d_name)) {
                continue;
            }

            if (includeSubDir) {
                fileInfo.isSubDir = (dent->d_type & DT_DIR) ? true : false;
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
    
    return files.Count();
}

PlatformPosixFileMapping::PlatformPosixFileMapping(int fileHandle, size_t size, const void *data) {
    this->fileHandle = fileHandle;
    this->size = size;
    this->data = data;
}

PlatformPosixFileMapping::~PlatformPosixFileMapping() {
    int retval = munmap((void *)data, size);
    if (retval != 0) {
        BE_ERRLOG("Unable to unmap memory\n");
    }
    msync(data, size, MS_SYNC);
    close(fileHandle);
}

void PlatformPosixFileMapping::Touch() {
    size_t pageSize = (size_t)sysconf(_SC_PAGESIZE);

    uint32_t checkSum = 0;
    for (byte *ptr = (byte *)data; ptr < (byte *)data + size; ptr += pageSize) {
        checkSum += *(uint32_t *)ptr;
    }
}

PlatformPosixFileMapping *PlatformPosixFileMapping::OpenFileRead(const char *filename) {
    Str normalizedFilename = NormalizeFilename(filename);
    int fd = open(normalizedFilename, O_RDONLY);
    if (fd == -1) {
        BE_ERRLOG("PlatformPosixFileMapping::Open: Couldn't open %s\n", filename);
        return nullptr;
    }

    struct stat fs;
    fstat(fileHandle, &fs);
    size_t size = fs.st_size;

    void *data = mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0);
    if (!data) {
        BE_ERRLOG("PlatformPosixFileMapping::Open: Couldn't map %s to memory\n", filename);
        return nullptr;
    }

    return new PlatformPosixFileMapping(fd, size, data);
}

PlatformPosixFileMapping *PlatformPosixFileMapping::OpenFileReadWrite(const char *filename, int newSize) {
    Str normalizedFilename = NormalizeFilename(filename);
    int fd = open(normalizedFilename, O_RDWR | O_CREAT);
    if (fd == -1) {
        BE_ERRLOG("PlatformPosixFileMapping::Open: Couldn't open %s\n", filename);
        return nullptr;
    }

    size_t size = newSize;
    if (size == 0) {
        struct stat fs;
        fstat(fileHandle, &fs);
        size = fs.st_size;
    }

    void *data = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (!data) {
        BE_ERRLOG("PlatformPosixFileMapping::Open: Couldn't map %s to memory\n", filename);
        return nullptr;
    }

    return new PlatformPosixFileMapping(fd, size, data);
}


BE_NAMESPACE_END
