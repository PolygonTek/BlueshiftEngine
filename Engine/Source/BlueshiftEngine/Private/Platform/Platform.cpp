#include "Precompiled.h"
#include "PlatformGeneric.h"
#ifdef __WIN32__
#include "PlatformWin.h"
#elif defined __MACOSX__
#include "PlatformMacOS.h"
#elif defined __IOS__
#include "PlatformIOS.h"
#elif defined __ANDROID__
#include "PlatformAndroid.h"
#endif

BE_NAMESPACE_BEGIN

PlatformAbstract *  platform = nullptr;

void Platform::Init() {
#ifdef __WIN32__
    platform = new PlatformWin;
#elif defined __MACOSX__
    platform = new PlatformMacOS;
#elif defined __IOS__
    platform = new PlatformIOS;
#elif defined __ANDROID__
    platform = new PlatformAndroid;
#endif

    platform->Init();
}

void Platform::Shutdown() {
    platform->Shutdown();

    delete platform;
}

PlatformAbstract::~PlatformAbstract() {
}

BE_NAMESPACE_END
