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
#include "Core/Heap.h"
#include "Core/CVars.h"
#include "Core/Cmds.h"
#include "Platform/PlatformSystem.h"
#include "Platform/PlatformProcess.h"
#include "File/FileSystem.h"
#include "minizip/zip.h"
#include "minizip/unzip.h"

BE_NAMESPACE_BEGIN

#define FILEHASH_SIZE       1024

static CVar                 fs_baseDir(L"fs_baseDir", L".", 0, L"");
static CVar                 fs_debug(L"fs_debug", L"0", CVar::Bool, L"");

FileSystem                  fileSystem;

//--------------------------------------------------------------------------------------------------
//
// ZIP archive
//
//--------------------------------------------------------------------------------------------------

struct ZipEntry {
    char                    name[MaxRelativePath];
    size_t                  compressedSize;
    size_t                  uncompressedSize;
    uLong                   unzOffset;
};

struct ZipArchive {
    char                    name[MaxRelativePath];
    char                    fullPath[MaxAbsolutePath];
    unzFile                 unzArchive;
    int                     numEntries;
    Array<ZipEntry *>       entryList;
    HashIndex               entryHash;
};

//--------------------------------------------------------------------------------------------------
//
// FileSystem
//
//--------------------------------------------------------------------------------------------------

void FileSystem::ClearSearchPath() {
    SearchPath *next;
    
    for (SearchPath *s = searchPath; s; s = next) {
        next = s->next;
        
        if (s->archive) {
            s->archive->entryList.DeleteContents(true);
            s->archive->entryHash.Free();
            unzClose(s->archive->unzArchive);
            delete s->archive;
        } else if (s->pathname) {
            delete[] s->pathname;
        }
        
        delete s;
    }
    
    searchPath = nullptr;
}

void FileSystem::SetSearchPath(const char *multiPath) {
    Str multiPathStr = multiPath;
    int len = (int)Str::Length(multiPath);
    int start = 0;
    
    ClearSearchPath();

    while (start < len) {
        int last = multiPathStr.Find(';', start);
        if (last == -1) {
            last = len;
        }

        Str singlePathStr = multiPathStr.Mid(start, last - start);
        if (!singlePathStr.IsEmpty()) {
            AddSearchPath(singlePathStr);
        }

        start = last + 1;
    }
}

void FileSystem::AddSearchPath(const char *path) {
    if (!path || !path[0]) {
        return;
    }

    SearchPath *search = new SearchPath;
    search->pathname = new char[MaxAbsolutePath];
    strcpy(search->pathname, path);
    search->archive = nullptr;
    search->next = searchPath;
    searchPath = search;

    // path 의 zip 파일들을 searchpath 에 추가
    Array<FileInfo> files;
    int num = PlatformFile::ListFiles(ToRelativePath(path), "*.zip", false, false, files);

    for (int i = 0; i < num; i++) {
        AddSearchPath_ZIP(path, files[i].relativePath);
    }
}

#if defined(__ANDROID__) 

static voidpf ZCALLBACK _fopen_file_func(voidpf opaque, const char* filename, int mode) {
    PlatformFile *file = nullptr;
    PlatformFile *(*mode_fopen)(const char *filename) = nullptr;
    if ((mode & ZLIB_FILEFUNC_MODE_READWRITEFILTER) == ZLIB_FILEFUNC_MODE_READ) {
        mode_fopen = PlatformFile::OpenFileRead;
    } else {
        if (mode & ZLIB_FILEFUNC_MODE_EXISTING) {
            mode_fopen = PlatformFile::OpenFileAppend;
        } else {
            if (mode & ZLIB_FILEFUNC_MODE_CREATE) {
                mode_fopen = PlatformFile::OpenFileWrite;
            }
        }
    }

    if ((filename != nullptr) && (mode_fopen != nullptr)) {
        file = (*mode_fopen)(filename);
    }
    return (FILE *) file;
}

static uLong ZCALLBACK _fread_file_func(voidpf opaque, voidpf stream, void* buf, uLong size) {
    if (((PlatformFile *)stream)->Read(buf, size)) {
        return size;
    }
    return -1;
}

