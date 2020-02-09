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
#include "Platform/PlatformSystem.h"
#include <SystemConfiguration/SCNetworkReachability.h>
#include <sys/sysctl.h>
#include <netinet/in.h>

BE_NAMESPACE_BEGIN

void PlatformIOSSystem::GetEnvVar(const char *varName, char *result, uint32_t resultLength) {
    char *ret = getenv(varName);
    if (ret) {
        strncpy(result, ret, resultLength);
    } else {
        *result = 0;
    }
}

void PlatformIOSSystem::SetEnvVar(const char *varName, const char *value) {
}

const char *PlatformIOSSystem::UserDir() {
    static char path[1024] = "";
    if (!path[0]) {
        NSString *userDir = [NSSearchPathForDirectoriesInDomains(NSUserDirectory, NSUserDomainMask, YES) objectAtIndex:0];
        strcpy(path, (const char *)[userDir cStringUsingEncoding:NSUTF8StringEncoding]);
    }
    return path;
}

const char *PlatformIOSSystem::UserDocumentDir() {
    static char path[1024] = "";
    if (!path[0]) {
        NSString *documentDir = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex: 0];
        strcpy(path, (const char *)[documentDir cStringUsingEncoding:NSUTF8StringEncoding]);
    }
    return path;
}

const char *PlatformIOSSystem::UserAppDataDir() {
    static char path[1024] = "";
    if (!path[0]) {
        NSString *libraryDir = [NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES) objectAtIndex: 0];
        strcpy(path, (const char *)[libraryDir cStringUsingEncoding:NSUTF8StringEncoding]);
    }
    return path;
}

const char *PlatformIOSSystem::UserTempDir() {
    static char path[1024] = "";
    if (!path[0]) {
        NSString *tempDir = NSTemporaryDirectory();
        strcpy(path, (const char *)[tempDir cStringUsingEncoding:NSUTF8StringEncoding]);
    }
    return path;
}

int32_t PlatformIOSSystem::NumCPUCores() {
    static int32_t numCores = -1;
    if (numCores == -1) {
        size_t size = sizeof(int32_t);
        if (sysctlbyname("hw.ncpu", &numCores, &size, nullptr, 0) != 0) {
            numCores = 1;
        }
    }
    return numCores;
}

bool PlatformIOSSystem::IsDebuggerPresent() {
    // Based on http://developer.apple.com/library/mac/#qa/qa1361/index.html
    int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid()};
    struct kinfo_proc info;
    size_t size = sizeof(info);
    info.kp_proc.p_flag = 0;

    sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);
    
    return (info.kp_proc.p_flag & P_TRACED) != 0;
}

bool PlatformIOSSystem::HasActiveWiFiConnection() {
    struct sockaddr_in zeroAddress;
    memset(&zeroAddress, 0, sizeof(zeroAddress));
    zeroAddress.sin_len = sizeof(zeroAddress);
    zeroAddress.sin_family = AF_INET;

    SCNetworkReachabilityRef reachabilityRef = SCNetworkReachabilityCreateWithAddress(kCFAllocatorDefault, (const struct sockaddr *)&zeroAddress);
    SCNetworkReachabilityFlags reachabilityFlags;
    bool isFlagsAvailable = SCNetworkReachabilityGetFlags(reachabilityRef, &reachabilityFlags);
    CFRelease(reachabilityRef);
    
    bool hasActiveWiFiConnection = false;
    if (isFlagsAvailable) {
        bool isReachable = (reachabilityFlags & kSCNetworkReachabilityFlagsReachable) != 0 &&
                           (reachabilityFlags & kSCNetworkReachabilityFlagsConnectionRequired) == 0 &&
                           // in case kSCNetworkReachabilityFlagsConnectionOnDemand || kSCNetworkReachabilityFlagsConnectionOnTraffic
                           (reachabilityFlags & kSCNetworkReachabilityFlagsInterventionRequired) == 0; 
                    
        hasActiveWiFiConnection = isReachable && (reachabilityFlags & kSCNetworkReachabilityFlagsIsWWAN) == 0;
    }
    
    return hasActiveWiFiConnection; 
}

BE_NAMESPACE_END
