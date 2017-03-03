#include "Precompiled.h"
#include "Core/Str.h"
#include "Platform/PlatformFile.h"

BE_NAMESPACE_BEGIN

Str PlatformBaseFile::basePath;

PlatformBaseFile::~PlatformBaseFile() {
}

const char *PlatformBaseFile::GetBasePath() {
    return basePath;
}

void PlatformBaseFile::SetBasePath(const char *basePath) {
    PlatformBaseFile::basePath = basePath;
    if (!PlatformBaseFile::basePath.IsEmpty() &&  PlatformBaseFile::basePath[PlatformBaseFile::basePath.Length() - 1] != '/') {
        PlatformBaseFile::basePath.Append('/');
    }
}

Str PlatformBaseFile::NormalizeFilename(const char *filename) {
    return "";
}

Str PlatformBaseFile::NormalizeDirectoryName(const char *dirname) {
    return "";
}

PlatformBaseFile *PlatformBaseFile::OpenFileRead(const char *filename) {
    return nullptr;
}

PlatformBaseFile *PlatformBaseFile::OpenFileWrite(const char *filename) {
    return nullptr;
}

PlatformBaseFile *PlatformBaseFile::OpenFileAppend(const char *filename) {
    return nullptr;
}

bool PlatformBaseFile::FileExists(const char *filename) {
    return false;
}

size_t PlatformBaseFile::FileSize(const char *filename) {
    return 0;
}

bool PlatformBaseFile::IsFileWritable(const char *filename) {
    return false;
}

bool PlatformBaseFile::IsReadOnly(const char *filename) {
    return false;
}

bool PlatformBaseFile::SetReadOnly(const char *filename, bool readOnly) {
    return false;
}

bool PlatformBaseFile::RemoveFile(const char *filename) {
    return false;
}

bool PlatformBaseFile::MoveFile(const char *srcFilename, const char *dstFilename) {
    return false;
}

DateTime GetTimeStamp(const char *filename) {
    return DateTime(0);
}

void PlatformBaseFile::SetTimeStamp(const char *filename, const DateTime &timeStamp) {
}

bool PlatformBaseFile::DirectoryExists(const char *dirname) {
    return false;
}

bool PlatformBaseFile::CreateDirectory(const char *dirname) {
    return false;
}

bool PlatformBaseFile::CreateDirectoryTree(const char *dirname) {
    int i;

    Str path = dirname;
    path.CleanPath(PATHSEPERATOR_CHAR);
        
    if (PlatformFile::CreateDirectory(path)) {
        return true;
    }

    for (i = path.Length(); i > 0;) {
        if (path[--i] == PATHSEPERATOR_CHAR) {
            path[i] = 0;
            if (PlatformFile::CreateDirectory(path)) {
                path[i++] = PATHSEPERATOR_CHAR;
                break;
            }
            path[i] = PATHSEPERATOR_CHAR;
        }
    }
    for (; path[i]; i++) {
        if (path[i] == PATHSEPERATOR_CHAR) {
            path[i] = 0;

            if (!PlatformFile::CreateDirectory(path)) {
                return false;
            }
            path[i] = PATHSEPERATOR_CHAR;

        }
    }
    return PlatformFile::CreateDirectory(path);
}

bool PlatformBaseFile::RemoveDirectory(const char *dirname) {
    return false;
}

bool PlatformBaseFile::RemoveDirectoryTree(const char *dirname) {
    return false;
}

const char *PlatformBaseFile::Cwd() {
    return "";
}

bool PlatformBaseFile::SetCwd(const char *dirname) {
    return false;
}

const char *PlatformBaseFile::ExecutablePath() {
    return "";
}

const char *PlatformBaseFile::HomePath() {
    return "";
}

int PlatformBaseFile::ListFiles(const char *directory, const char *nameFilter, bool recursive, bool includeSubDir, Array<FileInfo> &files) {
    files.Clear();
    return 0;
}



BE_NAMESPACE_END
