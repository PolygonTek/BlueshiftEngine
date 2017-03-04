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
#include "File/ZipArchiver.h"
#include "minizip/zip.h"

BE_NAMESPACE_BEGIN

#define WRITEBUFFERSIZE (16384)
#define MAXFILENAME (256)

static int ToZCompressLevel(ZipArchiver::CompressionLevel compressionLevel) {
    int ret;

    switch (compressionLevel) {
    case ZipArchiver::NoCompression:
        ret = Z_DEFAULT_COMPRESSION;
        break;
    case ZipArchiver::BestCompression:
        ret = Z_BEST_COMPRESSION;
        break;
    case ZipArchiver::BestSpeed:
        ret = Z_BEST_SPEED;
        break;
    case ZipArchiver::DefaultCompression:
    default:
        ret = Z_DEFAULT_COMPRESSION;
    }

    return ret;
}

bool ZipArchiver::Open(const char *filename) {
    Str zipFilename = fileSystem.ToAbsolutePath(filename);
    zFile = (void *)zipOpen64((const char *)zipFilename, APPEND_STATUS_CREATE);
    if (!zFile) {
        BE_WARNLOG(L"Unabled to open zip file to create '%hs'\n", zipFilename.c_str());
        return false;
    }

    return true;
}

void ZipArchiver::Close() {
    if (zFile) {
        zipClose(zFile, nullptr);
        zFile = nullptr;
    }
}

bool ZipArchiver::AddFile(const char *filename, CompressionLevel compressionLevel) {
    zip_fileinfo zi;
    memset(&zi, 0, sizeof(zi));

    DateTime timeStamp = fileSystem.GetTimeStamp(filename);
    zi.tmz_date.tm_sec = timeStamp.Second();
    zi.tmz_date.tm_min = timeStamp.Minute();
    zi.tmz_date.tm_hour = timeStamp.Hour();
    zi.tmz_date.tm_mday = timeStamp.Day();
    zi.tmz_date.tm_year = timeStamp.Year();

    int opt_compress_level = ToZCompressLevel(compressionLevel);
    bool largeFile = false;

    int err = zipOpenNewFileInZip3_64(zFile, filename, &zi,
        nullptr, 0, nullptr, 0, nullptr, (opt_compress_level != 0) ? Z_DEFLATED : 0, opt_compress_level, 0,
        -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY,
        nullptr, 0, largeFile ? 1 : 0);
    if (err != ZIP_OK) {
        BE_WARNLOG(L"Failed to open '%hs' in zipfile\n", filename);
        return false;
    }

    size_t srcFileSize;
    File *srcFile = fileSystem.OpenFileRead(filename, false, &srcFileSize);
    if (!srcFile) {
        BE_WARNLOG(L"Failed to open '%hs'\n", filename);
        zipCloseFileInZip(zFile);
        return false;
    }

    size_t allocSize = Min((size_t)WRITEBUFFERSIZE, srcFileSize);
    byte *buffer = (byte *)_alloca16(allocSize);
    size_t remainSize = srcFileSize;

    while (remainSize > 0) {
        size_t s = Min(allocSize, remainSize);
        srcFile->Read(buffer, s);

        err = zipWriteInFileInZip(zFile, buffer, (unsigned int)s);
        if (err != ZIP_OK) {
            BE_WARNLOG(L"Error in writing '%hs' in zipfile\n", filename);
            zipCloseFileInZip(zFile);
            return false;
        }

        remainSize -= s;
    }

    fileSystem.CloseFile(srcFile);

    err = zipCloseFileInZip(zFile);
    if (err != ZIP_OK) {
        BE_WARNLOG(L"Error in closing '%hs' in zipfile\n", filename);
        return false;
    }

    return true;
}

bool ZipArchiver::Archive(const char *zipFilename, const char *archiveDirectory, const char *filter, const char *baseDir, ProgressCallback *progress) {
    if (progress && !progress->Poll(0.0f)) {
        return false;
    }

    Str oldBaseDir = fileSystem.GetBaseDir();

    Str zipFilename2 = zipFilename;
    Str::ConvertPathSeperator(zipFilename2, PATHSEPERATOR_CHAR);
    ZipArchiver zipFile;
    if (!zipFile.Open(zipFilename2)) {
        return false;
    }

    if (baseDir && baseDir[0]) {
        fileSystem.SetBaseDir(baseDir);
    }

    FileArray fileArray;
    fileSystem.ListFiles(archiveDirectory, filter, fileArray, false, true);

    for (int i = 0; i < fileArray.NumFiles(); i++) {
        const auto &fileInfo = fileArray.GetArray()[i];
        if (fileInfo.isSubDir) {
            continue;
        }

        Str filename = archiveDirectory;
        filename.AppendPath(fileInfo.relativePath);
        zipFile.AddFile(filename);

        if (progress) {
            float fraction = float(i + 1) / fileArray.NumFiles();
            if (!progress->Poll(fraction)) {
                break;
            }
        }
    }

    zipFile.Close();

    if (baseDir && baseDir[0]) {
        fileSystem.SetBaseDir(oldBaseDir);
    }

    return true;
}

BE_NAMESPACE_END