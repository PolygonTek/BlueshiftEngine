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
#include "Main/Common.h"
#include "Core/Object.h"
#include "Core/Heap.h"

BE_NAMESPACE_BEGIN

static const int    MaxEventStringLen   = 128;
static const int    MaxEventsPerFrame   = 4096;

static bool         eventError = false;
static char         eventErrorMsg[128];

EventDef *          EventDef::eventDefList[EventDef::MaxEvents];
int                 EventDef::numEventDefs = 0;

EventDef::EventDef(const char *command, bool guiEvent, const char *formatSpec, char returnType) {
    assert(command);
    assert(!Event::initialized);

    // Allow nullptr to indicate no args, but always store it as ""
    // so we don't have to check for it.
    if (!formatSpec) {
        formatSpec = "";
    }

    this->name          = command;
    this->formatSpec    = formatSpec;
    this->guiEvent      = guiEvent;
    this->returnType    = returnType;
    this->numArgs       = (int)strlen(formatSpec);
    assert(this->numArgs <= EventArg::MaxArgs);

    if (this->numArgs > EventArg::MaxArgs) {
        eventError = true;
        ::sprintf(eventErrorMsg, "EventDef::EventDef : Too many args for '%s' event.", command);
        return;
    }

    // Calculate the offsets for each arg
    memset(this->argOffset, 0, sizeof(this->argOffset));
    this->argSize = 0;

    for (int i = 0; i < this->numArgs; i++)	{
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
            this->argSize += MaxEventStringLen * sizeof(char);
            break;
        case EventArg::WStringType:
            this->argSize += MaxEventStringLen * sizeof(wchar_t);
            break;
        case EventArg::PointerType:
            this->argSize += sizeof(void *);
            break;
        default:
            eventError = true;
            ::sprintf(eventErrorMsg, "EventDef::EventDef : Invalid arg format '%s' string for '%s' event.", formatSpec, command);
            return;
            break;
        }
    }

    for (int i = 0; i < this->numEventDefs; i++) {
        EventDef *evdef = this->eventDefList[i];

        if (!Str::Cmp(command, evdef->name)) {
            // 같은 이름이지만 formatSpec 이 다른 경우
            if (Str::Cmp(formatSpec, evdef->formatSpec)) {
                eventError = true;
                ::sprintf(eventErrorMsg, "Event '%s' defined twice with same name but differing format strings ('%s'!='%s').", command, formatSpec, evdef->formatSpec);
                return;
            }

            // 같은 이름이지만 returnType 이 다른 경우 
            if (evdef->returnType != returnType) {
                eventError = true;
                ::sprintf(eventErrorMsg, "Event '%s' defined twice with same name but differing return types ('%c'!='%c').", command, returnType, evdef->returnType);
                return;
            }

            this->eventnum = evdef->eventnum;
            return;
        }
    }

    if (this->numEventDefs >= MaxEvents) {
        eventError = true;
        ::sprintf(eventErrorMsg, "numEventDefs >= MaxEvents");
        return;
    }

    this->eventnum = this->numEventDefs;
    this->eventDefList[this->numEventDefs] = this;
    this->numEventDefs++;
}

int EventDef::NumEvents() {
    return numEventDefs;
}

const EventDef *EventDef::GetEvent(int eventnum) {
    return eventDefList[eventnum];
}

const EventDef *EventDef::FindEvent(const char *name) {
    assert(name);

    int num = numEventDefs;
    for (int i = 0; i < num; i++) {
        EventDef *ev = eventDefList[i];
        if (!Str::Cmp(name, ev->name)) {
            return ev;
        }
    }

    return nullptr;
}

//-----------------------------------------------------------------------------------------

bool                Event::initialized = false;
LinkList<Event>     Event::freeEvents;
LinkList<Event>     Event::eventQueue;
LinkList<Event>     Event::guiEventQueue;
Event               Event::eventPool[EventDef::MaxEvents];

Event::~Event() {
    Free();
}

