#pragma once

BE_NAMESPACE_BEGIN

class BE_API ZipArchiver {
public:
    enum CompressionLevel {
        NoCompression,
        BestCompression,
        BestSpeed,
        DefaultCompression
    };

    ZipArchiver();
    ZipArchiver(const char *filename);
    ~ZipArchiver();

    bool            Open(const char *filename);
    void            Close();

    bool            AddFile(const char *filename, CompressionLevel compressionLevel = DefaultCompression);

    static bool     Archive(const char *zipFilename, const char *archiveDirectory, const char *filter, const char *baseDir = "", ProgressCallback *progress = nullptr);

private:
    void *          zFile;
};

BE_INLINE ZipArchiver::ZipArchiver() {
    zFile = nullptr;
}

BE_INLINE ZipArchiver::ZipArchiver(const char *filename) {
    Open(filename);
}

BE_INLINE ZipArchiver::~ZipArchiver() {
    Close();
}

BE_NAMESPACE_END
