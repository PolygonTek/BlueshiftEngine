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

static const int    MaxEventStringLen = 128;
static const int    MaxEventsPerFrame = 4096;

static bool         eventErrorOccured = false;
static char         eventErrorMsg[128];

EventDef *          EventDef::eventDefs[EventDef::MaxEvents];
int                 EventDef::numEventDefs = 0;

EventDef::EventDef(const char *name, bool guiEvent, const char *formatSpec, char returnType) {
    assert(name);
    assert(!Event::initialized);

    // Allow NULL to indicate no args, but always store it as ""
    // so we don't have to check for it.
    if (!formatSpec) {
        formatSpec = "";
    }

    this->name = name;
    this->formatSpec = formatSpec;
    this->returnType = returnType;
    this->numArgs = Str::Length(formatSpec);
    this->guiEvent = guiEvent;

    if (this->numArgs > EventArg::MaxArgs) {
        eventErrorOccured = true;
        ::sprintf(eventErrorMsg, "EventDef::EventDef: Too many args for '%s' event.", name);
        return;
    }

    // Calculate the offsets for each arguments
    memset(this->argOffset, 0, sizeof(this->argOffset));
    this->argSize = 0;

    for (int argIndex = 0; argIndex < this->numArgs; argIndex++) {
        this->argOffset[argIndex] = (int)this->argSize;

        switch (this->formatSpec[argIndex]) {
        case EventArg::IntType:
            this->argSize += sizeof(int);
            break;
        case EventArg::BoolType:
            this->argSize += sizeof(bool);
            break;
        case EventArg::FloatType:
            this->argSize += sizeof(float);
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
            eventErrorOccured = true;
            ::sprintf(eventErrorMsg, "EventDef::EventDef: Invalid arg format '%s' string for '%s' event.", formatSpec, name);
            return;
        }
    }

    // Check if same name event def exist
    for (int i = 0; i < this->numEventDefs; i++) {
        EventDef *evdef = this->eventDefs[i];

        if (!Str::Cmp(name, evdef->name)) {
            // Same name but different formatSpec
            if (Str::Cmp(formatSpec, evdef->formatSpec)) {
                eventErrorOccured = true;
                ::sprintf(eventErrorMsg, "Event '%s' defined twice with same name but differing format strings ('%s'!='%s').", name, formatSpec, evdef->formatSpec);
                return;
            }

            // Same name but different returnType
            if (evdef->returnType != returnType) {
                eventErrorOccured = true;
                ::sprintf(eventErrorMsg, "Event '%s' defined twice with same name but differing return types ('%c'!='%c').", name, returnType, evdef->returnType);
                return;
            }

            this->eventNum = evdef->eventNum;
            return;
        }
    }

    if (EventDef::numEventDefs >= MaxEvents) {
        eventErrorOccured = true;
        ::sprintf(eventErrorMsg, "numEventDefs >= MaxEvents");
        return;
    }

    this->eventNum = EventDef::numEventDefs;

    EventDef::eventDefs[EventDef::numEventDefs++] = this;
}

const EventDef *EventDef::FindEvent(const char *name) {
    assert(name);

    for (int i = 0; i < numEventDefs; i++) {
        EventDef *ev = eventDefs[i];
        if (!Str::Cmp(name, ev->name)) {
            return ev;
        }
    }
    return nullptr;
}

//-----------------------------------------------------------------------------------------

bool                Event::initialized = false;
Event               Event::eventPool[EventDef::MaxEvents];
LinkList<Event>     Event::freeEvents;
LinkList<Event>     Event::eventQueue;
LinkList<Event>     Event::guiEventQueue;

Event::~Event() {
    Free();
}

void Event::ClearEvents() {
    freeEvents.Clear();
    eventQueue.Clear();
    guiEventQueue.Clear();

    for (int i = 0; i < EventDef::MaxEvents; i++) {
        eventPool[i].Free();
    }
}

void Event::Init() {
    BE_LOG(L"Initializing event system\n");

    if (eventErrorOccured) {
        BE_ERRLOG(L"%hs", eventErrorMsg);
    }

    ClearEvents();

    if (initialized) {
        BE_LOG(L"...already initialized\n");
        return;
    }

    BE_LOG(L"...%i event definitions\n", EventDef::NumEvents());

    initialized = true;
}