static uLong ZCALLBACK _fwrite_file_func(voidpf opaque, voidpf stream, const void* buf, uLong size) {
    if (((PlatformFile *)stream)->Write(buf, size)) {
        return size;
    }
    return -1;
}

static long ZCALLBACK _ftell_file_func(voidpf opaque, voidpf stream) {
    return ((PlatformFile *)stream)->Tell();
}

static long ZCALLBACK _fseek_file_func(voidpf  opaque, voidpf stream, uLong offset, int origin) {
    enum PlatformFile::Origin fseek_origin;
    switch (origin) {
    case ZLIB_FILEFUNC_SEEK_CUR:
        fseek_origin = PlatformFile::Current;
        break;
    case ZLIB_FILEFUNC_SEEK_END:
        fseek_origin = PlatformFile::End;
        break;
    case ZLIB_FILEFUNC_SEEK_SET:
        fseek_origin = PlatformFile::Start;
        break;
    default:
        return -1;
    }
    return ((PlatformFile *)stream)->Seek(offset, fseek_origin);
}

static int ZCALLBACK _fclose_file_func(voidpf opaque, voidpf stream) {
    delete ((PlatformFile *)stream);
    return 0;
}

static int ZCALLBACK _ferror_file_func(voidpf opaque, voidpf stream) {
    return 0;
}

static void _fill_fopen_filefunc(zlib_filefunc_def* pzlib_filefunc_def) {
    pzlib_filefunc_def->zopen_file = _fopen_file_func;
    pzlib_filefunc_def->zread_file = _fread_file_func;
    pzlib_filefunc_def->zwrite_file = _fwrite_file_func;
    pzlib_filefunc_def->ztell_file = _ftell_file_func;
    pzlib_filefunc_def->zseek_file = _fseek_file_func;
    pzlib_filefunc_def->zclose_file = _fclose_file_func;
    pzlib_filefunc_def->zerror_file = _ferror_file_func;
    pzlib_filefunc_def->opaque = nullptr;
}

#endif

void FileSystem::AddSearchPath_ZIP(const char *path, const char *filename) {
    char fullpath[MaxAbsolutePath];
    fileSystem.MakeFullPath(fullpath, sizeof(fullpath), path, "", filename);
    
    unz_global_info z_global_info;
#if defined(__ANDROID__) 
    zlib_filefunc_def zlib_filefunc32_def;
    _fill_fopen_filefunc(&zlib_filefunc32_def);
    unzFile z_file = unzOpen2(ToRelativePath(fullpath), &zlib_filefunc32_def);
#else
    unzFile z_file = unzOpen(fullpath);
#endif
    if (unzGetGlobalInfo(z_file, &z_global_info) != UNZ_OK) {
        return;
    }

    ZipArchive *archive = new ZipArchive;
        
    strcpy(archive->fullPath, fullpath);
    strcpy(archive->name, filename);

    archive->unzArchive = z_file;
    archive->numEntries = (int)z_global_info.number_entry;
    
    archive->entryList.Resize((int)z_global_info.number_entry);
    
    // Set the current file of the zipfile to the first file.
    unzGoToFirstFile(z_file);
    
    for (int i = 0; i < z_global_info.number_entry; i++) {
        unz_file_info z_entry_info;
        char entryFilename[MaxRelativePath];

        if (unzGetCurrentFileInfo(z_file, &z_entry_info, entryFilename, COUNT_OF(entryFilename), nullptr, 0, nullptr, 0) != UNZ_OK) {
            break;
        }

        Str::ConvertPathSeperator(entryFilename, PATHSEPERATOR_CHAR);

        ZipEntry *entry = new ZipEntry;
        strcpy(entry->name, entryFilename);
        entry->unzOffset = unzGetOffset(z_file);
        entry->compressedSize = z_entry_info.compressed_size;
        entry->uncompressedSize = z_entry_info.uncompressed_size;

        archive->entryList.Append(entry);
        archive->entryHash.Add(archive->entryHash.GenerateHash(entryFilename, false), i);
    
        unzGoToNextFile(z_file);
    }

    SearchPath *search = new SearchPath;
    search->archive = archive;
    search->next = searchPath;
    searchPath = search;
}

