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
#include "Core/WStr.h"
#include "Containers/StrArray.h"

BE_NAMESPACE_BEGIN

void ProcessHandle::Close() {
    NSTask *nsTask = (__bridge_transfer NSTask *)task;
    NSPipe *nsPipe = (__bridge_transfer NSPipe *)stdOutPipe;
    
    if (nsTask) {
        nsTask = nil;
        nsPipe = nil;
    }
}

static void ArgumentsToStrArray(const wchar_t *args, WStrArray &argsArray) {
    wchar_t buffer[2048];
    wchar_t *ptr = buffer;
    bool inQuote = false;
    int length = WStr::Length(args);
    
    for (int i = 0; i < length; i++) {
        const wchar_t c = args[i];
        
        if (c == '"') {
            inQuote = !inQuote;
            continue;
        }
        
        if (c == ' ' && !inQuote) {
            if (ptr != buffer) {
                *ptr++ = '\0';
                ptr = buffer;
                argsArray.Append(buffer);
            }
        } else {
            *ptr++ = c;
        }
    }
    
    if (ptr != buffer && !inQuote) {
        *ptr++ = '\0';
        argsArray.Append(buffer);
    }
}

static NSString *GetAppPath(const NSString *appPath) {
    NSString *userShell = [[[NSProcessInfo processInfo] environment] objectForKey:@"SHELL"];
    
    NSTask *task = [[NSTask alloc] init];
    [task setLaunchPath:userShell];
    
    NSArray *args = [NSArray arrayWithObjects:@"-l",
                     @"-c",
                     [NSString stringWithFormat:@"which %@", appPath],
                     nil];
    [task setArguments:args];
    
    NSPipe *outPipe = [[NSPipe alloc] init];
    [task setStandardOutput:outPipe];
    [task launch];
    
    NSData *data = [[outPipe fileHandleForReading] readDataToEndOfFile];
    NSString *path = [[NSString alloc] initWithData:data encoding:NSASCIIStringEncoding];
    path = [path stringByReplacingOccurrencesOfString:@"\n" withString:@""];
    
    return path;
}

ProcessHandle PlatformMacOSProcess::CreateProccess(const wchar_t *appPath, const wchar_t *args, const wchar_t *workingPath) {
    NSString *nsAppPath = (__bridge NSString *)WideStringToCFString(appPath);

    if (![[NSFileManager defaultManager] isExecutableFileAtPath:nsAppPath]) {
        nsAppPath = GetAppPath(nsAppPath);
        
        if (![[NSFileManager defaultManager] isExecutableFileAtPath:nsAppPath]) {
            return ProcessHandle(nullptr);
        }
    }
    
    NSTask *task = [[NSTask alloc] init];
    [task setLaunchPath:nsAppPath];

    WStrArray argsArray;
    ArgumentsToStrArray(args, argsArray);
    
    NSMutableArray *argumentsArray = [[NSMutableArray alloc] init];
    
    for (int i = 0; i < argsArray.Count(); i++) {
        WStr arg = argsArray[i];
        
        [argumentsArray addObject:(__bridge NSString *)WideStringToCFString(arg.c_str())];
    }
    
    [task setArguments:argumentsArray];
    
    if (workingPath) {
        [task setCurrentDirectoryPath:(__bridge NSString *)WideStringToCFString(workingPath)];
    }
    
    NSPipe *stdOutPipe = [[NSPipe alloc] init];
    
    [task setStandardOutput:stdOutPipe];
    [task setStandardError:stdOutPipe];
    
    [task launch];

    ProcessHandle processHandle = ProcessHandle((__bridge_retained void *)task);
    processHandle.stdOutPipe = (__bridge_retained void *)stdOutPipe;
    return processHandle;
}

bool PlatformMacOSProcess::IsProccessRunning(ProcessHandle &processHandle) {
    NSTask *task = (__bridge NSTask *)processHandle.task;
    return [task isRunning];
}

void PlatformMacOSProcess::WaitForProccess(ProcessHandle &processHandle) {
    NSTask *task = (__bridge NSTask *)processHandle.task;
    [task waitUntilExit];
}

void PlatformMacOSProcess::TerminateProccess(ProcessHandle &processHandle) {
    NSTask *task = (__bridge NSTask *)processHandle.task;
    [task terminate];
    processHandle.Close();
}

bool PlatformMacOSProcess::GetProcessExitCode(ProcessHandle &processHandle, int32_t *exitCode) {
    if (IsProccessRunning(processHandle)) {
        return false;
    }
    
    NSTask *task = (__bridge NSTask *)processHandle.task;
    *exitCode = [task terminationStatus];
    return true;
}

bool PlatformMacOSProcess::ReadProcessOutput(ProcessHandle &processHandle, int bufferLength, char *buffer) {
    NSPipe *pipe = (__bridge NSPipe *)processHandle.stdOutPipe;
    NSFileHandle *fileHandle = [pipe fileHandleForReading];
    
    NSData *data = [fileHandle readDataToEndOfFile];
    NSString *string = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    const char *cString = [string cStringUsingEncoding:NSUTF8StringEncoding];
    Str::Copynz(buffer, cString, bufferLength);
    
    return true;
}

BE_NAMESPACE_END
