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

    intptr_t argPtrs[EventDef::MaxArgs];

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

        con->receiver->ExecuteCallback(con->function, sigdef, argPtrs);
    }

    return true;
}

bool SignalObject::ExecuteCallback(const SignalCallback &callback, const SignalDef *sigdef, intptr_t *data) {
    // formatSpecBits = 0bNNNFFFFFF
    // N means number of arguments
    // F means float bit mask
    switch (sigdef->GetFormatSpecBits()) {
    //----------------------------------------------------------------------------------------------
    // 0 args
    //----------------------------------------------------------------------------------------------
    case 0:
        (this->*callback)();
        return true;
    //----------------------------------------------------------------------------------------------
    // 1 args
    //----------------------------------------------------------------------------------------------
    case 0b001000000:
        using EventCallback_i = void (SignalObject::*)(const intptr_t);
        (this->*(EventCallback_i)callback)(data[0]);
        return true;
    case 0b001000001:
        using EventCallback_f = void (SignalObject::*)(const float);
        (this->*(EventCallback_f)callback)(*(float *)&data[0]);
        return true;
    //----------------------------------------------------------------------------------------------
    // 2 args
    //----------------------------------------------------------------------------------------------
    case 0b010000000:
        using EventCallback_ii = void (SignalObject::*)(const intptr_t, const intptr_t);
        (this->*(EventCallback_ii)callback)(data[0], data[1]);
        return true;
    case 0b010000001:
        using EventCallback_fi = void (SignalObject::*)(const float, const intptr_t);
        (this->*(EventCallback_fi)callback)(*(float *)&data[0], data[1]);
        return true;
    case 0b010000010:
        using EventCallback_if = void (SignalObject::*)(const intptr_t, const float);
        (this->*(EventCallback_if)callback)(data[0], *(float *)&data[1]);
        return true;
    case 0b010000011:
        using EventCallback_ff = void (SignalObject::*)(const float, const float);
        (this->*(EventCallback_ff)callback)(*(float *)&data[0], *(float *)&data[1]);
        return true;
    // 3 args
    case 0b011000000:
        using EventCallback_iii = void (SignalObject::*)(const intptr_t, const intptr_t, const intptr_t);
        (this->*(EventCallback_iii)callback)(data[0], data[1], data[2]);
        return true;
    case 0b011000001:
        using EventCallback_fii = void (SignalObject::*)(const float, const intptr_t, const intptr_t);
        (this->*(EventCallback_fii)callback)(*(float *)&data[0], data[1], data[2]);
        return true;
    case 0b011000010:
        using EventCallback_ifi = void (SignalObject::*)(const intptr_t, const float, const intptr_t);
        (this->*(EventCallback_ifi)callback)(data[0], *(float *)&data[1], data[2]);
        return true;
    case 0b011000011:
        using EventCallback_ffi = void (SignalObject::*)(const float, const float, const intptr_t);
        (this->*(EventCallback_ffi)callback)(*(float *)&data[0], *(float *)&data[1], data[2]);
        return true;
    case 0b011000100:
        using EventCallback_iif = void (SignalObject::*)(const intptr_t, const intptr_t, const float);
        (this->*(EventCallback_iif)callback)(data[0], data[1], *(float *)&data[2]);
        return true;
    case 0b011000101:
        using EventCallback_fif = void (SignalObject::*)(const float, const intptr_t, const float);
        (this->*(EventCallback_fif)callback)(*(float *)&data[0], data[1], *(float *)&data[2]);
        return true;
    case 0b011000110:
        using EventCallback_iff = void (SignalObject::*)(const intptr_t, const float, const float);
        (this->*(EventCallback_iff)callback)(data[0], *(float *)&data[1], *(float *)&data[2]);
        return true;
    case 0b011000111:
        using EventCallback_fff = void (SignalObject::*)(const float, const float, const float);
        (this->*(EventCallback_fff)callback)(*(float *)&data[0], *(float *)&data[1], *(float *)&data[2]);
        return true;
    //----------------------------------------------------------------------------------------------
    // 4 args
    //----------------------------------------------------------------------------------------------
    case 0b100000000:
        using EventCallback_iiii = void (SignalObject::*)(const intptr_t, const intptr_t, const intptr_t, const intptr_t);
        (this->*(EventCallback_iiii)callback)(data[0], data[1], data[2], data[3]);
        return true;
    case 0b100000001:
        using EventCallback_fiii = void (SignalObject::*)(const float, const intptr_t, const intptr_t, const intptr_t);
        (this->*(EventCallback_fiii)callback)(*(float *)&data[0], data[1], data[2], data[3]);
        return true;
    case 0b100000010:
        using EventCallback_ifii = void (SignalObject::*)(const intptr_t, const float, const intptr_t, const intptr_t);
        (this->*(EventCallback_ifii)callback)(data[0], *(float *)&data[1], data[2], data[3]);
        return true;
    case 0b100000011:
        using EventCallback_ffii = void (SignalObject::*)(const float, const float, const intptr_t, const intptr_t);
        (this->*(EventCallback_ffii)callback)(*(float *)&data[0], *(float *)&data[1], data[2], data[3]);
        return true;
    case 0b100000100:
        using EventCallback_iifi = void (SignalObject::*)(const intptr_t, const intptr_t, const float, const intptr_t);
        (this->*(EventCallback_iifi)callback)(data[0], data[1], *(float *)&data[2], data[3]);
        return true;
    case 0b100000101:
        using EventCallback_fifi = void (SignalObject::*)(const float, const intptr_t, const float, const intptr_t);
        (this->*(EventCallback_fifi)callback)(*(float *)&data[0], data[1], *(float *)&data[2], data[3]);
        return true;
    case 0b100000110:
        using EventCallback_iffi = void (SignalObject::*)(const intptr_t, const float, const float, const intptr_t);
        (this->*(EventCallback_iffi)callback)(data[0], *(float *)&data[1], *(float *)&data[2], data[3]);
        return true;
    case 0b100000111:
        using EventCallback_fffi = void (SignalObject::*)(const float, const float, const float, const intptr_t);
        (this->*(EventCallback_fffi)callback)(*(float *)&data[0], *(float *)&data[1], *(float *)&data[2], data[3]);
        return true;
    case 0b100001000:
        using EventCallback_iiif = void (SignalObject::*)(const intptr_t, const intptr_t, const intptr_t, const float);
        (this->*(EventCallback_iiif)callback)(data[0], data[1], data[2], *(float *)&data[3]);
        return true;
    case 0b100001001:
        using EventCallback_fiif = void (SignalObject::*)(const float, const intptr_t, const intptr_t, const float);
        (this->*(EventCallback_fiif)callback)(*(float *)&data[0], data[1], data[2], *(float *)&data[3]);
        return true;
    case 0b100001010:
        using EventCallback_ifif = void (SignalObject::*)(const intptr_t, const float, const intptr_t, const float);
        (this->*(EventCallback_ifif)callback)(data[0], *(float *)&data[1], data[2], *(float *)&data[3]);
        return true;
    case 0b100001011:
        using EventCallback_ffif = void (SignalObject::*)(const float, const float, const intptr_t, const float);
        (this->*(EventCallback_ffif)callback)(*(float *)&data[0], *(float *)&data[1], data[2], *(float *)&data[3]);
        return true;
    case 0b100001100:
        using EventCallback_iiff = void (SignalObject::*)(const intptr_t, const intptr_t, const float, const float);
        (this->*(EventCallback_iiff)callback)(data[0], data[1], *(float *)&data[2], *(float *)&data[3]);
        return true;
    case 0b100001101:
        using EventCallback_fiff = void (SignalObject::*)(const float, const intptr_t, const float, const float);
        (this->*(EventCallback_fiff)callback)(*(float *)&data[0], data[1], *(float *)&data[2], *(float *)&data[3]);
        return true;
    case 0b100001110:
        using EventCallback_ifff = void (SignalObject::*)(const intptr_t, const float, const float, const float);
        (this->*(EventCallback_ifff)callback)(data[0], *(float *)&data[1], *(float *)&data[2], *(float *)&data[3]);
        return true;
    case 0b100001111:
        using EventCallback_ffff = void (SignalObject::*)(const float, const float, const float, const float);
        (this->*(EventCallback_ffff)callback)(*(float *)&data[0], *(float *)&data[1], *(float *)&data[2], *(float *)&data[3]);
        return true;
    //----------------------------------------------------------------------------------------------
    // 5 args
    //----------------------------------------------------------------------------------------------
    case 0b101000000:
        using EventCallback_iiiii = void (SignalObject::*)(const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t);
        (this->*(EventCallback_iiiii)callback)(data[0], data[1], data[2], data[3], data[4]);
        return true;
    case 0b101000001:
        using EventCallback_fiiii = void (SignalObject::*)(const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t);
        (this->*(EventCallback_fiiii)callback)(*(float *)&data[0], data[1], data[2], data[3], data[4]);
        return true;
    case 0b101000010:
        using EventCallback_ifiii = void (SignalObject::*)(const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t);
        (this->*(EventCallback_ifiii)callback)(data[0], *(float *)&data[1], data[2], data[3], data[4]);
        return true;
    case 0b101000011:
        using EventCallback_ffiii = void (SignalObject::*)(const float, const float, const intptr_t, const intptr_t, const intptr_t);
        (this->*(EventCallback_ffiii)callback)(*(float *)&data[0], *(float *)&data[1], data[2], data[3], data[4]);
        return true;
    case 0b101000100:
        using EventCallback_iifii = void (SignalObject::*)(const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t);
        (this->*(EventCallback_iifii)callback)(data[0], data[1], *(float *)&data[2], data[3], data[4]);
        return true;
    case 0b101000101:
        using EventCallback_fifii = void (SignalObject::*)(const float, const intptr_t, const float, const intptr_t, const intptr_t);
        (this->*(EventCallback_fifii)callback)(*(float *)&data[0], data[1], *(float *)&data[2], data[3], data[4]);
        return true;
    case 0b101000110:
        using EventCallback_iffii = void (SignalObject::*)(const intptr_t, const float, const float, const intptr_t, const intptr_t);
        (this->*(EventCallback_iffii)callback)(data[0], *(float *)&data[1], *(float *)&data[2], data[3], data[4]);
        return true;
    case 0b101000111:
        using EventCallback_fffii = void (SignalObject::*)(const float, const float, const float, const intptr_t, const intptr_t);
        (this->*(EventCallback_fffii)callback)(*(float *)&data[0], *(float *)&data[1], *(float *)&data[2], data[3], data[4]);
        return true;
    case 0b101001000:
        using EventCallback_iiifi = void (SignalObject::*)(const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t);
        (this->*(EventCallback_iiifi)callback)(data[0], data[1], data[2], *(float *)&data[3], data[4]);
        return true;
    case 0b101001001:
        using EventCallback_fiifi = void (SignalObject::*)(const float, const intptr_t, const intptr_t, const float, const intptr_t);
        (this->*(EventCallback_fiifi)callback)(*(float *)&data[0], data[1], data[2], *(float *)&data[3], data[4]);
        return true;
    case 0b101001010:
        using EventCallback_ififi = void (SignalObject::*)(const intptr_t, const float, const intptr_t, const float, const intptr_t);
        (this->*(EventCallback_ififi)callback)(data[0], *(float *)&data[1], data[2], *(float *)&data[3], data[4]);
        return true;
    case 0b101001011:
        using EventCallback_ffifi = void (SignalObject::*)(const float, const float, const intptr_t, const float, const intptr_t);
        (this->*(EventCallback_ffifi)callback)(*(float *)&data[0], *(float *)&data[1], data[2], *(float *)&data[3], data[4]);
        return true;
    case 0b101001100:
        using EventCallback_iiffi = void (SignalObject::*)(const intptr_t, const intptr_t, const float, const float, const intptr_t);
        (this->*(EventCallback_iiffi)callback)(data[0], data[1], *(float *)&data[2], *(float *)&data[3], data[4]);
        return true;
    case 0b101001101:
        using EventCallback_fiffi = void (SignalObject::*)(const float, const intptr_t, const float, const float, const intptr_t);
        (this->*(EventCallback_fiffi)callback)(*(float *)&data[0], data[1], *(float *)&data[2], *(float *)&data[3], data[4]);
        return true;
    case 0b101001110:
        using EventCallback_ifffi = void (SignalObject::*)(const intptr_t, const float, const float, const float, const intptr_t);
        (this->*(EventCallback_ifffi)callback)(data[0], *(float *)&data[1], *(float *)&data[2], *(float *)&data[3], data[4]);
        return true;
    case 0b101001111:
        using EventCallback_ffffi = void (SignalObject::*)(const float, const float, const float, const float, const intptr_t);
        (this->*(EventCallback_ffffi)callback)(*(float *)&data[0], *(float *)&data[1], *(float *)&data[2], *(float *)&data[3], data[4]);
        return true;
    case 0b101010000:
        using EventCallback_iiiif = void (SignalObject::*)(const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float);
        (this->*(EventCallback_iiiif)callback)(data[0], data[1], data[2], data[3], *(float *)&data[4]);
        return true;
    case 0b101010001:
        using EventCallback_fiiif = void (SignalObject::*)(const float, const intptr_t, const intptr_t, const intptr_t, const float);
        (this->*(EventCallback_fiiif)callback)(*(float *)&data[0], data[1], data[2], data[3], *(float *)&data[4]);
        return true;
    case 0b101010010:
        using EventCallback_ifiif = void (SignalObject::*)(const intptr_t, const float, const intptr_t, const intptr_t, const float);
        (this->*(EventCallback_ifiif)callback)(data[0], *(float *)&data[1], data[2], data[3], *(float *)&data[4]);
        return true;
    case 0b101010011:
        using EventCallback_ffiif = void (SignalObject::*)(const float, const float, const intptr_t, const intptr_t, const float);
        (this->*(EventCallback_ffiif)callback)(*(float *)&data[0], *(float *)&data[1], data[2], data[3], *(float *)&data[4]);
        return true;
    case 0b101010100:
        using EventCallback_iifif = void (SignalObject::*)(const intptr_t, const intptr_t, const float, const intptr_t, const float);
        (this->*(EventCallback_iifif)callback)(data[0], data[1], *(float *)&data[2], data[3], *(float *)&data[4]);
        return true;
    case 0b101010101:
        using EventCallback_fifif = void (SignalObject::*)(const float, const intptr_t, const float, const intptr_t, const float);
        (this->*(EventCallback_fifif)callback)(*(float *)&data[0], data[1], *(float *)&data[2], data[3], *(float *)&data[4]);
        return true;
    case 0b101010110:
        using EventCallback_iffif = void (SignalObject::*)(const intptr_t, const float, const float, const intptr_t, const float);
        (this->*(EventCallback_iffif)callback)(data[0], *(float *)&data[1], *(float *)&data[2], data[3], *(float *)&data[4]);
        return true;
    case 0b101010111:
        using EventCallback_fffif = void (SignalObject::*)(const float, const float, const float, const intptr_t, const float);
        (this->*(EventCallback_fffif)callback)(*(float *)&data[0], *(float *)&data[1], *(float *)&data[2], data[3], *(float *)&data[4]);
        return true;
    case 0b101011000:
        using EventCallback_iiiff = void (SignalObject::*)(const intptr_t, const intptr_t, const intptr_t, const float, const float);
        (this->*(EventCallback_iiiff)callback)(data[0], data[1], data[2], *(float *)&data[3], *(float *)&data[4]);
        return true;
    case 0b101011001:
        using EventCallback_fiiff = void (SignalObject::*)(const float, const intptr_t, const intptr_t, const float, const float);
        (this->*(EventCallback_fiiff)callback)(*(float *)&data[0], data[1], data[2], *(float *)&data[3], *(float *)&data[4]);
        return true;
    case 0b101011010:
        using EventCallback_ififf = void (SignalObject::*)(const intptr_t, const float, const intptr_t, const float, const float);
        (this->*(EventCallback_ififf)callback)(data[0], *(float *)&data[1], data[2], *(float *)&data[3], *(float *)&data[4]);
        return true;
    case 0b101011011:
        using EventCallback_ffiff = void (SignalObject::*)(const float, const float, const intptr_t, const float, const float);
        (this->*(EventCallback_ffiff)callback)(*(float *)&data[0], *(float *)&data[1], data[2], *(float *)&data[3], *(float *)&data[4]);
        return true;
    case 0b101011100:
        using EventCallback_iifff = void (SignalObject::*)(const intptr_t, const intptr_t, const float, const float, const float);
        (this->*(EventCallback_iifff)callback)(data[0], data[1], *(float *)&data[2], *(float *)&data[3], *(float *)&data[4]);
        return true;
    case 0b101011101:
        using EventCallback_fifff = void (SignalObject::*)(const float, const intptr_t, const float, const float, const float);
        (this->*(EventCallback_fifff)callback)(*(float *)&data[0], data[1], *(float *)&data[2], *(float *)&data[3], *(float *)&data[4]);
        return true;
    case 0b101011110:
        using EventCallback_iffff = void (SignalObject::*)(const intptr_t, const float, const float, const float, const float);
        (this->*(EventCallback_iffff)callback)(data[0], *(float *)&data[1], *(float *)&data[2], *(float *)&data[3], *(float *)&data[4]);
        return true;
    case 0b101011111:
        using EventCallback_fffff = void (SignalObject::*)(const float, const float, const float, const float, const float);
        (this->*(EventCallback_fffff)callback)(*(float *)&data[0], *(float *)&data[1], *(float *)&data[2], *(float *)&data[3], *(float *)&data[4]);
        return true;
    //----------------------------------------------------------------------------------------------
    // 6 args
    //----------------------------------------------------------------------------------------------
    case 0b110000000:
        using EventCallback_iiiiii = void (SignalObject::*)(const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t);
        (this->*(EventCallback_iiiiii)callback)(data[0], data[1], data[2], data[3], data[4], data[5]);
        return true;
    case 0b110000001:
        using EventCallback_fiiiii = void (SignalObject::*)(const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t);
        (this->*(EventCallback_fiiiii)callback)(*(float *)&data[0], data[1], data[2], data[3], data[4], data[5]);
        return true;
    case 0b110000010:
        using EventCallback_ifiiii = void (SignalObject::*)(const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t);
        (this->*(EventCallback_ifiiii)callback)(data[0], *(float *)&data[1], data[2], data[3], data[4], data[5]);
        return true;
    case 0b110000011:
        using EventCallback_ffiiii = void (SignalObject::*)(const float, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t);
        (this->*(EventCallback_ffiiii)callback)(*(float *)&data[0], *(float *)&data[1], data[2], data[3], data[4], data[5]);
        return true;
    case 0b110000100:
        using EventCallback_iifiii = void (SignalObject::*)(const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t);
        (this->*(EventCallback_iifiii)callback)(data[0], data[1], *(float *)&data[2], data[3], data[4], data[5]);
        return true;
    case 0b110000101:
        using EventCallback_fifiii = void (SignalObject::*)(const float, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t);
        (this->*(EventCallback_fifiii)callback)(*(float *)&data[0], data[1], *(float *)&data[2], data[3], data[4], data[5]);
        return true;
    case 0b110000110:
        using EventCallback_iffiii = void (SignalObject::*)(const intptr_t, const float, const float, const intptr_t, const intptr_t, const intptr_t);
        (this->*(EventCallback_iffiii)callback)(data[0], *(float *)&data[1], *(float *)&data[2], data[3], data[4], data[5]);
        return true;
    case 0b110000111:
        using EventCallback_fffiii = void (SignalObject::*)(const float, const float, const float, const intptr_t, const intptr_t, const intptr_t);
        (this->*(EventCallback_fffiii)callback)(*(float *)&data[0], *(float *)&data[1], *(float *)&data[2], data[3], data[4], data[5]);
        return true;
    case 0b110001000:
        using EventCallback_iiifii = void (SignalObject::*)(const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t);
        (this->*(EventCallback_iiifii)callback)(data[0], data[1], data[2], *(float *)&data[3], data[4], data[5]);
        return true;
    case 0b110001001:
        using EventCallback_fiifii = void (SignalObject::*)(const float, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t);
        (this->*(EventCallback_fiifii)callback)(*(float *)&data[0], data[1], data[2], *(float *)&data[3], data[4], data[5]);
        return true;
    case 0b110001010:
        using EventCallback_ififii = void (SignalObject::*)(const intptr_t, const float, const intptr_t, const float, const intptr_t, const intptr_t);
        (this->*(EventCallback_ififii)callback)(data[0], *(float *)&data[1], data[2], *(float *)&data[3], data[4], data[5]);
        return true;
    case 0b110001011:
        using EventCallback_ffifii = void (SignalObject::*)(const float, const float, const intptr_t, const float, const intptr_t, const intptr_t);
        (this->*(EventCallback_ffifii)callback)(*(float *)&data[0], *(float *)&data[1], data[2], *(float *)&data[3], data[4], data[5]);
        return true;
    case 0b110001100:
        using EventCallback_iiffii = void (SignalObject::*)(const intptr_t, const intptr_t, const float, const float, const intptr_t, const intptr_t);
        (this->*(EventCallback_iiffii)callback)(data[0], data[1], *(float *)&data[2], *(float *)&data[3], data[4], data[5]);
        return true;
    case 0b110001101:
        using EventCallback_fiffii = void (SignalObject::*)(const float, const intptr_t, const float, const float, const intptr_t, const intptr_t);
        (this->*(EventCallback_fiffii)callback)(*(float *)&data[0], data[1], *(float *)&data[2], *(float *)&data[3], data[4], data[5]);
        return true;
    case 0b110001110:
        using EventCallback_ifffii = void (SignalObject::*)(const intptr_t, const float, const float, const float, const intptr_t, const intptr_t);
        (this->*(EventCallback_ifffii)callback)(data[0], *(float *)&data[1], *(float *)&data[2], *(float *)&data[3], data[4], data[5]);
        return true;
    case 0b110001111:
        using EventCallback_ffffii = void (SignalObject::*)(const float, const float, const float, const float, const intptr_t, const intptr_t);
        (this->*(EventCallback_ffffii)callback)(*(float *)&data[0], *(float *)&data[1], *(float *)&data[2], *(float *)&data[3], data[4], data[5]);
        return true;
    case 0b110010000:
        using EventCallback_iiiifi = void (SignalObject::*)(const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t);
        (this->*(EventCallback_iiiifi)callback)(data[0], data[1], data[2], data[3], *(float *)&data[4], data[5]);
        return true;
    case 0b110010001:
        using EventCallback_fiiifi = void (SignalObject::*)(const float, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t);
        (this->*(EventCallback_fiiifi)callback)(*(float *)&data[0], data[1], data[2], data[3], *(float *)&data[4], data[5]);
        return true;
    case 0b110010010:
        using EventCallback_ifiifi = void (SignalObject::*)(const intptr_t, const float, const intptr_t, const intptr_t, const float, const intptr_t);
        (this->*(EventCallback_ifiifi)callback)(data[0], *(float *)&data[1], data[2], data[3], *(float *)&data[4], data[5]);
        return true;
    case 0b110010011:
        using EventCallback_ffiifi = void (SignalObject::*)(const float, const float, const intptr_t, const intptr_t, const float, const intptr_t);
        (this->*(EventCallback_ffiifi)callback)(*(float *)&data[0], *(float *)&data[1], data[2], data[3], *(float *)&data[4], data[5]);
        return true;
    case 0b110010100:
        using EventCallback_iififi = void (SignalObject::*)(const intptr_t, const intptr_t, const float, const intptr_t, const float, const intptr_t);
        (this->*(EventCallback_iififi)callback)(data[0], data[1], *(float *)&data[2], data[3], *(float *)&data[4], data[5]);
        return true;
    case 0b110010101:
        using EventCallback_fififi = void (SignalObject::*)(const float, const intptr_t, const float, const intptr_t, const float, const intptr_t);
        (this->*(EventCallback_fifiii)callback)(*(float *)&data[0], data[1], *(float *)&data[2], data[3], *(float *)&data[4], data[5]);
        return true;
    case 0b110010110:
        using EventCallback_iffifi = void (SignalObject::*)(const intptr_t, const float, const float, const intptr_t, const float, const intptr_t);
        (this->*(EventCallback_iffifi)callback)(data[0], *(float *)&data[1], *(float *)&data[2], data[3], *(float *)&data[4], data[5]);
        return true;
    case 0b110010111:
        using EventCallback_fffifi = void (SignalObject::*)(const float, const float, const float, const intptr_t, const float, const intptr_t);
        (this->*(EventCallback_fffifi)callback)(*(float *)&data[0], *(float *)&data[1], *(float *)&data[2], data[3], *(float *)&data[4], data[5]);
        return true;
    case 0b110011000:
        using EventCallback_iiiffi = void (SignalObject::*)(const intptr_t, const intptr_t, const intptr_t, const float, const float, const intptr_t);
        (this->*(EventCallback_iiiffi)callback)(data[0], data[1], data[2], *(float *)&data[3], *(float *)&data[4], data[5]);
        return true;
    case 0b110011001:
        using EventCallback_fiiffi = void (SignalObject::*)(const float, const intptr_t, const intptr_t, const float, const float, const intptr_t);
        (this->*(EventCallback_fiiffi)callback)(*(float *)&data[0], data[1], data[2], *(float *)&data[3], *(float *)&data[4], data[5]);
        return true;
    case 0b110011010:
        using EventCallback_ififfi = void (SignalObject::*)(const intptr_t, const float, const intptr_t, const float, const float, const intptr_t);
        (this->*(EventCallback_ififfi)callback)(data[0], *(float *)&data[1], data[2], *(float *)&data[3], *(float *)&data[4], data[5]);
        return true;
    case 0b110011011:
        using EventCallback_ffiffi = void (SignalObject::*)(const float, const float, const intptr_t, const float, const float, const intptr_t);
        (this->*(EventCallback_ffiffi)callback)(*(float *)&data[0], *(float *)&data[1], data[2], *(float *)&data[3], *(float *)&data[4], data[5]);
        return true;
    case 0b110011100:
        using EventCallback_iifffi = void (SignalObject::*)(const intptr_t, const intptr_t, const float, const float, const float, const intptr_t);
        (this->*(EventCallback_iifffi)callback)(data[0], data[1], *(float *)&data[2], *(float *)&data[3], *(float *)&data[4], data[5]);
        return true;
    case 0b110011101:
        using EventCallback_fifffi = void (SignalObject::*)(const float, const intptr_t, const float, const float, const float, const intptr_t);
        (this->*(EventCallback_fifffi)callback)(*(float *)&data[0], data[1], *(float *)&data[2], *(float *)&data[3], *(float *)&data[4], data[5]);
        return true;
    case 0b110011110:
        using EventCallback_iffffi = void (SignalObject::*)(const intptr_t, const float, const float, const float, const float, const intptr_t);
        (this->*(EventCallback_iffffi)callback)(data[0], *(float *)&data[1], *(float *)&data[2], *(float *)&data[3], *(float *)&data[4], data[5]);
        return true;
    case 0b110011111:
        using EventCallback_fffffi = void (SignalObject::*)(const float, const float, const float, const float, const float, const intptr_t);
        (this->*(EventCallback_fffffi)callback)(*(float *)&data[0], *(float *)&data[1], *(float *)&data[2], *(float *)&data[3], *(float *)&data[4], data[5]);
        return true;
    case 0b110100000:
        using EventCallback_iiiiif = void (SignalObject::*)(const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float);
        (this->*(EventCallback_iiiiif)callback)(data[0], data[1], data[2], data[3], data[4], *(float *)&data[5]);
        return true;
    case 0b110100001:
        using EventCallback_fiiiif = void (SignalObject::*)(const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float);
        (this->*(EventCallback_fiiiif)callback)(*(float *)&data[0], data[1], data[2], data[3], data[4], *(float *)&data[5]);
        return true;
    case 0b110100010:
        using EventCallback_ifiiif = void (SignalObject::*)(const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const float);
        (this->*(EventCallback_ifiiif)callback)(data[0], *(float *)&data[1], data[2], data[3], data[4], *(float *)&data[5]);
        return true;
    case 0b110100011:
        using EventCallback_ffiiif = void (SignalObject::*)(const float, const float, const intptr_t, const intptr_t, const intptr_t, const float);
        (this->*(EventCallback_ffiiif)callback)(*(float *)&data[0], *(float *)&data[1], data[2], data[3], data[4], *(float *)&data[5]);
        return true;
    case 0b110100100:
        using EventCallback_iifiif = void (SignalObject::*)(const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const float);
        (this->*(EventCallback_iifiif)callback)(data[0], data[1], *(float *)&data[2], data[3], data[4], *(float *)&data[5]);
        return true;
    case 0b110100101:
        using EventCallback_fifiif = void (SignalObject::*)(const float, const intptr_t, const float, const intptr_t, const intptr_t, const float);
        (this->*(EventCallback_fifiif)callback)(*(float *)&data[0], data[1], *(float *)&data[2], data[3], data[4], *(float *)&data[5]);
        return true;
    case 0b110100110:
        using EventCallback_iffiif = void (SignalObject::*)(const intptr_t, const float, const float, const intptr_t, const intptr_t, const float);
        (this->*(EventCallback_iffiif)callback)(data[0], *(float *)&data[1], *(float *)&data[2], data[3], data[4], *(float *)&data[5]);
        return true;
    case 0b110100111:
        using EventCallback_fffiif = void (SignalObject::*)(const float, const float, const float, const intptr_t, const intptr_t, const float);
        (this->*(EventCallback_fffiif)callback)(*(float *)&data[0], *(float *)&data[1], *(float *)&data[2], data[3], data[4], *(float *)&data[5]);
        return true;
    case 0b110101000:
        using EventCallback_iiifif = void (SignalObject::*)(const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const float);
        (this->*(EventCallback_iiifif)callback)(data[0], data[1], data[2], *(float *)&data[3], data[4], *(float *)&data[5]);
        return true;
    case 0b110101001:
        using EventCallback_fiifif = void (SignalObject::*)(const float, const intptr_t, const intptr_t, const float, const intptr_t, const float);
        (this->*(EventCallback_fiifif)callback)(*(float *)&data[0], data[1], data[2], *(float *)&data[3], data[4], *(float *)&data[5]);
        return true;
    case 0b110101010:
        using EventCallback_ififif = void (SignalObject::*)(const intptr_t, const float, const intptr_t, const float, const intptr_t, const float);
        (this->*(EventCallback_ififif)callback)(data[0], *(float *)&data[1], data[2], *(float *)&data[3], data[4], *(float *)&data[5]);
        return true;
    case 0b110101011:
        using EventCallback_ffifif = void (SignalObject::*)(const float, const float, const intptr_t, const float, const intptr_t, const float);
        (this->*(EventCallback_ffifif)callback)(*(float *)&data[0], *(float *)&data[1], data[2], *(float *)&data[3], data[4], *(float *)&data[5]);
        return true;
    case 0b110101100:
        using EventCallback_iiffif = void (SignalObject::*)(const intptr_t, const intptr_t, const float, const float, const intptr_t, const float);
        (this->*(EventCallback_iiffif)callback)(data[0], data[1], *(float *)&data[2], *(float *)&data[3], data[4], *(float *)&data[5]);
        return true;
    case 0b110101101:
        using EventCallback_fiffif = void (SignalObject::*)(const float, const intptr_t, const float, const float, const intptr_t, const float);
        (this->*(EventCallback_fiffif)callback)(*(float *)&data[0], data[1], *(float *)&data[2], *(float *)&data[3], data[4], *(float *)&data[5]);
        return true;
    case 0b110101110:
        using EventCallback_ifffif = void (SignalObject::*)(const intptr_t, const float, const float, const float, const intptr_t, const float);
        (this->*(EventCallback_ifffif)callback)(data[0], *(float *)&data[1], *(float *)&data[2], *(float *)&data[3], data[4], *(float *)&data[5]);
        return true;
    case 0b110101111:
        using EventCallback_ffffif = void (SignalObject::*)(const float, const float, const float, const float, const intptr_t, const float);
        (this->*(EventCallback_ffffif)callback)(*(float *)&data[0], *(float *)&data[1], *(float *)&data[2], *(float *)&data[3], data[4], *(float *)&data[5]);
        return true;
    case 0b110110000:
        using EventCallback_iiiiff = void (SignalObject::*)(const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const float);
        (this->*(EventCallback_iiiiff)callback)(data[0], data[1], data[2], data[3], *(float *)&data[4], *(float *)&data[5]);
        return true;
    case 0b110110001:
        using EventCallback_fiiiff = void (SignalObject::*)(const float, const intptr_t, const intptr_t, const intptr_t, const float, const float);
        (this->*(EventCallback_fiiiff)callback)(*(float *)&data[0], data[1], data[2], data[3], *(float *)&data[4], *(float *)&data[5]);
        return true;
    case 0b110110010:
        using EventCallback_ifiiff = void (SignalObject::*)(const intptr_t, const float, const intptr_t, const intptr_t, const float, const float);
        (this->*(EventCallback_ifiiff)callback)(data[0], *(float *)&data[1], data[2], data[3], *(float *)&data[4], *(float *)&data[5]);
        return true;
    case 0b110110011:
        using EventCallback_ffiiff = void (SignalObject::*)(const float, const float, const intptr_t, const intptr_t, const float, const float);
        (this->*(EventCallback_ffiiff)callback)(*(float *)&data[0], *(float *)&data[1], data[2], data[3], *(float *)&data[4], *(float *)&data[5]);
        return true;
    case 0b110110100:
        using EventCallback_iififf = void (SignalObject::*)(const intptr_t, const intptr_t, const float, const intptr_t, const float, const float);
        (this->*(EventCallback_iififf)callback)(data[0], data[1], *(float *)&data[2], data[3], *(float *)&data[4], *(float *)&data[5]);
        return true;
    case 0b110110101:
        using EventCallback_fififf = void (SignalObject::*)(const float, const intptr_t, const float, const intptr_t, const float, const float);
        (this->*(EventCallback_fifiif)callback)(*(float *)&data[0], data[1], *(float *)&data[2], data[3], *(float *)&data[4], *(float *)&data[5]);
        return true;
    case 0b110110110:
        using EventCallback_iffiff = void (SignalObject::*)(const intptr_t, const float, const float, const intptr_t, const float, const float);
        (this->*(EventCallback_iffiff)callback)(data[0], *(float *)&data[1], *(float *)&data[2], data[3], *(float *)&data[4], *(float *)&data[5]);
        return true;
    case 0b110110111:
        using EventCallback_fffiff = void (SignalObject::*)(const float, const float, const float, const intptr_t, const float, const float);
        (this->*(EventCallback_fffiff)callback)(*(float *)&data[0], *(float *)&data[1], *(float *)&data[2], data[3], *(float *)&data[4], *(float *)&data[5]);
        return true;
    case 0b110111000:
        using EventCallback_iiifff = void (SignalObject::*)(const intptr_t, const intptr_t, const intptr_t, const float, const float, const float);
        (this->*(EventCallback_iiifff)callback)(data[0], data[1], data[2], *(float *)&data[3], *(float *)&data[4], *(float *)&data[5]);
        return true;
    case 0b110111001:
        using EventCallback_fiifff = void (SignalObject::*)(const float, const intptr_t, const intptr_t, const float, const float, const float);
        (this->*(EventCallback_fiifff)callback)(*(float *)&data[0], data[1], data[2], *(float *)&data[3], *(float *)&data[4], *(float *)&data[5]);
        return true;
    case 0b110111010:
        using EventCallback_ififff = void (SignalObject::*)(const intptr_t, const float, const intptr_t, const float, const float, const float);
        (this->*(EventCallback_ififff)callback)(data[0], *(float *)&data[1], data[2], *(float *)&data[3], *(float *)&data[4], *(float *)&data[5]);
        return true;
    case 0b110111011:
        using EventCallback_ffifff = void (SignalObject::*)(const float, const float, const intptr_t, const float, const float, const float);
        (this->*(EventCallback_ffifff)callback)(*(float *)&data[0], *(float *)&data[1], data[2], *(float *)&data[3], *(float *)&data[4], *(float *)&data[5]);
        return true;
    case 0b110111100:
        using EventCallback_iiffff = void (SignalObject::*)(const intptr_t, const intptr_t, const float, const float, const float, const float);
        (this->*(EventCallback_iiffff)callback)(data[0], data[1], *(float *)&data[2], *(float *)&data[3], *(float *)&data[4], *(float *)&data[5]);
        return true;
    case 0b110111101:
        using EventCallback_fiffff = void (SignalObject::*)(const float, const intptr_t, const float, const float, const float, const float);
        (this->*(EventCallback_fiffff)callback)(*(float *)&data[0], data[1], *(float *)&data[2], *(float *)&data[3], *(float *)&data[4], *(float *)&data[5]);
        return true;
    case 0b110111110:
        using EventCallback_ifffff = void (SignalObject::*)(const intptr_t, const float, const float, const float, const float, const float);
        (this->*(EventCallback_ifffff)callback)(data[0], *(float *)&data[1], *(float *)&data[2], *(float *)&data[3], *(float *)&data[4], *(float *)&data[5]);
        return true;
    case 0b110111111:
        using EventCallback_ffffff = void (SignalObject::*)(const float, const float, const float, const float, const float, const float);
        (this->*(EventCallback_ffffff)callback)(*(float *)&data[0], *(float *)&data[1], *(float *)&data[2], *(float *)&data[3], *(float *)&data[4], *(float *)&data[5]);
        return true;
    default:
        BE_WARNLOG(L"Invalid formatSpec on signal '%hs'\n", sigdef->GetName());
        break;
    }

    return false;
}

bool SignalObject::BlockSignals(bool block) {
    bool prev = signalBlocked;
    signalBlocked = block;
    return prev;
}

BE_NAMESPACE_END
