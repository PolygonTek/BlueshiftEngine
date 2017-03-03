#pragma once

BE_NAMESPACE_BEGIN

class BE_API PlatformWinFile : public PlatformBaseFile {
public:
    PlatformWinFile(HANDLE fileHandle);
    virtual ~PlatformWinFile();
                            /// Returns offset in file.
    virtual int             Tell() const;
                            /// Returns file size.
    virtual int             Size() const;
                            /// Seek from the start on a file.
    virtual int             Seek(long offset, Origin origin);
    
                            /// Reads data from the file to the buffer.
    virtual size_t          Read(void *buffer, size_t bytesToRead) const;
                            /// Writes data from the buffer to the file.
    virtual bool            Write(const void *buffer, size_t bytesToWrite);
    
    static Str              NormalizeFilename(const char *filename);
    static Str              NormalizeDirectory(const char *dirname);
    
    static PlatformWinFile *OpenFileRead(const char *filename);
    static PlatformWinFile *OpenFileWrite(const char *filename);
    static PlatformWinFile *OpenFileAppend(const char *filename);
    
    static bool             FileExists(const char *filename);
    static size_t           FileSize(const char *filename);
    static bool             IsFileWritable(const char *filename);
    static bool             IsReadOnly(const char *filename);
    static bool             SetReadOnly(const char *filename, bool readOnly);
    static bool             RemoveFile(const char *filename);
    static bool             MoveFile(const char *oldname, const char *newname);
    
    static DateTime         GetTimeStamp(const char *filename);
    static void             SetTimeStamp(const char *filename, const DateTime &timeStamp);
    
    static bool             DirectoryExists(const char *dirname);
    static bool             CreateDirectory(const char *dirname);
    static bool             RemoveDirectory(const char *dirname);
    static bool             RemoveDirectoryTree(const char *dirname);

    static const char *     Cwd();
    static bool             SetCwd(const char *dirname);
    static const char *     ExecutablePath();
    static const char *     HomePath();
    
    static int              ListFiles(const char *directory, const char *nameFilter, bool recursive, bool includeSubDir, Array<FileInfo> &list);
    
protected:
    HANDLE                  fileHandle;
};

typedef PlatformWinFile     PlatformFile;

BE_NAMESPACE_END
