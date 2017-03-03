#pragma once

#define USE_BASE_PLATFORM_POSIX_FILE
#include "../Posix/PlatformPosixFile.h"

BE_NAMESPACE_BEGIN

class BE_API PlatformIOSFile : public PlatformPosixFile {
public:
                            PlatformIOSFile(FILE *fp);
    virtual                 ~PlatformIOSFile();
    
    static Str              NormalizeFilename(const char *filename);
    static Str              NormalizeDirectoryName(const char *dirname);
    
    static PlatformIOSFile *OpenFileRead(const char *filename);
    static PlatformIOSFile *OpenFileWrite(const char *filename);
    static PlatformIOSFile *OpenFileAppend(const char *filename);
    
    static bool             FileExists(const char *filename);
    static size_t           FileSize(const char *filename);
    static bool             IsFileWritable(const char *filename);
    static bool             IsReadOnly(const char *filename);
    static bool             RemoveFile(const char *filename);
    static bool             MoveFile(const char *srcFilename, const char *dstFilename);

    static const char *     ExecutablePath();
    static const char *     HomePath();
    
    static Str              ConvertToIOSPath(const Str &filename, bool forWrite);
};

typedef PlatformIOSFile     PlatformFile;

BE_NAMESPACE_END