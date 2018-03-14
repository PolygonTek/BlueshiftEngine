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
#include "Core/Guid.h"
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

static const int    MaxSignalStringLen = 128;
static const int    MaxSignalsPerFrame = 4096;

static bool         signalErrorOccured = false;
static char         signalErrorMsg[128];

SignalDef *         SignalDef::signalDefs[SignalDef::MaxSignalDefs];
int                 SignalDef::numSignalDefs = 0;

SignalDef::SignalDef(const char *name, const char *formatSpec, char returnType) {
    assert(name);
    assert(!SignalSystem::initialized);

    // Allow NULL to indicate no args, but always store it as ""
    // so we don't have to check for it.
    if (!formatSpec) {
        formatSpec = "";
    }

    this->name = name;
    this->formatSpec = formatSpec;
    this->returnType = returnType;
    this->numArgs = (int)strlen(formatSpec);
    assert(this->numArgs <= EventDef::MaxArgs);

    if (this->numArgs > EventDef::MaxArgs) {
        signalErrorOccured = true;
        ::sprintf(signalErrorMsg, "SignalDef::SignalDef : Too many args for '%s' event.", name);
        return;
    }

    // Calculate the offsets for each argument
    memset(this->argOffset, 0, sizeof(this->argOffset));
    this->argSize = 0;

    unsigned int argBits = 0;

    for (int i = 0; i < this->numArgs; i++) {
        this->argOffset[i] = (int)this->argSize;

        switch (this->formatSpec[i]) {
        case VariantArg::IntType:
            this->argSize += sizeof(int);
            break;
        case VariantArg::FloatType:
            argBits |= 1 << i;
            this->argSize += sizeof(float);
            break;
        case VariantArg::PointerType:
            this->argSize += sizeof(void *);
            break;
        case VariantArg::PointType:
            this->argSize += sizeof(Point);
            break;
        case VariantArg::RectType:
            this->argSize += sizeof(Rect);
            break;
        case VariantArg::Vec3Type:
            this->argSize += sizeof(Vec3);
            break;
        case VariantArg::Mat3x3Type:
            this->argSize += sizeof(Mat3);
            break;
        case VariantArg::Mat4x4Type:
            this->argSize += sizeof(Mat4);
            break;
        case VariantArg::GuidType:
            this->argSize += sizeof(Guid);
            break;
        case VariantArg::StringType:
            this->argSize += MaxSignalStringLen * sizeof(char);
            break;
        case VariantArg::WStringType:
            this->argSize += MaxSignalStringLen * sizeof(wchar_t);
            break;
        default:
            signalErrorOccured = true;
            ::sprintf(signalErrorMsg, "SignalDef::SignalDef : Invalid arg format '%s' string for '%s' event.", formatSpec, name);
            return;
        }
    }

    formatSpecBits = (numArgs << EventDef::MaxArgs) | argBits;

    for (int i = 0; i < this->numSignalDefs; i++) {
        SignalDef *sigdef = this->signalDefs[i];

        if (!Str::Cmp(name, sigdef->name)) {
            // Same name but different formatSpec
            if (Str::Cmp(formatSpec, sigdef->formatSpec)) {
                signalErrorOccured = true;
                ::sprintf(signalErrorMsg, "Signal '%s' defined twice with same name but differing format strings ('%s'!='%s').", name, formatSpec, sigdef->formatSpec);
                return;
            }

            // Same name but different returnType
            if (sigdef->returnType != returnType) {
                signalErrorOccured = true;
                ::sprintf(signalErrorMsg, "Signal '%s' defined twice with same name but differing return types ('%c'!='%c').", name, returnType, sigdef->returnType);
                return;
            }

            this->signalNum = sigdef->signalNum;
            return;
        }
    }

    if (this->numSignalDefs >= MaxSignalDefs) {
        signalErrorOccured = true;
        ::sprintf(signalErrorMsg, "numSignalDefs >= MaxSignalDefs");
        return;
    }

    this->signalNum = this->numSignalDefs;
    this->signalDefs[this->numSignalDefs] = this;
    this->numSignalDefs++;
}

const SignalDef *SignalDef::FindSignal(const char *name) {
    assert(name);

    for (int i = 0; i < numSignalDefs; i++) {
        SignalDef *ev = signalDefs[i];
        if (!Str::Cmp(name, ev->name)) {
            return ev;
        }
    }

    return nullptr;
}

//-----------------------------------------------------------------------------------------

bool                SignalSystem::initialized = false;
LinkList<Signal>    SignalSystem::freeSignals;
LinkList<Signal>    SignalSystem::signalQueue;
Signal              SignalSystem::signalPool[SignalSystem::MaxSignals];

Signal::~Signal() {
    SignalSystem::FreeSignal(this);
}

void SignalSystem::Clear() {
    freeSignals.Clear();
    signalQueue.Clear();

    for (int i = 0; i < SignalSystem::MaxSignals; i++) {
        FreeSignal(&signalPool[i]);
    }
}

