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
#include "Core/Signal.h"
#include "Core/Heap.h"
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

static const int	MaxSignalStringLen = 128;
static const int	MaxSignalsPerFrame = 4096;

static bool         signalError = false;
static char         signalErrorMsg[128];

SignalDef *         SignalDef::signalDefList[SignalDef::MaxSignals];
int                 SignalDef::numSignalDefs = 0;

SignalDef::SignalDef(const char *command, const char *formatSpec, char returnType) {
    assert(command);
    assert(!Signal::initialized);

    // Allow nullptr to indicate no args, but always store it as ""
    // so we don't have to check for it.
    if (!formatSpec) {
        formatSpec = "";
    }

    this->name			= command;
    this->formatSpec	= formatSpec;
    this->returnType	= returnType;
    this->numArgs		= (int)strlen(formatSpec);
    assert(this->numArgs <= EventArg::MaxArgs);

    if (this->numArgs > EventArg::MaxArgs) {
        signalError = true;
        ::sprintf(signalErrorMsg, "SignalDef::SignalDef : Too many args for '%s' event.", command);
        return;
    }

    // Calculate the offsets for each arg
    memset(this->argOffset, 0, sizeof(this->argOffset));
    this->argSize = 0;

    for (int i = 0; i < this->numArgs; i++) {
        this->argOffset[i] = (int)this->argSize;

        switch (this->formatSpec[i]) {
        case EventArg::FloatType:
            this->argSize += sizeof(float);
            break;
        case EventArg::IntType:
            this->argSize += sizeof(int);
            break;
        case EventArg::PointType:
            this->argSize += sizeof(Point);
            break;
        case EventArg::RectType:
            this->argSize += sizeof(Rect);
            break;
        case EventArg::Vec3Type:
            this->argSize += sizeof(Vec3);
            break;
        case EventArg::Mat3x3Type:
            this->argSize += sizeof(Mat3);
            break;
        case EventArg::Mat4x4Type:
            this->argSize += sizeof(Mat4);
            break;
        case EventArg::StringType:
            this->argSize += MaxSignalStringLen * sizeof(char);
            break;
        case EventArg::WStringType:
            this->argSize += MaxSignalStringLen * sizeof(wchar_t);
            break;
        case EventArg::PointerType:
            this->argSize += sizeof(void *);
            break;
        default:
            signalError = true;
            ::sprintf(signalErrorMsg, "SignalDef::SignalDef : Invalid arg format '%s' string for '%s' event.", formatSpec, command);
            return;
            break;
        }
    }

    for (int i = 0; i < this->numSignalDefs; i++) {
        SignalDef *sigdef = this->signalDefList[i];

        if (!Str::Cmp(command, sigdef->name)) {
            // 같은 이름이지만 formatSpec 이 다른 경우
            if (Str::Cmp(formatSpec, sigdef->formatSpec)) {
                signalError = true;
                ::sprintf(signalErrorMsg, "Signal '%s' defined twice with same name but differing format strings ('%s'!='%s').", command, formatSpec, sigdef->formatSpec);
                return;
            }

            // 같은 이름이지만 returnType 이 다른 경우 
            if (sigdef->returnType != returnType) {
                signalError = true;
                ::sprintf(signalErrorMsg, "Signal '%s' defined twice with same name but differing return types ('%c'!='%c').", command, returnType, sigdef->returnType);
                return;
            }

            this->signalnum = sigdef->signalnum;
            return;
        }
    }

    if (this->numSignalDefs >= MaxSignals) {
        signalError = true;
        ::sprintf(signalErrorMsg, "numSignalDefs >= MaxSignals");
        return;
    }

    this->signalnum = this->numSignalDefs;
    this->signalDefList[this->numSignalDefs] = this;
    this->numSignalDefs++;
}

int SignalDef::NumSignals() {
    return numSignalDefs;
}

const SignalDef *SignalDef::GetSignal(int signalnum) {
    return signalDefList[signalnum];
}

const SignalDef *SignalDef::FindSignal(const char *name) {
    assert(name);

    int num = numSignalDefs;
    for (int i = 0; i < num; i++) {
        SignalDef *ev = signalDefList[i];
        if (!Str::Cmp(name, ev->name)) {
            return ev;
        }
    }

    return nullptr;
}

//-----------------------------------------------------------------------------------------

bool                Signal::initialized = false;
LinkList<Signal>    Signal::freeSignals;
LinkList<Signal>    Signal::signalQueue;
Signal              Signal::signalPool[SignalDef::MaxSignals];

Signal::~Signal() {
    Free();
}

