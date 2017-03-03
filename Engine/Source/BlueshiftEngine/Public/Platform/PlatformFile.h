#pragma once

#include "Containers/Array.h"
#include "Core/DateTime.h"

BE_NAMESPACE_BEGIN

#undef MoveFile
#undef CopyFile
#undef CreateDirectory
#undef RemoveDirectory

struct BE_API FileInfo {
    bool                    operator==(const FileInfo &rhs) const { return (isSubDir == rhs.isSubDir && relativePath.IcmpPath(rhs.relativePath) == 0); }

    bool                    operator<(const FileInfo &rhs) const { return relativePath.IcmpPath(rhs.relativePath) < 0; }
        
    bool                    isSubDir;
    Str                     relativePath;
};

class BE_API PlatformBaseFile {
public:
    enum Origin {
        Start, Current, End
    };
    
    virtual ~PlatformBaseFile() = 0;

                            /// Returns offset in file.
    virtual int             Tell() const = 0;
                            /// Returns file size
    virtual int             Size() const = 0;
                            /// Seek from the start on a file.
    virtual int             Seek(long offset, Origin origin) = 0;
                            /// Reads data from the file to the buffer.
    virtual size_t          Read(void *buffer, size_t bytesToRead) const = 0;
                            /// Writes data from the buffer to the file.
    virtual bool            Write(const void *buffer, size_t bytesToWrite) = 0;

    static const char *     GetBasePath();
    static void             SetBasePath(const char *basePath);
    static Str              NormalizeFilename(const char *filename);
    static Str              NormalizeDirectoryName(const char *dirname);
    
    static PlatformBaseFile *OpenFileRead(const char *filename);
    static PlatformBaseFile *OpenFileWrite(const char *filename);
    static PlatformBaseFile *OpenFileAppend(const char *filename);
    
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
    static bool             CreateDirectoryTree(const char *dirname);
    static bool             RemoveDirectory(const char *dirname);
    static bool             RemoveDirectoryTree(const char *dirname);
    
    static const char *     Cwd();
    static bool             SetCwd(const char *dirname);
    static const char *     ExecutablePath();
    static const char *     HomePath();
    
    static int              ListFiles(const char *directory, const char *nameFilter, bool recursive, bool includeSubDir, Array<FileInfo> &files);

protected:
    static Str              basePath;
};

BE_NAMESPACE_END

#ifdef __WIN32__
#include "Windows/PlatformWinFile.h"
#elif defined(__IOS__)
#include "IOS/PlatformIOSFile.h"
#elif defined(__ANDROID__)
#include "Android/PlatformAndroidFile.h"
#elif defined(__UNIX__)
#include "Posix/PlatformPosixFile.h"
#endif