void FileSystem::Init(const char *baseDir) {
    SetBaseDir(baseDir);

    cmdSystem.AddCommand(L"dir", Cmd_Dir);
    cmdSystem.AddCommand(L"path", Cmd_Path);

    BE_LOG(L"Current search path:\n");
    for (SearchPath *s = searchPath; s; s = s->next) {
        if (s->archive) {
            BE_LOG(L"%hs (%i files)\n", s->archive->fullPath, s->archive->numEntries);
        } else {
            BE_LOG(L"%hs\n", s->pathname);
        }
    }
}

void FileSystem::Shutdown() {
    ClearSearchPath();
    
    cmdSystem.RemoveCommand(L"dir");
    cmdSystem.RemoveCommand(L"path");
}

void FileSystem::Restart(const char *baseDir) {
    if (!baseDir) {
        baseDir = WStr::ToStr(fs_baseDir.GetString());
    }
    
    Shutdown();
    Init(baseDir);
}

const char *FileSystem::GetBaseDir() const {
    return PlatformFile::GetBasePath();
}

void FileSystem::SetBaseDir(const char *baseDir) {
    fs_baseDir.SetString(Str::ToWStr(baseDir));
    
    PlatformFile::SetBasePath(baseDir);
}

int FileSystem::MakeFullPath(char *fullpath, int size, const char *path, const char *directory, const char *file) {
    fullpath[0] = 0;
    strcpy(fullpath, path);
    int len;

    if (directory && directory[0]) {
        len = Str::snPrintf(fullpath, size, "%s/%s", fullpath, directory);
        if (len == size) {
            BE_FATALERROR(L"FileSystem::MakeFullPath: not enough fullpath string data size");
        }
    }

    if (file && file[0]) {
        len = Str::snPrintf(fullpath, size, "%s/%s", fullpath, file);
        if (len == size) {
            BE_FATALERROR(L"FileSystem::MakeFullPath: not enough fullpath string data size");
        }
    }

    Str::ConvertPathSeperator(fullpath, PATHSEPERATOR_CHAR);	

    return len;
}

bool FileSystem::IsRelativePath(const char *path) {
    if (!path || path[0] == '/' || path[0] == '\\') {
        return false;
    }
    
    if (strlen(path) >= 2 && isalpha(path[0]) && path[1] == ':') {
        return false;
    }
    
    return true;
}

bool FileSystem::IsAbsolutePath(const char *path)  {
    if (!path) {
        return false;
    }
    
    return !IsRelativePath(path);
}

const Str FileSystem::ToRelativePath(const char *absolutePath) const {
    return Str(absolutePath).ToRelativePath(GetBaseDir());
}

const Str FileSystem::ToAbsolutePath(const char *relativePath) const {
    if (IsAbsolutePath(relativePath)) {
        return relativePath;
    }
    return Str(relativePath).ToAbsolutePath(GetBaseDir());
}

/*
-------------------------------------------------------------------------------
 
  File utility functions

-------------------------------------------------------------------------------
*/

bool FileSystem::RemoveFile(const char *filename, bool evenReadOnly) {
    bool fileExists = PlatformFile::FileExists(filename);
    if (!fileExists) {
        BE_LOG(L"Couldn't remove %hs because it doesn't exist\n", filename);
        return false;
    }
    
    if (evenReadOnly) {
        PlatformFile::SetReadOnly(filename, false);
    }
    
    if (!PlatformFile::RemoveFile(filename)) {
        BE_WARNLOG(L"Error occured removing file %hs\n", filename);
        return false;
    }
    
    return true;
}

bool FileSystem::MoveFile(const char *srcFilename, const char *dstFilename) {
    if (fs_debug.GetBool()) {
        BE_LOG(L"MoveFile: %hs --> %hs\n", srcFilename, dstFilename);
    }
    
    Str path = dstFilename;
    path.StripFileName();
    CreateDirectory(path, true);

    if (!PlatformFile::MoveFile(srcFilename, dstFilename)) {
        bool success = false;
        int retryCount = 3;
        
        while (retryCount--) {
            PlatformProcess::Sleep(0.5f);

            success = PlatformFile::MoveFile(srcFilename, dstFilename);
            if (success) {
                break;
            }
        }

        if (!success) {
            return false;
        }
    }

    return true;
}

