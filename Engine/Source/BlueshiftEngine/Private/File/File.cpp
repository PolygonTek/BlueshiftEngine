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
#include "Core/ByteOrder.h"
#include "Core/Guid.h"
#include "Core/Object.h"
#include "File/File.h"
#include "minizip/unzip.h"

BE_NAMESPACE_BEGIN

//---------------------------------------------------------------
// File
//---------------------------------------------------------------

File::~File() {
}

bool File::Printf(const char *format, ...) {
    char buffer[4096];
    va_list	args;
    
    va_start(args, format);
    ::vsprintf(buffer, format, args);
    va_end(args);
    
    return Write(buffer, (int)strlen(buffer));
}

bool File::Printf(const wchar_t *format, ...) {
    wchar_t buffer[4096];
    va_list	args;
    
    va_start(args, format);
    ::vswprintf(buffer, COUNT_OF(buffer), format, args);
    va_end(args);
    
    return Write(buffer, (int)wcslen(buffer) * sizeof(wchar_t));
}

size_t File::ReadChar(char &value) {
    return Read(&value, sizeof(value));
}

size_t File::ReadUChar(unsigned char &value) {
    return Read(&value, sizeof(value));
}

size_t File::ReadInt16(int16_t &value) {
    size_t result = Read(&value, sizeof(value));
    ByteOrder::LittleEndianToSystem(value);
    return result;
}

size_t File::ReadUInt16(uint16_t &value) {
    size_t result = Read(&value, sizeof(value));
    ByteOrder::LittleEndianToSystem(value);
    return result;
}

size_t File::ReadInt32(int32_t &value) {
    size_t result = Read(&value, sizeof(value));
    ByteOrder::LittleEndianToSystem(value);
    return result;
}

size_t File::ReadUInt32(uint32_t &value) {
    size_t result = Read(&value, sizeof(value));
    ByteOrder::LittleEndianToSystem(value);
    return result;
}

size_t File::ReadInt64(int64_t &value) {
    size_t result = Read(&value, sizeof(value));
    ByteOrder::LittleEndianToSystem(value);
    return result;
}

size_t File::ReadUInt64(uint64_t &value) {
    size_t result = Read(&value, sizeof(value));
    ByteOrder::LittleEndianToSystem(value);
    return result;
}

size_t File::ReadFloat(float &value) { 
    size_t result = Read(&value, sizeof(value));
    ByteOrder::LittleEndianToSystem(value);
    return result;
}

size_t File::ReadDouble(double &value) {
    size_t result = Read(&value, sizeof(value));
    ByteOrder::LittleEndianToSystem(value);
    return result;
}

size_t File::ReadString(Str &value) {
    size_t result = 0;
    int32_t len;

    ReadInt32(len);
    if (len >= 0) {
        value.Fill(' ', len);
        result = Read(&value[0], len);
    }

    return result;
}

size_t File::ReadGuid(Guid &value) {
    uint32_t a, b, c, d;
    size_t result = ReadUInt32(a);
    result += ReadUInt32(b);
    result += ReadUInt32(c);
    result += ReadUInt32(d);
    value.Set(a, b, c, d);
    return result;
}

size_t File::ReadObject(Object &object) {
    Str strValue;
    size_t size = ReadString(strValue);

    Json::Value jsonValue;
    Json::Reader jsonReader;
    if (!jsonReader.parse(strValue.c_str(), jsonValue)) {
        BE_WARNLOG(L"File::ReadObject: Failed to parse JSON text\n");
        return 0;
    }

    object.Deserialize(jsonValue);

    return size;
}

size_t File::WriteChar(const char value) { 
    return Write(&value, sizeof(value));
}

size_t File::WriteUChar(const unsigned char value) {
    return Write(&value, sizeof(value));
}

size_t File::WriteInt16(const int16_t value) { 
    int16_t v = value;
    ByteOrder::SystemToLittleEndian(value);
    return Write(&v, sizeof(v));
}

size_t File::WriteUInt16(const uint16_t value) {
    int16_t v = value;
    ByteOrder::SystemToLittleEndian(value);
    return Write(&v, sizeof(v));
}

