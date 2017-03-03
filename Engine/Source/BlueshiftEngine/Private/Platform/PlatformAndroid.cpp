#include "Precompiled.h"
#include "PlatformGeneric.h"
#include "PlatformAndroid.h"
#include <android/log.h>
#include "Core/WStr.h"

BE_NAMESPACE_BEGIN

PlatformAndroid::PlatformAndroid() {
//    window = nil;
}

void PlatformAndroid::Init() {
	PlatformGeneric::Init();
}

void PlatformAndroid::Shutdown() {
	PlatformGeneric::Shutdown();
}

void PlatformAndroid::SetMainWindowHandle(void *windowHandle) {
//	window = (NvEGLUtil *)windowHandle;
}

void PlatformAndroid::Quit() {
    //[[UIApplication sharedApplication] terminateWithSuccess];
 //   exit(EXIT_SUCCESS);
	assert(0);;
}

void PlatformAndroid::Log(const wchar_t *msg) {
    //NSString *nsmsg = [[NSString alloc] initWithBytes:msg
    //                                           length:wcslen(msg) * sizeof(*msg)
    //                                         encoding:NSUTF32LittleEndianStringEncoding];
    //NSLog(@"%@", nsmsg);
	__android_log_write(ANDROID_LOG_INFO, "blueshift", tombs(msg));
}

void PlatformAndroid::Error(const wchar_t *msg) {
    //NSString *nsmsg = [[NSString alloc] initWithBytes:msg
    //                                           length:wcslen(msg) * sizeof(*msg)
    //                                         encoding:NSUTF32LittleEndianStringEncoding];
    //
    //UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Error"
    //                                                message:nsmsg
    //                                               delegate:nil
    //                                      cancelButtonTitle:@"OK"
    //                                      otherButtonTitles: nil];
    //[alert show];
    //
    //Quit();
	__android_log_write(ANDROID_LOG_ERROR, "blueshift", tombs(msg));
}


BE_NAMESPACE_END