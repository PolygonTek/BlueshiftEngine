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
#include "Core/Str.h"
#include "Math/Math.h"
#include "File/FileSystem.h"
#include "Image/Image.h"
#include "ImageInternal.h"

BE_NAMESPACE_BEGIN

bool Image::Load(const char *filename) {
    if (!filename || filename[0] == 0) {
        return false;
    }

    Str name = filename;

    byte *data;
    size_t size = fileSystem.LoadFile(name, true, (void **)&data);
    if (data) {
        // 확장자에 맞춰서 로딩함수 call
        if (name.CheckExtension(".btex")) {
            //LoadBTexFromMemory(name, data, size);
        } else if (name.CheckExtension(".dds")) {
            LoadDDSFromMemory(name, data, size);
        } else if (name.CheckExtension(".pvr")) {
            LoadPVRFromMemory(name, data, size);
        } else if (name.CheckExtension(".tga")) {
            LoadTGAFromMemory(name, data, size);
        } else if (name.CheckExtension(".jpg")) {
            LoadJPGFromMemory(name, data, size);
        } else if (name.CheckExtension(".png")) {
            LoadPNGFromMemory(name, data, size);
        } else if (name.CheckExtension(".bmp")) {
            LoadBMPFromMemory(name, data, size);
        } else if (name.CheckExtension(".pcx")) {
            LoadPCXFromMemory(name, data, size);
        } else if (name.CheckExtension(".hdr")) {
            LoadHDRFromMemory(name, data, size);
        }

        fileSystem.FreeFile(data);

        if (pic) {
            return true;
        }
    }
   
#if 0
    static const char *extensions[] = { ".dds", ".pvr", ".tga", ".jpg", ".png", ".bmp", ".pcx", ".hdr" };

    // 못 읽었다면 확장자를 붙여가면서 읽어본다.
    for (int i = 0; i < COUNT_OF(extensions); i++) {
        Str name2 = name + extensions[i];

        size_t size = fileSystem.LoadFile(name2, true, (void **)&data);
        if (data) {
            if (name2.CheckExtension(".dds")) {
                LoadDDSFromMemory(name2, data, size);
            } else if (name2.CheckExtension(".pvr")) {
                LoadPVRFromMemory(name2, data, size);
            } else if (name2.CheckExtension(".tga")) {
                LoadTGAFromMemory(name2, data, size);
            } else if (name2.CheckExtension(".jpg")) {
                LoadJPGFromMemory(name2, data, size);
            } else if (name2.CheckExtension(".png")) {
                LoadPNGFromMemory(name2, data, size);
            } else if (name2.CheckExtension(".bmp")) {
                LoadBMPFromMemory(name2, data, size);
            } else if (name2.CheckExtension(".pcx")) {
                LoadPCXFromMemory(name2, data, size);
            } else if (name2.CheckExtension(".hdr")) {
                LoadHDRFromMemory(name2, data, size);
            }

            fileSystem.FreeFile(data);

            if (pic) {
                return true;
            }
        }
    }
#endif

    return false;
}

bool Image::Write(const char *filename) const {
    if (!filename || filename[0] == 0) {
        return false;
    }

    bool ret = false;
    //std::string s = filename;
    //std::string ext = s.substr(s.find_last_of("."));
    Str name = filename;
    Str extension;
    name.ExtractFileExtension(extension);

    if (!extension.Icmp("dds")) {
        ret = WriteDDS(filename);
    } else if (!extension.Icmp("pvr")) {
        ret = WritePVR(filename);
    } else if (!extension.Icmp("tga")) {
        ret = WriteTGA(filename);
    } else if (!extension.Icmp("jpg")) {
        ret = WriteJPG(filename, 100);
    } else if (!extension.Icmp("png")) {
        ret = WritePNG(filename);
    } else if (!extension.Icmp("bmp")) {
        ret = WriteBMP(filename);
    } else if (!extension.Icmp("pcx")) {
        ret = WritePCX(filename);
    }

    return ret;
}

Image *Image::NewImageFromFile(const char *filename) {
    Image *image = new Image;
    image->Load(filename);
    return image;
}

BE_NAMESPACE_END