void Event::ClearEventList() {
    freeEvents.Clear();
    eventQueue.Clear();

    for (int i = 0; i < EventDef::MaxEvents; i++) {
        eventPool[i].Free();
    }
}

void Event::Init() {
    BE_LOG(L"Initializing event system\n");

    if (eventError) {
        BE_ERRLOG(L"%hs", eventErrorMsg);
    }

    if (initialized) {
        BE_LOG(L"...already initialized\n");
        ClearEventList();
        return;
    }

    ClearEventList();

    BE_LOG(L"...%i event definitions\n", EventDef::NumEvents());

    // the event system has started
    initialized = true;
}

void Event::Shutdown() {
    BE_LOG(L"Shutdown event system\n");

    if (!initialized) {
        BE_LOG(L"...not started\n");
        return;
    }

    ClearEventList();

    initialized = false;
}

void Event::Free() {
    if (data) {
        Mem_Free(data);
        data = nullptr;
    }

    this->eventDef = nullptr;
    this->time = 0;
    this->sender = nullptr;

    node.SetOwner(this);
    node.AddToEnd(Event::freeEvents);
}

Event *Event::Alloc(const EventDef *evdef, int numArgs, va_list args) {
    if (freeEvents.IsListEmpty()) {
        BE_ERRLOG(L"Event::Alloc: No more free events\n");
    }

    Event *ev = freeEvents.Next();
    ev->node.Remove();
    ev->eventDef = evdef;

    if (numArgs != evdef->GetNumArgs()) {
        BE_ERRLOG(L"Event::Alloc: Wrong number of args for '%hs' event.\n", evdef->GetName());
    }

    size_t size = evdef->GetArgSize();
    if (size) {
        ev->data = (byte *)Mem_Alloc(size);
        memset(ev->data, 0, size);
    } else {
        ev->data = nullptr;
    }

    // Copy arguments to event data
    const char *format = evdef->GetArgFormat();
    for (int i = 0; i < numArgs; i++) {
        EventArg *arg = va_arg(args, EventArg *);
        if (format[i] != arg->type) {
            BE_ERRLOG(L"Event::Alloc: Wrong type passed in for arg # %d on '%hs' event.\n", i, evdef->GetName());
        }

        byte *dataPtr = &ev->data[evdef->GetArgOffset(i)];

        switch (format[i]) {
        case EventArg::FloatType:
            if (arg->pointer) {
                *reinterpret_cast<float *>(dataPtr) = *reinterpret_cast<const float *>(arg->pointer);
            }
            break;
        case EventArg::IntType:
            if (arg->pointer) {
                *reinterpret_cast<int *>(dataPtr) = *reinterpret_cast<const int *>(arg->pointer);
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
                Str::Copynz(reinterpret_cast<char *>(dataPtr), reinterpret_cast<const char *>(arg->pointer), MaxEventStringLen);
            }
            break;
        case EventArg::WStringType:
            if (arg->pointer) {
                WStr::Copynz(reinterpret_cast<wchar_t *>(dataPtr), reinterpret_cast<const wchar_t *>(arg->pointer), MaxEventStringLen);
            }
            break;
        case EventArg::PointerType:
            *reinterpret_cast<void **>(dataPtr) = reinterpret_cast<void *>(arg->pointer);
            break;
        default:
            BE_ERRLOG(L"Event::Alloc: Invalid arg format '%hs' string for '%hs' event.\n", format, evdef->GetName());
            break;
        }
    }

    return ev;
}

void Event::CopyArgPtrs(const EventDef *evdef, int numArgs, va_list args, intptr_t argPtrs[EventArg::MaxArgs]) {
    const char *format = evdef->GetArgFormat();
    if (numArgs != evdef->GetNumArgs()) {
        BE_ERRLOG(L"Event::CopyArgPtrs: Wrong number of args for '%hs' event.\n", evdef->GetName());
    }

    for (int i = 0; i < numArgs; i++) {
        EventArg *arg = va_arg(args, EventArg *);
        if (format[i] != arg->type) {
            BE_ERRLOG(L"Event::CopyArgPtrs: Wrong type passed in for arg # %d on '%hs' event.\n", i, evdef->GetName());
        }

        argPtrs[i] = arg->pointer;
    }
}

