#include "Precompiled.h"
#include "PlatformGeneric.h"
#include "PlatformIOS.h"

BE_NAMESPACE_BEGIN

PlatformIOS::PlatformIOS() {
    window = nil;
}

void PlatformIOS::Init() {
    PlatformGeneric::Init();
}

void PlatformIOS::Shutdown() {
    PlatformGeneric::Shutdown();
}

void PlatformIOS::SetMainWindowHandle(void *windowHandle) {
    window = (__bridge UIWindow *)windowHandle;
}

void PlatformIOS::Quit() {
    //[[UIApplication sharedApplication] terminateWithSuccess];
    exit(EXIT_SUCCESS);
}

void PlatformIOS::Log(const wchar_t *msg) {
    NSString *nsmsg = [[NSString alloc] initWithBytes:msg
                                               length:wcslen(msg) * sizeof(*msg)
                                             encoding:NSUTF32LittleEndianStringEncoding];
    NSLog(@"%@", nsmsg);
}

void PlatformIOS::Error(const wchar_t *msg) {
    NSString *nsmsg = [[NSString alloc] initWithBytes:msg
                                               length:wcslen(msg) * sizeof(*msg)
                                             encoding:NSUTF32LittleEndianStringEncoding];
    
    UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Error"
                                                    message:nsmsg
                                                   delegate:nil
                                          cancelButtonTitle:@"OK"
                                          otherButtonTitles: nil];
    [alert show];
    
    Quit();
}

BE_NAMESPACE_END
