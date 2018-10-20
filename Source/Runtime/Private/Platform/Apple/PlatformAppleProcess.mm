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
#include "Core/Str.h"

BE_NAMESPACE_BEGIN

// get the full path to the running executable
const char *PlatformAppleProcess::ExecutableFileName() {
	static char name[512] = "";
	if (!name[0]) {
        NSString *nsExeName = [[[NSBundle mainBundle] executablePath] lastPathComponent];
        int l = [nsExeName length];
        memcpy(name, [nsExeName cStringUsingEncoding:NSUTF8StringEncoding], l);
        name[l] = '\0';
	}
	return name;
}

const char *PlatformAppleProcess::ComputerName() {
	static char name[256] = "";
	if (!name[0]) {
		char cname[COUNT_OF(name)];
		gethostname(cname, COUNT_OF(cname));
		Str::Copynz(name, cname, COUNT_OF(name));
	}
	return name;
}

const char *PlatformAppleProcess::UserName() {
	static char name[256] = "";
	if (!name[0]) {
        NSString *nsUserName = NSUserName();
        int l = [nsUserName length];
        memcpy(name, [nsUserName cStringUsingEncoding:NSUTF8StringEncoding], l);
        name[l] = '\0';
	}
	return name;
}

BE_NAMESPACE_END
