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
    
    UIAlertController *alert = [UIAlertController
                                alertControllerWithTitle:@"Error"
                                message:nsmsg
                                preferredStyle:UIAlertControllerStyleAlert];
    
    UIAlertAction *okButton = [UIAlertAction
                               actionWithTitle:@"OK"
                               style:UIAlertActionStyleDefault
                               handler:^(UIAlertAction * action) {
                                    // Handle your ok button action here
                               }];
    
    [alert addAction:okButton];
    
    UIViewController *viewController = [[[[UIApplication sharedApplication] delegate] window] rootViewController];
    [viewController presentViewController:alert animated:YES completion:nil];
    
    Quit();
}

BE_NAMESPACE_END