void SignalSystem::Init() {
    BE_LOG(L"Initializing signal system\n");

    if (signalErrorOccured) {
        BE_ERRLOG(L"%hs", signalErrorMsg);
    }

    Clear();

    if (initialized) {
        BE_LOG(L"...already initialized\n");
        return;
    }

    BE_LOG(L"...%i signal definitions\n", SignalDef::NumSignals());

    initialized = true;
}

void SignalSystem::Shutdown() {
    BE_LOG(L"Shutdown signal system\n");

    if (!initialized) {
        BE_LOG(L"...not started\n");
        return;
    }

    Clear();

    initialized = false;
}

void SignalSystem::FreeSignal(Signal *signal) {
    if (signal->data) {
        Mem_Free(signal->data);
        signal->data = nullptr;
    }

    signal->signalDef = nullptr;
    signal->receiver = nullptr;
    signal->callback = nullptr;

    signal->node.SetOwner(signal);
    signal->node.AddToEnd(SignalSystem::freeSignals);
}

Signal *SignalSystem::AllocSignal(const SignalDef *sigdef, const SignalCallback callback, int numArgs, va_list args) {
    if (freeSignals.IsListEmpty()) {
        BE_ERRLOG(L"SignalSystem::AllocSignal: No more free signals\n");
    }

    Signal *newSignal = freeSignals.Next();
    newSignal->node.Remove();
    newSignal->signalDef = sigdef;
    newSignal->callback = callback;

    if (numArgs != sigdef->GetNumArgs()) {
        BE_ERRLOG(L"SignalSystem::AllocSignal: Wrong number of args for '%hs' signal.\n", sigdef->GetName());
    }

    size_t size = sigdef->GetArgSize();
    if (size) {
        newSignal->data = (byte *)Mem_Alloc(size);
        memset(newSignal->data, 0, size);
    } else {
        newSignal->data = nullptr;
    }

    // Copy arguments to signal data
    const char *format = sigdef->GetArgFormat();
    for (int argIndex = 0; argIndex < numArgs; argIndex++) {
        VariantArg *arg = va_arg(args, VariantArg *);
        if (format[argIndex] != arg->type) {
            BE_ERRLOG(L"SignalSystem::AllocSignal: Wrong type passed in for arg #%d on '%hs' signal.\n", argIndex, sigdef->GetName());
        }

        byte *dataPtr = &newSignal->data[sigdef->GetArgOffset(argIndex)];

        switch (format[argIndex]) {
        case VariantArg::IntType:
            *reinterpret_cast<int *>(dataPtr) = arg->intValue;
            break;
        case VariantArg::FloatType:
            *reinterpret_cast<float *>(dataPtr) = arg->floatValue;
            break;
        case VariantArg::PointerType:
            *reinterpret_cast<void **>(dataPtr) = reinterpret_cast<void *>(arg->pointer);
            break;
        case VariantArg::PointType:
            if (arg->pointer) {
                *reinterpret_cast<Point *>(dataPtr) = *reinterpret_cast<const Point *>(arg->pointer);
            }
            break;
        case VariantArg::RectType:
            if (arg->pointer) {
                *reinterpret_cast<Rect *>(dataPtr) = *reinterpret_cast<const Rect *>(arg->pointer);
            }
            break;
        case VariantArg::Vec3Type:
            if (arg->pointer) {
                *reinterpret_cast<Vec3 *>(dataPtr) = *reinterpret_cast<const Vec3 *>(arg->pointer);
            }
            break;
        case VariantArg::Mat3x3Type:
            if (arg->pointer) {
                *reinterpret_cast<Mat3 *>(dataPtr) = *reinterpret_cast<const Mat3 *>(arg->pointer);
            }
            break;
        case VariantArg::Mat4x4Type:
            if (arg->pointer) {
                *reinterpret_cast<Mat4 *>(dataPtr) = *reinterpret_cast<const Mat4 *>(arg->pointer);
            }
            break;
        case VariantArg::GuidType:
            if (arg->pointer) {
                *reinterpret_cast<Guid *>(dataPtr) = *reinterpret_cast<const Guid *>(arg->pointer);
            }
            break;
        case VariantArg::StringType:
            if (arg->pointer) {
                Str::Copynz(reinterpret_cast<char *>(dataPtr), reinterpret_cast<const char *>(arg->pointer), MaxSignalStringLen);
            }
            break;
        case VariantArg::WStringType:
            if (arg->pointer) {
                WStr::Copynz(reinterpret_cast<wchar_t *>(dataPtr), reinterpret_cast<const wchar_t *>(arg->pointer), MaxSignalStringLen);
            }
            break;
        default:
            BE_ERRLOG(L"SignalSystem::AllocSignal: Invalid arg format '%hs' string for '%hs' signal.\n", format, sigdef->GetName());
            break;
        }
    }

    return newSignal;
}