bool FileSystem::CopyFile(const char *srcFilename, const char *dstFilename, ProgressCallback *progress) {
    if (progress && !progress->Poll(0.0f)) {
        return false;
    }

    if (fs_debug.GetBool()) {
        BE_LOG(L"CopyFile: %hs --> %hs\n", srcFilename, dstFilename);
    }

    size_t srcFileSize;
    File *srcFile = OpenFileRead(srcFilename, false, &srcFileSize);
    if (!srcFile /*|| srcFileSize < 1 */) {
        BE_WARNLOG(L"CopyFile: Failed to open src file '%hs'\n", srcFilename);
        return false;
    }
    
    Str path = dstFilename;
    path.StripFileName();
    path += PATHSEPERATOR_CHAR;
    CreateDirectory(path, true);

    File *dstFile = OpenFileWrite(dstFilename);
    if (!dstFile) {
        BE_WARNLOG(L"CopyFile: Failed to open dst file '%hs'\n", dstFilename);
        CloseFile(srcFile);
        return false;
    }

    static const size_t maxBufferSize = 1024 * 1024;
    size_t allocSize = Min(maxBufferSize, srcFileSize);
    byte *buffer = (byte *)Mem_Alloc(allocSize);
    bool complete = false;

    size_t remainSize = srcFileSize;
    while (remainSize > 0) {
        const size_t s = Min(allocSize, remainSize);

        if (!srcFile->Read(buffer, s)) {
            break;
        }
        if (!dstFile->Write(buffer, s)) {
            break;
        }

        remainSize -= s;

        if (progress) {
            float fraction = 1.0f - (float)remainSize / srcFileSize;
            if (!progress->Poll(fraction)) {
                break;
            }
        }
    }

    if (remainSize == 0) {
        complete = true;
    }

    Mem_Free(buffer);

    CloseFile(dstFile);
    CloseFile(srcFile);
    
    int mode = PlatformFile::GetFileMode(srcFilename);
    PlatformFile::SetFileMode(dstFilename, mode);
    
    return complete;
}

bool FileSystem::CopyDirectoryTree(const char *from, const char *to) {
    if (!DirectoryExists(from)) {
        return false;
    }

    CreateDirectory(to, false);

    FileArray fileArray;
    ListFiles(from, "*", fileArray, false, true);

    for (int i = 0; i < fileArray.NumFiles(); i++) {
        if (fileArray.GetArray()[i].isSubDir) {
            continue;
        }

        const auto &fileInfo = fileArray.GetArray()[i];

        Str srcFilename = from;
        srcFilename.AppendPath(fileInfo.relativePath);

        Str dstFilename = to;
        dstFilename.AppendPath(fileInfo.relativePath);

        if (!CopyFile(srcFilename, dstFilename)) {
            return false;
        }
    }

    return true;
}

bool FileSystem::CopyFileOrDirectoryTree(const char *from, const char *to) {
    if (DirectoryExists(from)) {
        return CopyDirectoryTree(from, to);
    }
    return CopyFile(from, to);
}

bool FileSystem::IsReadOnly(const char *filename) {
    return PlatformFile::IsReadOnly(filename);
}

bool FileSystem::SetReadOnly(const char *filename, bool readOnly) {
    return PlatformFile::SetReadOnly(filename, readOnly);
}

DateTime FileSystem::GetTimeStamp(const char *filename) const {
    return PlatformFile::GetTimeStamp(filename);
}

void FileSystem::SetTimeStamp(const char *filename, const DateTime &timeStamp) {
    PlatformFile::SetTimeStamp(filename, timeStamp);
}

//-------------------------------------------------------------------------------
//
// File system I/O
//
//-------------------------------------------------------------------------------