void Event::Schedule(Object *sender, int time) {
    assert(initialized);
    if (!initialized) {
        return;
    }

    this->sender = sender;
    this->time = common.realTime + time;

    node.Remove();

    LinkList<Event> &queue = eventDef->IsGuiEvent() ? guiEventQueue : eventQueue;

    // event queue 는 시간 순으로 정렬되어 있다.
    Event *event = queue.Next();
    while (event && (this->time >= event->time)) {
        event = event->node.Next();
    }

    if (event) {
        node.InsertBefore(event->node);
    } else {
        node.AddToEnd(queue);
    }
}

void Event::CancelEvents(const Object *sender, const EventDef *evdef) {
    if (!initialized) {
        return;
    }

    LinkList<Event> &queue = evdef->IsGuiEvent() ? guiEventQueue : eventQueue;
        
    Event *next;
    for (Event *event = queue.Next(); event != nullptr; event = next) {
        next = event->node.Next();
        if (event->sender == sender) {
            if (!evdef || (evdef == event->eventDef)) {
                event->Free();
            }
        }
    }
}

void Event::ServiceEvent(Event *event) {
    intptr_t argPtrs[EventArg::MaxArgs];

    // copy the data into the local argPtrs array and set up pointers
    const EventDef *evdef = event->eventDef;
    const char *formatSpec = evdef->GetArgFormat();

    int numArgs = evdef->GetNumArgs();

    for (int i = 0; i < numArgs; i++) {
        int offset = evdef->GetArgOffset(i);
        byte *data = event->data;

        switch (formatSpec[i]) {
        case EventArg::FloatType:
            *reinterpret_cast<int **>(&argPtrs[i]) = reinterpret_cast<int *>(&data[offset]);
            break;
        case EventArg::IntType:
            *reinterpret_cast<float **>(&argPtrs[i]) = reinterpret_cast<float *>(&data[offset]);
            break;
        case EventArg::PointType:
            *reinterpret_cast<Point **>(&argPtrs[i]) = reinterpret_cast<Point *>(&data[offset]);
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
            BE_ERRLOG(L"Event::ServiceEvent : Invalid arg format '%hs' string for '%hs' event.\n", formatSpec, evdef->GetName());
        }
    }

    // the event is removed from its list so that if then object
    // is deleted, the event won't be freed twice
    event->node.Remove();

    assert(event->sender);
    event->sender->ProcessEventArgPtr(evdef, argPtrs);

    // return the event to the free list
    event->Free();
}

void Event::ServiceEvents() {
    int num = 0;

    while (!eventQueue.IsListEmpty()) {
        Event *ev = eventQueue.Next();
        assert(ev);

        if (ev->time > common.realTime) {
            break;
        }

        ServiceEvent(ev);

        // Don't allow ourselves to stay in here too long.  An abnormally high number
        // of events being processed is evidence of an infinite loop of events.
        num++;
        if (num > MaxEventsPerFrame) {
            BE_ERRLOG(L"Event overflow.  Possible infinite loop in script.\n");
        }
    }
}

void Event::ServiceGuiEvents() {
    int num = 0;

    while (!guiEventQueue.IsListEmpty()) {
        Event *ev = guiEventQueue.Next();
        assert(ev);

        if (ev->time > common.realTime) {
            break;
        }

        ServiceEvent(ev);

        // Don't allow ourselves to stay in here too long.  An abnormally high number
        // of events being processed is evidence of an infinite loop of events.
        num++;
        if (num > MaxEventsPerFrame) {
            BE_ERRLOG(L"Event overflow.  Possible infinite loop in script.\n");
        }
    }
}

BE_NAMESPACE_END