void SignalSystem::CopyArgPtrs(const SignalDef *sigdef, int numArgs, va_list args, intptr_t argPtrs[EventDef::MaxArgs]) {
    if (numArgs != sigdef->GetNumArgs()) { 
        BE_ERRLOG(L"SignalSystem::CopyArgPtrs: Wrong number of args for '%hs' signal.\n", sigdef->GetName());
    }

    const char *format = sigdef->GetArgFormat();

    for (int argIndex = 0; argIndex < numArgs; argIndex++) {
        VariantArg *arg = va_arg(args, VariantArg *);
        if (format[argIndex] != arg->type) {
            BE_ERRLOG(L"SignalSystem::CopyArgPtrs: Wrong type passed in for arg #%d on '%hs' signal.\n", argIndex, sigdef->GetName());
        }

        argPtrs[argIndex] = arg->pointer;
    }
}

void SignalSystem::ScheduleSignal(Signal *signal, SignalObject *receiver) {
    assert(initialized);
    if (!initialized) {
        return;
    }

    signal->receiver = receiver;

    signal->node.Remove();
    signal->node.AddToEnd(signalQueue);
}

void SignalSystem::CancelSignal(const SignalObject *receiver, const SignalDef *sigdef) {
    if (!initialized) {
        return;
    }

    LinkList<Signal> &queue = signalQueue;

    Signal *next;
    for (Signal *signal = queue.Next(); signal != nullptr; signal = next) {
        next = signal->node.Next();
        if (signal->receiver == receiver) {
            if (!sigdef || (sigdef == signal->signalDef)) {
                FreeSignal(signal);
            }
        }
    }
}

void SignalSystem::ServiceSignal(Signal *signal) {
    intptr_t argPtrs[EventDef::MaxArgs];

    // copy the data into the local argPtrs array and set up pointers
    const SignalDef *sigdef = signal->signalDef;
    const char *formatSpec = sigdef->GetArgFormat();

    int numArgs = sigdef->GetNumArgs();

    for (int i = 0; i < numArgs; i++) {
        int offset = sigdef->GetArgOffset(i);
        byte *data = signal->data;

        switch (formatSpec[i]) {
        case VariantArg::IntType:
            argPtrs[i] = *reinterpret_cast<int *>(&data[offset]);
            break;
        case VariantArg::FloatType:
            argPtrs[i] = *reinterpret_cast<float *>(&data[offset]);
            break; 
        case VariantArg::PointerType:
            *reinterpret_cast<void **>(&argPtrs[i]) = *reinterpret_cast<void **>(&data[offset]);
            break;
        case VariantArg::PointType:
            *reinterpret_cast<Point **>(argPtrs[i]) = reinterpret_cast<Point *>(&data[offset]);
            break;
        case VariantArg::RectType:
            *reinterpret_cast<Rect **>(&argPtrs[i]) = reinterpret_cast<Rect *>(&data[offset]);
            break;
        case VariantArg::Vec3Type:
            *reinterpret_cast<Vec3 **>(&argPtrs[i]) = reinterpret_cast<Vec3 *>(&data[offset]);
            break;
        case VariantArg::Mat3x3Type:
            *reinterpret_cast<Mat3 **>(&argPtrs[i]) = reinterpret_cast<Mat3 *>(&data[offset]);
            break;
        case VariantArg::Mat4x4Type:
            *reinterpret_cast<Mat4 **>(&argPtrs[i]) = reinterpret_cast<Mat4 *>(&data[offset]);
            break;
        case VariantArg::GuidType:
            *reinterpret_cast<Guid **>(&argPtrs[i]) = reinterpret_cast<Guid *>(&data[offset]);
            break;
        case VariantArg::StringType:
            *reinterpret_cast<const char **>(&argPtrs[i]) = reinterpret_cast<const char *>(&data[offset]);
            break;
        case VariantArg::WStringType:
            *reinterpret_cast<const wchar_t **>(&argPtrs[i]) = reinterpret_cast<const wchar_t *>(&data[offset]);
            break;
        default:
            BE_ERRLOG(L"SignalSystem::ServiceEvent : Invalid arg format '%hs' string for '%hs' signal.\n", formatSpec, sigdef->GetName());
        }
    }

    // the signal is removed from its list so that if then object
    // is deleted, the signal won't be freed twice
    signal->node.Remove();
    assert(signal->receiver);
    signal->receiver->ExecuteCallback(signal->callback, sigdef, argPtrs);

    // return the signal to the free list
    FreeSignal(signal);
}

void SignalSystem::ServiceSignals() {
    int processedCount = 0;

    while (!signalQueue.IsListEmpty()) {
        Signal *sig = signalQueue.Next();
        assert(sig);

        ServiceSignal(sig);

        processedCount++;
        if (processedCount > MaxSignalsPerFrame) {
            BE_ERRLOG(L"Signal overflow.  Possible infinite loop in script.\n");
        }
    }
}

BE_NAMESPACE_END