void Signal::ClearSignalList() {
    freeSignals.Clear();
    signalQueue.Clear();

    for (int i = 0; i < SignalDef::MaxSignals; i++) {
        signalPool[i].Free();
    }
}

void Signal::Init() {
    BE_LOG(L"Initializing signal system\n");

    if (signalError) {
        BE_ERRLOG(L"%hs", signalErrorMsg);
    }

    if (initialized) {
        BE_LOG(L"...already initialized\n");
        ClearSignalList();
        return;
    }

    ClearSignalList();

    BE_LOG(L"...%i signal definitions\n", SignalDef::NumSignals());

    // the signal system has started
    initialized = true;
}

void Signal::Shutdown() {
    BE_LOG(L"Shutdown signal system\n");

    if (!initialized) {
        BE_LOG(L"...not started\n");
        return;
    }

    ClearSignalList();

    initialized = false;
}

void Signal::Free() {
    if (data) {
        Mem_Free(data);
        data = nullptr;
    }

    this->signalDef = nullptr;
    this->receiver = nullptr;
    this->callback = nullptr;

    node.SetOwner(this);
    node.AddToEnd(Signal::freeSignals);
}

Signal *Signal::Alloc(const SignalDef *sigdef, const SignalCallback callback, int numArgs, va_list args) {
    if (freeSignals.IsListEmpty()) {
        BE_ERRLOG(L"Signal::Alloc: No more free signals\n");
    }

    Signal *sig = freeSignals.Next();
    sig->node.Remove();
    sig->signalDef = sigdef;
    sig->callback = callback;

    if (numArgs != sigdef->GetNumArgs()) {
        BE_ERRLOG(L"Signal::Alloc: Wrong number of args for '%hs' signal.\n", sigdef->GetName());
    }

    size_t size = sigdef->GetArgSize();
    if (size) {
        sig->data = (byte *)Mem_Alloc(size);
        memset(sig->data, 0, size);
    } else {
        sig->data = nullptr;
    }

    // Copy arguments to signal data
    const char *format = sigdef->GetArgFormat();
    for (int i = 0; i < numArgs; i++) {
        EventArg *arg = va_arg(args, EventArg *);
        if (format[i] != arg->type) {
            BE_ERRLOG(L"Signal::Alloc: Wrong type passed in for arg # %d on '%hs' signal.\n", i, sigdef->GetName());
        }

        byte *dataPtr = &sig->data[sigdef->GetArgOffset(i)];

        switch (format[i]) {
        case EventArg::IntType:
            if (arg->pointer) {
                *reinterpret_cast<int *>(dataPtr) = arg->intValue;
            }
            break;
        case EventArg::FloatType:
            if (arg->pointer) {
                *reinterpret_cast<float *>(dataPtr) = arg->floatValue;
            }
            break;  
        case EventArg::PointType:
            if (arg->pointer) {
                *reinterpret_cast<Point *>(dataPtr) = *reinterpret_cast<const Point *>(arg->pointer);
            }
            break;
        case EventArg::RectType:
            if (arg->pointer) {
                *reinterpret_cast<Rect *>(dataPtr) = *reinterpret_cast<const Rect *>(arg->pointer);
            }
            break;
        case EventArg::Vec3Type:
            if (arg->pointer) {
                *reinterpret_cast<Vec3 *>(dataPtr) = *reinterpret_cast<const Vec3 *>(arg->pointer);
            }
            break;
        case EventArg::Mat3x3Type:
            if (arg->pointer) {
                *reinterpret_cast<Mat3 *>(dataPtr) = *reinterpret_cast<const Mat3 *>(arg->pointer);
            }
            break;
        case EventArg::Mat4x4Type:
            if (arg->pointer) {
                *reinterpret_cast<Mat4 *>(dataPtr) = *reinterpret_cast<const Mat4 *>(arg->pointer);
            }
            break;
        case EventArg::StringType:
            if (arg->pointer) {
                Str::Copynz(reinterpret_cast<char *>(dataPtr), reinterpret_cast<const char *>(arg->pointer), MaxSignalStringLen);
            }
            break;
        case EventArg::WStringType:
            if (arg->pointer) {
                WStr::Copynz(reinterpret_cast<wchar_t *>(dataPtr), reinterpret_cast<const wchar_t *>(arg->pointer), MaxSignalStringLen);
            }
            break;
        case EventArg::PointerType:
            *reinterpret_cast<void **>(dataPtr) = reinterpret_cast<void *>(arg->pointer);
            break;
        default:
            BE_ERRLOG(L"Signal::Alloc: Invalid arg format '%hs' string for '%hs' signal.\n", format, sigdef->GetName());
            break;
        }
    }

    return sig;
}

