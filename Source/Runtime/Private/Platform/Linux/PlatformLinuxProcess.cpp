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
#include "Platform/PlatformProcess.h"

BE_NAMESPACE_BEGIN

// get the full path to the running executable
const wchar_t *PlatformLinuxProcess::ExecutableFileName() {
    //static wchar_t name[512] = L"";
    //if (!name[0]) {
    //	NSString *nsExeName = [[[NSBundle mainBundle] executablePath] lastPathComponent];
    //	CFStringToWideString((__bridge CFStringRef)nsExeName, name);
    //}
    //return name;
    assert(0);; return 0;
}

const wchar_t *PlatformLinuxProcess::ComputerName() {
    //static wchar_t name[256] = L"";
    //if (!name[0]) {
    //	char cname[COUNT_OF(name)];
    //	gethostname(cname, COUNT_OF(cname));
    //	WStr::Copynz(name, Str::ToWStr(cname).c_str(), COUNT_OF(name));
    //}
    //return name;
    assert(0);; return 0;
}

const wchar_t *PlatformLinuxProcess::UserName() {
    //static wchar_t name[256] = L"";
    //if (!name[0]) {
    //	CFStringToWideString((__bridge CFStringRef)NSUserName(), name);
    //}
    //return name;
    assert(0);; return 0;
}

BE_NAMESPACE_END