void Event::Shutdown() {
    BE_LOG(L"Shutdown event system\n");

    if (!initialized) {
        BE_LOG(L"...not started\n");
        return;
    }

    ClearEvents();

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

    Event *newEvent = freeEvents.Next();
    newEvent->node.Remove();
    newEvent->eventDef = evdef;

    if (numArgs != evdef->GetNumArgs()) {
        BE_ERRLOG(L"Event::Alloc: Wrong number of args for '%hs' event.\n", evdef->GetName());
    }

    size_t size = evdef->GetArgSize();
    if (size > 0) {
        newEvent->data = (byte *)Mem_Alloc(size);
        memset(newEvent->data, 0, size);
    } else {
        newEvent->data = nullptr;
    }

    // Copy arguments to event data
    const char *format = evdef->GetArgFormat();
    for (int argIndex = 0; argIndex < numArgs; argIndex++) {
        const EventArg *arg = va_arg(args, EventArg *);

        if (arg->type != format[argIndex]) {
            BE_ERRLOG(L"Event::Alloc: Wrong type passed in for arg #%d on '%hs' event.\n", argIndex, evdef->GetName());
        }

        byte *dataPtr = &newEvent->data[evdef->GetArgOffset(argIndex)];

        switch (format[argIndex]) {
        case EventArg::IntType:
            if (arg->pointer) {
                *reinterpret_cast<int *>(dataPtr) = *reinterpret_cast<const int *>(arg->pointer);
            }
            break;
        case EventArg::BoolType:
            if (arg->pointer) {
                *reinterpret_cast<bool *>(dataPtr) = *reinterpret_cast<const bool *>(arg->pointer);
            }
            break;
        case EventArg::FloatType:
            if (arg->pointer) {
                *reinterpret_cast<float *>(dataPtr) = *reinterpret_cast<const float *>(arg->pointer);
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

    return newEvent;
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

    LinkList<Event> &queue = eventQueue; // evdef->IsGuiEvent() ? guiEventQueue : eventQueue;
        
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

    for (int argIndex = 0; argIndex < numArgs; argIndex++) {
        int offset = evdef->GetArgOffset(argIndex);
        byte *data = event->data;

        switch (formatSpec[argIndex]) {
        case EventArg::IntType:
            *reinterpret_cast<int **>(&argPtrs[argIndex]) = reinterpret_cast<int *>(&data[offset]);
            break;
        case EventArg::BoolType:
            *reinterpret_cast<bool **>(&argPtrs[argIndex]) = reinterpret_cast<bool *>(&data[offset]);
            break;
        case EventArg::FloatType:
            *reinterpret_cast<float **>(&argPtrs[argIndex]) = reinterpret_cast<float *>(&data[offset]);
            break;
        case EventArg::PointType:
            *reinterpret_cast<Point **>(&argPtrs[argIndex]) = reinterpret_cast<Point *>(&data[offset]);
            break;
        case EventArg::RectType:
            *reinterpret_cast<Rect **>(&argPtrs[argIndex]) = reinterpret_cast<Rect *>(&data[offset]);
            break;
        case EventArg::Vec3Type:
            *reinterpret_cast<Vec3 **>(&argPtrs[argIndex]) = reinterpret_cast<Vec3 *>(&data[offset]);
            break;
        case EventArg::Mat3x3Type:
            *reinterpret_cast<Mat3 **>(&argPtrs[argIndex]) = reinterpret_cast<Mat3 *>(&data[offset]);
            break;
        case EventArg::Mat4x4Type:
            *reinterpret_cast<Mat4 **>(&argPtrs[argIndex]) = reinterpret_cast<Mat4 *>(&data[offset]);
            break;
        case EventArg::StringType:
            *reinterpret_cast<const char **>(&argPtrs[argIndex]) = reinterpret_cast<const char *>(&data[offset]);
            break;
        case EventArg::WStringType:
            *reinterpret_cast<const wchar_t **>(&argPtrs[argIndex]) = reinterpret_cast<const wchar_t *>(&data[offset]);
            break;
        case EventArg::PointerType:
            *reinterpret_cast<void **>(&argPtrs[argIndex]) = *reinterpret_cast<void **>(&data[offset]);
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
    int processedCount = 0;

    while (!eventQueue.IsListEmpty()) {
        Event *ev = eventQueue.Next();
        assert(ev);

        if (ev->time > common.realTime) {
            break;
        }

        ServiceEvent(ev);

        // Don't allow ourselves to stay in here too long.  An abnormally high number
        // of events being processed is evidence of an infinite loop of events.
        processedCount++;
        if (processedCount > MaxEventsPerFrame) {
            BE_ERRLOG(L"Event overflow.  Possible infinite loop in script.\n");
        }
    }
}

void Event::ServiceGuiEvents() {
    int processedCount = 0;

    while (!guiEventQueue.IsListEmpty()) {
        Event *ev = guiEventQueue.Next();
        assert(ev);

        if (ev->time > common.realTime) {
            break;
        }

        ServiceEvent(ev);

        // Don't allow ourselves to stay in here too long.  An abnormally high number
        // of events being processed is evidence of an infinite loop of events.
        processedCount++;
        if (processedCount > MaxEventsPerFrame) {
            BE_ERRLOG(L"Event overflow.  Possible infinite loop in script.\n");
        }
    }
}

BE_NAMESPACE_END
