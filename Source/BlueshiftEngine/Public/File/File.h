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

#include "Platform/PlatformFile.h"

BE_NAMESPACE_BEGIN

class Guid;
class Object;

class BE_API File {
    friend class FileSystem;

public:
    enum Mode {
        ReadMode,
        WriteMode,
        AppendMode
    };
    
    virtual ~File() = 0;

    virtual const char *    GetFilePath() const = 0;

                            /// Returns size of a file
    virtual size_t          Size() const = 0;
                            /// Returns offset in file.
    virtual int             Tell() const = 0;
                            /// Seek from the start on a file.
    virtual int             Seek(int64_t offset) = 0;
                            /// Seek from the end on a file
    virtual int             SeekFromEnd(int64_t offset) = 0;
    
                            /// Read data from the file to the buffer.
    virtual size_t          Read(void *buffer, size_t bytesToRead) const = 0;
                            /// Write data from the buffer to the file.
    virtual bool            Write(const void *buffer, size_t bytesToWrite) = 0;
                            /// Like fprintf
    virtual bool            Printf(const char *format, ...);
                            /// Like fprintf with wchar_t
    virtual bool            Printf(const wchar_t *format, ...);

    size_t                  ReadChar(char &value);
    size_t                  ReadUChar(unsigned char &value);
    size_t                  ReadInt16(int16_t &value);
    size_t                  ReadUInt16(uint16_t &value);
    size_t                  ReadInt32(int32_t &value);
    size_t                  ReadUInt32(uint32_t &value);
    size_t                  ReadInt64(int64_t &value);
    size_t                  ReadUInt64(uint64_t &value);
    size_t                  ReadFloat(float &value);
    size_t                  ReadDouble(double &value);
    size_t                  ReadString(Str &value);
    size_t                  ReadGuid(Guid &guid);
    size_t                  ReadObject(Object &object);

    size_t                  WriteChar(const char value);
    size_t                  WriteUChar(const unsigned char value);
    size_t                  WriteInt16(const int16_t value);
    size_t                  WriteUInt16(const uint16_t value);
    size_t                  WriteInt32(const int32_t value);
    size_t                  WriteUInt32(const uint32_t value);
    size_t                  WriteInt64(const int64_t value);
    size_t                  WriteUInt64(const uint64_t value);
    size_t                  WriteFloat(const float value);
    size_t                  WriteDouble(const double value);
    size_t                  WriteString(const char *value);
    size_t                  WriteGuid(const Guid &guid);
    size_t                  WriteObject(const Object &object);
};

class BE_API FileReal : public File {
    friend class FileSystem;

public:
    FileReal(const char *filename, PlatformFile *pf);
    virtual ~FileReal();
    
    virtual const char *    GetFilePath() const override { return filename; }
    
                            /// Returns size of a file 
    virtual size_t          Size() const override;
                            /// Returns offset in file.
    virtual int             Tell() const override;
                            /// Seek from the start on a file.
    virtual int             Seek(int64_t offset) override;
                            /// Seek from the end on a file
    virtual int             SeekFromEnd(int64_t offset) override;
    
                            /// Read data from the file to the buffer.
    virtual size_t          Read(void *buffer, size_t bytesToRead) const override;
                            /// Write data from the buffer to the file.
    virtual bool            Write(const void *buffer, size_t bytesToWrite) override;
    
protected:
    char                    filename[MaxAbsolutePath];
    PlatformFile *          pf;
    size_t                  size;
};

class BE_API FileInZip : public File {
    friend class FileSystem;
    
public:
    FileInZip(const char *filename, void *pointer);
    virtual ~FileInZip();
    
    virtual const char *    GetFilePath() const override { return filename; }
    
    virtual size_t          Size() const override;
                            /// Returns offset in file.
    virtual int             Tell() const override;
                            /// Seek from the start on a file.
    virtual int             Seek(int64_t offset) override;
                            /// Seek from the end on a file
    virtual int             SeekFromEnd(int64_t offset) override;
    
                            /// Read data from the file to the buffer.
    virtual size_t          Read(void *buffer, size_t bytesToRead) const override;
                            /// Write data from the buffer to the file.
    virtual bool            Write(const void *buffer, size_t bytesToWrite) override;
    
protected:
    char                    filename[MaxAbsolutePath];
    void *                  pointer;
    size_t                  size;
};

BE_NAMESPACE_END