File *FileSystem::OpenFileRead(const char *filename, bool useSearchPath, size_t *fileSize) {
    if (!filename) {
        BE_FATALERROR(L"FileSystem::OpenFileRead: nullptr 'filename' parameter passed");
        return nullptr;
    }

    PlatformFile *pf = PlatformFile::OpenFileRead(filename);
    if (pf) {
        if (fs_debug.GetBool()) {
            BE_LOG(L"FileSystem::OpenFileRead: %hs\n", filename);
        }
        
        FileReal *file = new FileReal(filename, pf);
        if (fileSize) {
            file->size = *fileSize = pf->Size(); //PlatformFile::FileSize(filename);
        }

        return file;
    }
    
    if (!useSearchPath) {
        return nullptr;
    }
    
    File *resultFile = nullptr;
    
    for (SearchPath *s = searchPath; s; s = s->next) {
        if (s->archive) {
            ZipArchive *archive = s->archive;
            ZipEntry *entry;

            Str entryFilename = filename;
            Str::ConvertPathSeperator(entryFilename, PATHSEPERATOR_CHAR);

            int hash = archive->entryHash.GenerateHash(entryFilename, false);
            int	i;
            for (i = archive->entryHash.First(hash); i != -1; i = archive->entryHash.Next(i)) {
                if (!Str::Icmp(archive->entryList[i]->name, entryFilename)) {
                    entry = archive->entryList[i];
                    break;
                }
            }
                        
            if (i != -1) {
                if (fs_debug.GetBool()) {
                    BE_LOG(L"FileSystem::OpenFileRead: %hs (found in '%hs')\n", filename, archive->name);
                }

                unzSetOffset(archive->unzArchive, entry->unzOffset);
                unzOpenCurrentFile(archive->unzArchive);
                FileInZip *file = new FileInZip(filename, (void *)archive->unzArchive);

                if (fileSize) {
                    file->size = *fileSize = entry->uncompressedSize;
                }

                resultFile = file;
                break;
            }
        } else {
            Str relativePath = Str(s->pathname).ToRelativePath(Str(fs_baseDir.GetString()));
            relativePath.AppendPath(filename);
            relativePath.CleanPath();

            PlatformFile *pf = PlatformFile::OpenFileRead(relativePath);
            if (pf) {
                if (fs_debug.GetBool()) {
                    BE_LOG(L"FileSystem::OpenFileRead: %hs (found in '%hs')\n", relativePath.c_str(), s->pathname);
                }

                FileReal *file = new FileReal(relativePath, pf);
                
                if (fileSize) {
                    file->size = *fileSize = pf->Size(); // PlatformFile::FileSize(filename);
                }

                resultFile = file;
                break;
            }
        }
    }

    return resultFile;
}

File *FileSystem::OpenFileWrite(const char *filename) {
    if (fs_debug.GetBool()) {
        BE_LOG(L"FileSystem::OpenFileWrite: %hs\n", filename);
    }
    
    Str writeDir = filename;
    writeDir.StripFileName();
    if (!writeDir.IsEmpty()) {
        CreateDirectory(writeDir, true);
    }
    
    PlatformFile *pf = PlatformFile::OpenFileWrite(filename);
    if (!pf) {
        if (PlatformFile::FileExists(filename)) {
            int fileMode = PlatformFile::GetFileMode(filename);
            if (!(fileMode & PlatformFile::Writable)) {
                PlatformFile::SetFileMode(filename, fileMode | PlatformFile::Writable);
                pf = PlatformFile::OpenFileWrite(filename);
            }
        }
        
        if (!pf) {
            return nullptr;
        }
    }

    FileReal *file = new FileReal(filename, pf);
    return file;
}

File *FileSystem::OpenFileAppend(const char *filename) {
    if (fs_debug.GetBool()) {
        BE_LOG(L"FileSystem::OpenFileAppend: %hs\n", filename);
    }

    Str writeDir = filename;
    writeDir.StripFileName();
    if (!writeDir.IsEmpty()) {
        CreateDirectory(writeDir, true);
    }

    PlatformFile *pf = PlatformFile::OpenFileAppend(filename);
    if (!pf) {
        return nullptr;
    }

    FileReal *file = new FileReal(filename, pf);
    return file;
}

