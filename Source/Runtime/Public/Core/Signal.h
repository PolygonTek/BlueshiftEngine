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
    static const int MaxSignalDefs = 4096;
    
    explicit SignalDef(const char *name, const char *formatSpec = nullptr, char returnType = 0);
    /// Prevents copy constructor
    SignalDef(const SignalDef &rhs) = delete;

                            /// Prevents assignment operator
    SignalDef &             operator=(const SignalDef &rhs) = delete;

    const char *            GetName() const { return name; }
    const char *            GetArgFormat() const { return formatSpec; }
    char                    GetReturnType() const { return returnType; }
    int                     GetSignalNum() const { return signalNum; }
    int                     GetNumArgs() const { return numArgs; }
    unsigned int            GetFormatSpecBits() const { return formatSpecBits; }
    size_t                  GetArgSize() const { return argSize; }
    int                     GetArgOffset(int arg) const { assert((arg >= 0) && (arg < EventDef::MaxArgs)); return argOffset[arg]; }

    static int              NumSignals() { return numSignalDefs; }
    static const SignalDef *GetSignal(int signalNum) { return signalDefs[signalNum]; }
    static const SignalDef *FindSignal(const char *name);

private:
    const char *            name;
    const char *            formatSpec;
    unsigned int            formatSpecBits;
    int                     returnType;
    int                     numArgs;
    size_t                  argSize;
    int                     argOffset[EventDef::MaxArgs];
    int                     signalNum;

    static SignalDef *      signalDefs[MaxSignalDefs];
    static int              numSignalDefs;
};

class BE_API Signal {
    friend class SignalSystem;

public:
    ~Signal();
    
    byte *                  GetData() { return data; }

private:
    const SignalDef *       signalDef;
    byte *                  data;
    SignalObject *          receiver;
    SignalCallback          callback;
    LinkList<Signal>        node;
};

class BE_API SignalSystem {
public:
    static const int MaxSignals = 4096;

    static void             Init();
    static void             Shutdown();

    static void             Clear();

                            /// Create a new signal with the given signal def and arguments
    static Signal *         AllocSignal(const SignalDef *signalDef, const SignalCallback callback, int numArgs, va_list args);
    static void             FreeSignal(Signal *signal);

    static void             CopyArgPtrs(const SignalDef *signalDef, int numArgs, va_list args, intptr_t data[EventDef::MaxArgs]);

    static void             ScheduleSignal(Signal *signal, SignalObject *receiver);

    static void             CancelSignal(const SignalObject *receiver, const SignalDef *signalDef = nullptr);

    static void             ServiceSignal(Signal *signal);
    static void             ServiceSignals();

    static bool             initialized;

private:
    static Signal           signalPool[MaxSignals];
    static LinkList<Signal> freeSignals;
    static LinkList<Signal> signalQueue;
};

BE_NAMESPACE_END