size_t File::WriteInt32(const int32_t value) { 
    int32_t v = value;
    ByteOrder::SystemToLittleEndian(value);
    return Write(&v, sizeof(v));
}

size_t File::WriteUInt32(const uint32_t value) {
    uint32_t v = value;
    ByteOrder::SystemToLittleEndian(value);
    return Write(&v, sizeof(v));
}

size_t File::WriteInt64(const int64_t value) { 
    int64_t v = value;
    ByteOrder::SystemToLittleEndian(value);
    return Write(&v, sizeof(v));
}

size_t File::WriteUInt64(const uint64_t value) {
    uint64_t v = value;
    ByteOrder::SystemToLittleEndian(value);
    return Write(&v, sizeof(v));
}

size_t File::WriteFloat(const float value) { 
    float v = value;
    ByteOrder::SystemToLittleEndian(value);
    return Write(&v, sizeof(v));
}

size_t File::WriteDouble(const double value) {
    double v = value;
    ByteOrder::SystemToLittleEndian(value);
    return Write(&v, sizeof(v));
}

size_t File::WriteString(const char *value) { 
    int32_t len = (int32_t)strlen(value);
    WriteInt32(len);
    return Write(value, len);
}

size_t File::WriteGuid(const Guid &value) {
    size_t result = WriteUInt32(value[0]);
    result += WriteUInt32(value[1]);
    result += WriteUInt32(value[2]);
    result += WriteUInt32(value[3]);
    return result;
}

size_t File::WriteObject(const Object &object) {
    Json::Value jsonValue;
    object.Serialize(jsonValue);

    Json::FastWriter jsonWriter;
    Str jsonText = jsonWriter.write(jsonValue).c_str();

    return WriteString(jsonText);
}

//---------------------------------------------------------------
// FileReal
//---------------------------------------------------------------

FileReal::FileReal(const char *filename, PlatformFile *pf) {
    Str::Copynz(this->filename, filename, COUNT_OF(this->filename));
    this->pf = pf;
}

FileReal::~FileReal() {
    SAFE_DELETE(pf);
}

size_t FileReal::Size() const {
    return size;
}

int FileReal::Tell() const {
    if (!pf) {
        return 0;
    }
    return pf->Tell();
}

int FileReal::Seek(int64_t offset) {
    if (!pf) {
        return 0;
    }

    return pf->Seek(offset, PlatformFile::Start);
}

int FileReal::SeekFromEnd(int64_t offset) {
    if (!pf) {
        return 0;
    }
    
    return pf->Seek(offset, PlatformFile::End);
}

size_t FileReal::Read(void *buffer, size_t bytesToRead) const {
    if (!pf) {
        return 0;
    }

    return pf->Read(buffer, bytesToRead);
}

bool FileReal::Write(const void *buffer, size_t bytesToWrite) {
    if (!pf) {
        return false;
    }
    
    return pf->Write(buffer, bytesToWrite);
}

//---------------------------------------------------------------
// FileInZip
//---------------------------------------------------------------

FileInZip::FileInZip(const char *filename, void *pointer) {
    Str::Copynz(this->filename, filename, COUNT_OF(this->filename));
    this->pointer = pointer;
}

FileInZip::~FileInZip() {
    unzCloseCurrentFile(pointer);
}

size_t FileInZip::Size() const {
    return size;
}

int FileInZip::Tell() const {
    return (int)unztell(pointer);
}

int FileInZip::Seek(int64_t offset) {
    BE_FATALERROR(L"ZIP FILE FSEEK NOT YET IMPLEMENTED");
    return 0;
}

int FileInZip::SeekFromEnd(int64_t offset) {
    BE_FATALERROR(L"ZIP FILE FSEEK NOT YET IMPLEMENTED");
    return 0;
}

size_t FileInZip::Read(void *buffer, size_t bytesToRead) const {
    return unzReadCurrentFile(pointer, buffer, (unsigned int)bytesToRead);
}

bool FileInZip::Write(const void *buffer, size_t len) {
    BE_FATALERROR(L"ZIP FILE WRITE IS NOT ALLOWED");
    return false;
}

BE_NAMESPACE_END