File *FileSystem::OpenFile(const char *filename, File::Mode mode, bool searchDirs) {
    switch (mode) {
    case File::ReadMode:
        return OpenFileRead(filename, searchDirs);
    case File::WriteMode:
        return OpenFileWrite(filename);
    case File::AppendMode:
        return OpenFileAppend(filename);
    }

    BE_FATALERROR(L"FileSystem::OpenFile: bad mode");
    return nullptr;
}

void FileSystem::CloseFile(File *file) {
    if (!file) {
        return;
    }

    delete file;
}

bool FileSystem::FileExists(const char *filename) const {
    return PlatformFile::FileExists(filename);
}

size_t FileSystem::FileSize(const char *filename) const {
    return PlatformFile::FileSize(filename);
}

bool FileSystem::DirectoryExists(const char *dirname) const {
    return PlatformFile::DirectoryExists(dirname);
}

bool FileSystem::CreateDirectory(const char *dirname, bool tree) const {
    if (DirectoryExists(dirname)) {
        return true;
    }
    
    if (tree) {
        return PlatformFile::CreateDirectoryTree(dirname);
    }
    
    return PlatformFile::CreateDirectory(dirname);
}

bool FileSystem::RemoveDirectory(const char *dirname, bool tree) const {
    if (tree) {
        if (!PlatformFile::RemoveDirectoryTree(dirname)) {
            BE_LOG(L"Failed to remove directory '%hs' recursively\n", dirname);
            return false;
        }
        
        return true;
    }

    return PlatformFile::RemoveDirectory(dirname) == 0 ? true : false;
}

//-------------------------------------------------------------------------------
//
//  Memory file I/O
//
//-------------------------------------------------------------------------------

size_t FileSystem::LoadFile(const char *path, bool searchDirs, void **buffer) {
    if (!path || !path[0]) {
        BE_ERRLOG(L"FileSystem::LoadFile: empty filename\n");
        if (buffer) {
            *buffer = nullptr;
        }
        return 0;
    }

    size_t size;
    File *file = OpenFileRead(path, searchDirs, &size);
    if (!file) {
        if (buffer) {
            *buffer = nullptr;
        }
        return 0;
    }
    
    if (!buffer) {
        CloseFile(file);
        return size;
    }

    *buffer = (byte *)Mem_Alloc(size + 1);

    file->Read(*buffer, size);
    (*(byte **)buffer)[size] = 0;

    CloseFile(file);

    return size;
}

void FileSystem::FreeFile(void *buffer) const {
    if (!buffer) {
        BE_FATALERROR(L"FileSystem::FreeFile: nullptr pointer");
        return;
    }
    
    Mem_Free(buffer);
}

void FileSystem::WriteFile(const char *filename, const void *buffer, int size) {
    if (!filename || !buffer) {
        BE_FATALERROR(L"FileSystem::WriteFile: nullptr parameter");
        return;
    }
    
    Str writeDir = filename;
    writeDir.StripFileName();
    if (!writeDir.IsEmpty()) {
        CreateDirectory(writeDir, true);
    }

    File *fp = OpenFile(filename, File::WriteMode);
    if (!fp) {
        BE_FATALERROR(L"FileSystem::WriteFile: failed to open %hs", filename);
        return;
    }

    fp->Write(buffer, size);

    CloseFile(fp);
}

bool FileSystem::ReadDict(const char *filename, Dict &dict) {
    char *text;
    size_t size = fileSystem.LoadFile(filename, false, (void **)&text);
    if (!text || !size) {
        return false;
    }

    Lexer lexer;
    lexer.Init(LexerFlag::LEXFL_NOERRORS);
    lexer.Load(text, Str::Length(text), va("%s", filename));
    
    dict.Parse(lexer);

    fileSystem.FreeFile(text);

    return true;
}

bool FileSystem::WriteDict(const char *filename, const Dict &dict) {
    File *fp = fileSystem.OpenFile(filename, File::WriteMode);
    if (!fp) {
        BE_WARNLOG(L"FileSystem::WriteDict: file open error\n");
        return false;
    }

    fp->Printf("{\n");

    for (int i = 0; i < dict.NumKeyValues(); i++) {
        const KeyValue *keyValue = dict.GetKeyValue(i);
        fp->Printf("  \"%s\" \"%s\"\n", keyValue->GetKey().c_str(), keyValue->GetValue().c_str());
    }

    fp->Printf("}\n");

    fileSystem.CloseFile(fp);

    return true;
}