void Signal::CopyArgPtrs(const SignalDef *sigdef, int numArgs, va_list args, intptr_t argPtrs[EventArg::MaxArgs]) {
    const char *format = sigdef->GetArgFormat();
    if (numArgs != sigdef->GetNumArgs()) {
        BE_ERRLOG(L"Signal::CopyArgPtrs: Wrong number of args for '%hs' signal.\n", sigdef->GetName());
    }

    for (int i = 0; i < numArgs; i++) {
        EventArg *arg = va_arg(args, EventArg *);
        if (format[i] != arg->type) {
            BE_ERRLOG(L"Signal::CopyArgPtrs: Wrong type passed in for arg # %d on '%hs' signal.\n", i, sigdef->GetName());
        }

        argPtrs[i] = arg->pointer;
    }
}

void Signal::Schedule(SignalObject *receiver) {
    assert(initialized);
    if (!initialized) {
        return;
    }

    this->receiver = receiver;

    node.Remove();
    node.AddToEnd(signalQueue);
}

void Signal::CancelSignal(const SignalObject *receiver, const SignalDef *sigdef) {
    if (!initialized) {
        return;
    }

    LinkList<Signal> &queue = signalQueue;

    Signal *next;
    for (Signal *signal = queue.Next(); signal != nullptr; signal = next) {
        next = signal->node.Next();
        if (signal->receiver == receiver) {
            if (!sigdef || (sigdef == signal->signalDef)) {
                signal->Free();
            }
        }
    }
}

void Signal::ServiceSignal(Signal *signal) {
    intptr_t argPtrs[EventArg::MaxArgs];

    // copy the data into the local argPtrs array and set up pointers
    const SignalDef *sigdef = signal->signalDef;
    const char *formatSpec = sigdef->GetArgFormat();

    int numArgs = sigdef->GetNumArgs();

    for (int i = 0; i < numArgs; i++) {
        int offset = sigdef->GetArgOffset(i);
        byte *data = signal->data;

        switch (formatSpec[i]) {
        case EventArg::IntType:
            argPtrs[i] = *reinterpret_cast<int *>(&data[offset]);
            break;
        case EventArg::FloatType:
            argPtrs[i] = *reinterpret_cast<float *>(&data[offset]);
            break; 
        case EventArg::PointType:
            *reinterpret_cast<Point **>(argPtrs[i]) = reinterpret_cast<Point *>(&data[offset]);
            break;
        case EventArg::RectType:
            *reinterpret_cast<Rect **>(&argPtrs[i]) = reinterpret_cast<Rect *>(&data[offset]);
            break;
        case EventArg::Vec3Type:
            *reinterpret_cast<Vec3 **>(&argPtrs[i]) = reinterpret_cast<Vec3 *>(&data[offset]);
            break;
        case EventArg::Mat3x3Type:
            *reinterpret_cast<Mat3 **>(&argPtrs[i]) = reinterpret_cast<Mat3 *>(&data[offset]);
            break;
        case EventArg::Mat4x4Type:
            *reinterpret_cast<Mat4 **>(&argPtrs[i]) = reinterpret_cast<Mat4 *>(&data[offset]);
            break;
        case EventArg::StringType:
            *reinterpret_cast<const char **>(&argPtrs[i]) = reinterpret_cast<const char *>(&data[offset]);
            break;
        case EventArg::WStringType:
            *reinterpret_cast<const wchar_t **>(&argPtrs[i]) = reinterpret_cast<const wchar_t *>(&data[offset]);
            break;
        case EventArg::PointerType:
            *reinterpret_cast<void **>(&argPtrs[i]) = *reinterpret_cast<void **>(&data[offset]);
            break;
        default:
            BE_ERRLOG(L"Signal::ServiceEvent : Invalid arg format '%hs' string for '%hs' signal.\n", formatSpec, sigdef->GetName());
        }
    }

    // the signal is removed from its list so that if then object
    // is deleted, the signal won't be freed twice
    signal->node.Remove();
    assert(signal->receiver);
    signal->receiver->ExecuteCallback(signal->callback, numArgs, argPtrs);

    // return the signal to the free list
    signal->Free();
}

void Signal::ServiceSignals() {
    int num = 0;
    while (!signalQueue.IsListEmpty()) {
        Signal *sig = signalQueue.Next();
        assert(sig);

        ServiceSignal(sig);

        num++;
        if (num > MaxSignalsPerFrame) {
            BE_ERRLOG(L"Signal overflow.  Possible infinite loop in script.\n");
        }
    }
}

BE_NAMESPACE_END
