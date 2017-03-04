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

#pragma once

#include "Event.h"
#include "SignalObject.h"

BE_NAMESPACE_BEGIN

class SignalObject;

class BE_API SignalDef {
public:
    static const int MaxSignals = 4096;
    
    explicit SignalDef(const char *command, const char *formatSpec = nullptr, char returnType = 0);
    
    const char *                GetName() const { return name; }
    const char *                GetArgFormat() const { return formatSpec; }
    char                        GetReturnType() const { return returnType; }
    int                         GetSignalNum() const { return signalnum; }
    int                         GetNumArgs() const { return numArgs; }
    size_t                      GetArgSize() const { return argSize; }
    int                         GetArgOffset(int arg) const { assert((arg >= 0) && (arg < EventArg::MaxArgs)); return argOffset[arg]; }
    
    static int                  NumSignals();
    static const SignalDef *    GetSignal(int signalnum);
    static const SignalDef *    FindSignal(const char *name);

private:
    const char *                name;
    const char *                formatSpec;
    int                         returnType;
    int                         numArgs;
    size_t                      argSize;
    int                         argOffset[EventArg::MaxArgs];
    int                         signalnum;
    
    static SignalDef *          signalDefList[MaxSignals];
    static int                  numSignalDefs;
};

class BE_API Signal {
public:
    ~Signal();
    
    void                        Free();
    void                        Schedule(SignalObject *receiver);
    byte *                      GetData() { return data; }
    
    static void                 Init();
    static void                 Shutdown();
    
    static Signal *             Alloc(const SignalDef *sigdef, const SignalCallback callback, int numArgs, va_list args);
    static void                 CopyArgPtrs(const SignalDef *sigdef, int numArgs, va_list args, intptr_t data[EventArg::MaxArgs]);

    static void                 CancelSignal(const SignalObject *receiver, const SignalDef *sigdef = nullptr);
    static void                 ServiceSignal(Signal *signal);
    static void                 ServiceSignals();
    static void                 ClearSignalList();

    static bool                 initialized;

private:
    const SignalDef *           signalDef;
    byte *                      data;
    SignalObject *              receiver;
    SignalCallback              callback;
    LinkList<Signal>            node;
    
    static LinkList<Signal>     freeSignals;
    static LinkList<Signal>     signalQueue;
    static Signal               signalPool[SignalDef::MaxSignals];
};

BE_NAMESPACE_END
