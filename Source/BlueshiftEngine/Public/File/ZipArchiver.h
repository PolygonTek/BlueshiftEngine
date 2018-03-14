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
