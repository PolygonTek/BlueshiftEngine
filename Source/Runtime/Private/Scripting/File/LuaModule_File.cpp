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
#include "Scripting/LuaVM.h"
#include "File/File.h"

BE_NAMESPACE_BEGIN

struct FileMode {};

void LuaVM::RegisterFile(LuaCpp::Module &module) {
    LuaCpp::Selector _File = module["File"];

    _File.SetClass<File>();
    _File.AddClassCtor<File>();
    _File.AddClassMembers<File>(
        "size", &File::Size,
        "tell", &File::Tell,
        "seek", &File::Seek,
        "seek_from_end", &File::SeekFromEnd,
        "read", &File::Read,
        "read_char", &File::ReadChar,
        "read_uchar", &File::ReadUChar,
        "read_int16", &File::ReadInt16,
        "read_uint16", &File::ReadUInt16,
        "read_int32", &File::ReadInt32,
        "read_uint32", &File::ReadUInt32, 
        "read_int64", &File::ReadInt64,
        "read_uint64", &File::ReadUInt64,
        "read_float", &File::ReadFloat,
        "read_double", &File::ReadDouble,
        "read_string", &File::ReadString,
        "write", &File::Write,
        "write_char", &File::WriteChar,
        "write_uchar", &File::WriteUChar,
        "write_int16", &File::WriteInt16,
        "write_uint16", &File::WriteUInt16,
        "write_int32", &File::WriteInt32,
        "write_uint32", &File::WriteUInt32,
        "write_int64", &File::WriteInt64,
        "write_uint64", &File::WriteUInt64,
        "write_float", &File::WriteFloat,
        "write_double", &File::WriteDouble,
        "write_string", &File::WriteString);

    LuaCpp::Selector _File_Mode = _File["Mode"];
    _File_Mode.SetClass<FileMode>();
    _File_Mode["ReadMode"] = File::ReadMode;
    _File_Mode["WriteMode"] = File::WriteMode;
    _File_Mode["AppendMode"] = File::AppendMode;
}

BE_NAMESPACE_END
