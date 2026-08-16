// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "BlueshiftEngine.h"
#include "Test.h"

static void SystemLog(const int logLevel, const char *msg) {
    printf("%s", msg);
}

static void SystemError(const int errLevel, const char *msg) {
    printf("ERROR: %s", msg);
};

int main(int argc, char *argv[]) {
#if defined(__WIN32__) && defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    //_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif

    BE1::Str workingDir = argv[0];
    workingDir.StripFileName();
    workingDir.AppendPath("../../.."); // Strip "TestBase.app/Contents/MacOS"
    workingDir.CleanPath();
    BE1::PlatformFile::SetCwd(workingDir);

    BE1::Str enginePath = BE1::PlatformFile::ExecutablePath();
    enginePath.AppendPath("../../.."); // Strip "Bin/macOS/<Configuration>"
    enginePath.CleanPath();
    BE1::Engine::InitBase(4096, enginePath, SystemLog, SystemError);

    //Test::TestContainer();

    //Test::TestRValue();

    Test::TestMath();

    Test::TestFloatConverter();

    Test::TestImage();

    Test::TestSIMD();

    Test::TestThread();

    //Test::TestTask();


    //Test::TestLua();

    BE1::Engine::ShutdownBase();
}
