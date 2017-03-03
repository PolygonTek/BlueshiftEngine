#include "BlueshiftEngine.h"
#include "TestContainer.h"
#include "TestMath.h"
#include "TestSIMD.h"
#include "TestCUDA.h"
#include "TestLua.h"

void SystemLog(const int logLevel, const wchar_t *msg) {
    printf("%ls", msg);
}

void SystemError(const int errLevel, const wchar_t *msg) {
    printf("ERROR: %ls", msg);
};

int main(int argc, char *argv[]) {
    BE1::Str workingDir = argv[0];
    workingDir.StripFileName();
    workingDir.AppendPath("../../.."); // Strip "TestBase.app/Contents/MacOS"
    workingDir.CleanPath();
    BE1::PlatformFile::SetCwd(workingDir);

    BE1::Str enginePath = BE1::PlatformFile::ExecutablePath();
    enginePath.AppendPath("../../.."); // Strip "Bin/macOS/<Configuration>"
    enginePath.CleanPath();
    BE1::Engine::InitBase(enginePath, false, SystemLog, SystemError);

    TestContainer();

    TestMath();
    
    TestSIMD();

#if TEST_CUDA
    bool cudaSupported = MyCuda::Init();
    
    if (cudaSupported) {
        TestCUDA();
    }

    if (cudaSupported) {
        MyCuda::Shutdown();
    }
#endif

    TestLua();

    BE1::Engine::ShutdownBase();
}