int FileSystem::ListFiles(const char *findPath, const char *nameFilter, FileArray &fileArray, bool searchDirs, bool includeSubDir, bool sort, bool recursive) {
    Array<FileInfo> files;
    FileInfo        fileInfo;

    if (!nameFilter) {
        nameFilter = "*";
    }

    if (!searchDirs) {
        fileArray.array.Clear();
        fileArray.path = findPath;

        int numEntries = PlatformFile::ListFiles(findPath, nameFilter, recursive, includeSubDir, files);

        for (int i = 0; i < numEntries; i++) {
            fileArray.array.AddUnique(files[i]);
        }
    } else {
        fileArray.array.Clear();
        fileArray.path = findPath;

        int findPathLen = (int)strlen(findPath);

        for (SearchPath *s = searchPath; s; s = s->next) {
            if (s->archive) {
                ZipArchive *archive = s->archive;
                int numEntries = archive->numEntries;
            
                for (int i = 0; i < numEntries; i++) {
                    ZipEntry *entry = archive->entryList[i];

                    // check directory
                    if (Str::Icmpn(entry->name, findPath, findPathLen)) {
                        continue;
                    }
                
                    // check extension
                    if (Str::Filter(nameFilter, entry->name + findPathLen + 1)) {
                        continue;
                    }

                    char *name = entry->name + findPathLen + 1;

                    fileInfo.isSubDir = false;
                    //fileInfo.size = entry->size;
                    fileInfo.relativePath = name;

                    fileArray.array.AddUnique(fileInfo);
                }
            } else if (s->pathname) {
                char path[MaxAbsolutePath];
                MakeFullPath(path, sizeof(path), s->pathname, findPath, "");
                
                int numEntries = PlatformFile::ListFiles(path, nameFilter, recursive, includeSubDir, files);

                for (int i = 0; i < numEntries; i++) {
                    fileArray.array.AddUnique(files[i]);
                }
            }
        }
    }

    if (sort) {
        fileArray.array.Sort();
    }

    return fileArray.NumFiles();
}

Str FileSystem::GetDocumentDir() const {
    Str dir = PlatformSystem::UserDocumentDir();
    return dir;
}

Str FileSystem::GetAppDataDir(const char *org, const char *app) const {
    Str dir = PlatformSystem::UserAppDataDir();
    dir.AppendPath(org);
    dir.AppendPath(app);
    CreateDirectory(dir, true);
    return dir;
}

//--------------------------------------------------------------------------------------------------

void FileSystem::Cmd_Dir(const CmdArgs &args) {
    int argc = args.Argc();
    
    if (argc < 2 || argc > 3) {
        BE_LOG(L"usage: dir <directory> [namefilter]\n");
        return;
    }

    char directory[1024];
    Str::Copynz(directory, tombs(args.Argv(1)), COUNT_OF(directory));

    char nameFilter[256] = "";
    if (argc > 2) {
        Str::Copynz(nameFilter, tombs(args.Argv(2)), COUNT_OF(nameFilter));
    }

    FileArray fileArray;
    fileSystem.ListFiles(directory, nameFilter, fileArray, true);

    BE_LOG(L"Directory of %hs %hs\n", directory, nameFilter);

    for (int i = 0; i < fileArray.NumFiles(); i++) {
        BE_LOG(L"%hs\n", fileArray.GetFilename(i));
    }
    
    BE_LOG(L"%i files found\n", fileArray.NumFiles());
}

void FileSystem::Cmd_Path(const CmdArgs &args) {
    SearchPath *s;
    
    BE_LOG(L"Current search path:\n");
    for (s = fileSystem.searchPath; s; s = s->next) {
        if (s->archive) {
            BE_LOG(L"%hs (%i files)\n", s->archive->fullPath, s->archive->numEntries);
        } else {
            BE_LOG(L"%hs\n", s->pathname);
        }
    }
}

BE_NAMESPACE_END
