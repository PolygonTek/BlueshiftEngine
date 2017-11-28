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
#include "Core/SignalObject.h"

BE_NAMESPACE_BEGIN

SignalObject::~SignalObject() {
    while (publications.Count() > 0) {
        const Connection *con = publications[0];
        con->sender->Disconnect(con->signalDef, con->receiver, con->function);
    }

    while (subscriptions.Count() > 0) {
        const Connection *con = subscriptions[0];
        con->sender->Disconnect(con->signalDef, con->receiver, con->function);
    }

    SignalSystem::CancelSignal(this);
}

bool SignalObject::IsConnected(const SignalDef *sigdef, SignalObject *receiver, SignalCallback function) const {
    for (int i = 0; i < publications.Count(); i++) {
        const Connection *con = publications[i];
        if (con->signalDef == sigdef && con->sender == this && con->receiver == receiver && con->function == function) {
            return true;
        }
    }

    return false;
}

bool SignalObject::IsConnected(const SignalDef *sigdef, SignalObject *receiver) const {
    for (int i = 0; i < publications.Count(); i++) {
        const Connection *con = publications[i];
        if (con->signalDef == sigdef && con->sender == this && con->receiver == receiver) {
            return true;
        }
    }

    return false;
}

bool SignalObject::Connect(const SignalDef *sigdef, SignalObject *receiver, SignalCallback function, int connectionType) {
    if (connectionType == Unique) {
        for (int i = 0; i < publications.Count(); i++) {
            const Connection *con = publications[i];

            if (con->signalDef == sigdef && con->sender == this && con->receiver == receiver && con->function == function) {
                return false;
            }
        }
    }

    Connection *con = new Connection;
    con->signalDef = sigdef;
    con->connectionType = connectionType;
    con->sender = this;
    con->receiver = receiver;
    con->function = function;

    // connection pointer is shared among sender's publications and receiver's subscriptions.
    this->publications.Append(con);
    receiver->subscriptions.Append(con);

    return true;
}

bool SignalObject::Disconnect(const SignalDef *sigdef, SignalObject *receiver, SignalCallback function) {
    for (int i = 0; i < publications.Count(); i++) {
        const Connection *con = publications[i];
        if (con->signalDef == sigdef && con->receiver == receiver && con->function == function) {
            // remove receiver's subscription
            int index = con->receiver->subscriptions.FindIndex(publications[i]);
            con->receiver->subscriptions.RemoveIndexFast(index);

            // remove sender's publication
            delete publications[i];
            publications.RemoveIndexFast(i);
            return true;
        }
    }

    return false;
}

bool SignalObject::Disconnect(const SignalDef *sigdef, SignalObject *receiver) {
    int *removeIndexes = (int *)_alloca(sizeof(removeIndexes[0]) * publications.Count());
    int count = 0;

    for (int i = 0; i < publications.Count(); i++) {
        const Connection *con = publications[i];
        if (con->signalDef == sigdef && con->receiver == receiver) {
            // remove receiver's subscription
            int index = con->receiver->subscriptions.FindIndex(publications[i]);
            con->receiver->subscriptions.RemoveIndexFast(index);

            // stack sender's publication for remove
            removeIndexes[count++] = i;	
        }
    }

    if (count > 0) {
        for (int i = 0; i < count; i++) {
            delete publications[removeIndexes[i]];
            publications.RemoveIndexFast(removeIndexes[i]);
        }
        return true;
    }

    return false;
}

bool SignalObject::Disconnect(const SignalDef *sigdef) {
    int *removeIndexes = (int *)_alloca(sizeof(removeIndexes[0]) * publications.Count());
    int count = 0;

    for (int i = 0; i < publications.Count(); i++) {
        const Connection *con = publications[i];
        if (con->signalDef == sigdef) {
            // remove receiver's subscription
            int index = con->receiver->subscriptions.FindIndex(publications[i]);
            con->receiver->subscriptions.RemoveIndexFast(index);
            
            // stack sender's publication for remove
            removeIndexes[count++] = i;
        }
    }

    if (count > 0) {
        for (int i = 0; i < count; i++) {
            delete publications[removeIndexes[i]];
            publications.RemoveIndexFast(removeIndexes[i]);
        }
        return true;
    }

    return false;
}

bool SignalObject::EmitSignalArgs(const SignalDef *sigdef, int numArgs, ...) {
    assert(sigdef);
    assert(SignalSystem::initialized);

    if (signalBlocked) {
        return false;
    }

    intptr_t argPtrs[EventArg::MaxArgs];

    va_list args;
    va_start(args, numArgs);
    SignalSystem::CopyArgPtrs(sigdef, numArgs, args, argPtrs);
    va_end(args);

    for (int i = 0; i < publications.Count(); i++) {
        const Connection *con = publications[i];

        if (con->signalDef != sigdef) {
            continue;
        }

        if (con->connectionType & Queued) {
            va_start(args, numArgs);
            Signal *signal = SignalSystem::AllocSignal(sigdef, con->function, numArgs, args);
            va_end(args);

            SignalSystem::ScheduleSignal(signal, con->receiver);
            continue;
        }

        if (!con->receiver->SignalObject::ExecuteCallback(con->function, sigdef->GetNumArgs(), argPtrs)) {
            BE_WARNLOG(L"Invalid formatSpec on signal '%hs'\n", sigdef->GetName());
        }
    }

    return true;
}

bool SignalObject::ExecuteCallback(const SignalCallback &callback, int numArgs, intptr_t *data) {
    switch (numArgs) {
    case 0:
        (this->*callback)();
        return true;
    case 1:
        using EventCallback_1 = void (SignalObject::*)(const intptr_t);
        (this->*(EventCallback_1)callback)(data[0]);
        return true;
    case 2:
        using EventCallback_2 = void (SignalObject::*)(const intptr_t, const intptr_t);
        (this->*(EventCallback_2)callback)(data[0], data[1]);
        return true;
    case 3:
        using EventCallback_3 = void (SignalObject::*)(const intptr_t, const intptr_t, const intptr_t);
        (this->*(EventCallback_3)callback)(data[0], data[1], data[2]);
        return true;
    case 4:
        using EventCallback_4 = void (SignalObject::*)(const intptr_t, const intptr_t, const intptr_t, const intptr_t);
        (this->*(EventCallback_4)callback)(data[0], data[1], data[2], data[3]);
        return true;
    case 5:
        using EventCallback_5 = void (SignalObject::*)(const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t);
        (this->*(EventCallback_5)callback)(data[0], data[1], data[2], data[3], data[4]);
        return true;
    case 6:
        using EventCallback_6 = void (SignalObject::*)(const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t);
        (this->*(EventCallback_6)callback)(data[0], data[1], data[2], data[3], data[4], data[5]);
        return true;
    case 7:
        using EventCallback_7 = void (SignalObject::*)(const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t);
        (this->*(EventCallback_7)callback)(data[0], data[1], data[2], data[3], data[4], data[5], data[6]);
        return true;
    case 8:
        using EventCallback_8 = void (SignalObject::*)(const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t);
        (this->*(EventCallback_8)callback)(data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
        return true;
    }

    return false;
}

bool SignalObject::BlockSignals(bool block) {
    bool prev = signalBlocked;
    signalBlocked = block;
    return prev;
}

BE_NAMESPACE_END
