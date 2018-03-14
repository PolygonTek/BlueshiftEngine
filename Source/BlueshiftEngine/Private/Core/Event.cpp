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
#include "Engine/Common.h"
#include "Core/Object.h"
#include "Core/Heap.h"

BE_NAMESPACE_BEGIN

static const int    MaxEventStringLen = 128;
static const int    MaxEventsPerFrame = 4096;

static bool         eventErrorOccured = false;
static char         eventErrorMsg[128];

EventDef *          EventDef::eventDefs[EventDef::MaxEventDefs];
int                 EventDef::numEventDefs = 0;

EventDef::EventDef(const char *name, bool guiEvent, const char *formatSpec, char returnType) {
    assert(name);
    assert(!EventSystem::initialized);

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

    if (this->numArgs > MaxArgs) {
        eventErrorOccured = true;
        ::sprintf(eventErrorMsg, "EventDef::EventDef: Too many args for '%s' event.", name);
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
            this->argSize += MaxEventStringLen * sizeof(char);
            break;
        case VariantArg::WStringType:
            this->argSize += MaxEventStringLen * sizeof(wchar_t);
            break;
        default:
            eventErrorOccured = true;
            ::sprintf(eventErrorMsg, "EventDef::EventDef: Invalid arg format '%s' string for '%s' event.", formatSpec, name);
            return;
        }
    }

    formatSpecBits = (numArgs << EventDef::MaxArgs) | argBits;

    // Check if same name event def already exist
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

    if (EventDef::numEventDefs >= EventDef::MaxEventDefs) {
        eventErrorOccured = true;
        ::sprintf(eventErrorMsg, "numEventDefs >= MaxEventDefs");
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

bool                EventSystem::initialized = false;
Event               EventSystem::eventPool[EventSystem::MaxEvents];
LinkList<Event>     EventSystem::freeEvents;
LinkList<Event>     EventSystem::eventQueue;
LinkList<Event>     EventSystem::guiEventQueue;

Event::~Event() {
    EventSystem::FreeEvent(this);
}

void EventSystem::Clear() {
    freeEvents.Clear();
    eventQueue.Clear();
    guiEventQueue.Clear();

    for (int i = 0; i < EventSystem::MaxEvents; i++) {
        FreeEvent(&eventPool[i]);
    }
}

void EventSystem::Init() {
    BE_LOG(L"Initializing event system\n");

    if (eventErrorOccured) {
        BE_ERRLOG(L"%hs", eventErrorMsg);
    }

    Clear();

    if (initialized) {
        BE_LOG(L"...already initialized\n");
        return;
    }

    BE_LOG(L"...%i event definitions\n", EventDef::NumEvents());

    initialized = true;
}

void EventSystem::Shutdown() {
    BE_LOG(L"Shutdown event system\n");

    if (!initialized) {
        BE_LOG(L"...not started\n");
        return;
    }

    Clear();

    initialized = false;
}

void EventSystem::FreeEvent(Event *event) {
    if (event->data) {
        Mem_Free(event->data);
        event->data = nullptr;
    }

    event->eventDef = nullptr;
    event->time = 0;
    event->sender = nullptr;

    event->node.SetOwner(event);
    event->node.AddToEnd(EventSystem::freeEvents);
}

Event *EventSystem::AllocEvent(const EventDef *evdef, int numArgs, va_list args) {
    if (freeEvents.IsListEmpty()) {
        BE_ERRLOG(L"Event::Alloc: No more free events\n");
    }

    Event *newEvent = freeEvents.Next();
    newEvent->node.Remove();
    newEvent->eventDef = evdef;

    if (numArgs != evdef->GetNumArgs()) {
        BE_ERRLOG(L"Event::AllocEvent: Wrong number of args for '%hs' event.\n", evdef->GetName());
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
        const VariantArg *arg = va_arg(args, VariantArg *);

        if (arg->type != format[argIndex]) {
            BE_ERRLOG(L"EventSystem::AllocEvent: Wrong type passed in for arg #%d on '%hs' event.\n", argIndex, evdef->GetName());
        }

        byte *dataPtr = &newEvent->data[evdef->GetArgOffset(argIndex)];

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
                Str::Copynz(reinterpret_cast<char *>(dataPtr), reinterpret_cast<const char *>(arg->pointer), MaxEventStringLen);
            }
            break;
        case VariantArg::WStringType:
            if (arg->pointer) {
                WStr::Copynz(reinterpret_cast<wchar_t *>(dataPtr), reinterpret_cast<const wchar_t *>(arg->pointer), MaxEventStringLen);
            }
            break;
        default:
            BE_ERRLOG(L"EventSystem::AllocEvent: Invalid arg format '%hs' string for '%hs' event.\n", format, evdef->GetName());
            break;
        }
    }

    return newEvent;
}

void EventSystem::CopyArgPtrs(const EventDef *evdef, int numArgs, va_list args, intptr_t argPtrs[EventDef::MaxArgs]) {
    if (numArgs != evdef->GetNumArgs()) {
        BE_ERRLOG(L"EventSystem::CopyArgPtrs: Wrong number of args for '%hs' event.\n", evdef->GetName());
    }

    const char *format = evdef->GetArgFormat();

    for (int argIndex = 0; argIndex < numArgs; argIndex++) {
        VariantArg *arg = va_arg(args, VariantArg *);
        if (format[argIndex] != arg->type) {
            BE_ERRLOG(L"EventSystem::CopyArgPtrs: Wrong type passed in for arg #%d on '%hs' event.\n", argIndex, evdef->GetName());
        }

        argPtrs[argIndex] = arg->pointer;
    }
}

void EventSystem::ScheduleEvent(Event *event, Object *sender, int time) {
    assert(initialized);
    if (!initialized) {
        return;
    }

    event->sender = sender;
    event->time = common.realTime + time;
    event->node.Remove();

    LinkList<Event> &queue = event->eventDef->IsGuiEvent() ? guiEventQueue : eventQueue;

    // event queue 는 시간 순으로 정렬되어 있다.
    Event *ev = queue.Next();
    while (ev && (event->time >= ev->time)) {
        ev = ev->node.Next();
    }

    if (ev) {
        event->node.InsertBefore(ev->node);
    } else {
        event->node.AddToEnd(queue);
    }
}

void EventSystem::CancelEvents(const Object *sender, const EventDef *evdef) {
    if (!initialized) {
        return;
    }

    LinkList<Event> &queue = eventQueue; // evdef->IsGuiEvent() ? guiEventQueue : eventQueue;
        
    Event *next;
    for (Event *event = queue.Next(); event != nullptr; event = next) {
        next = event->node.Next();
        if (event->sender == sender) {
            if (!evdef || (evdef == event->eventDef)) {
                FreeEvent(event);
            }
        }
    }
}

void EventSystem::ServiceEvent(Event *event) {
    intptr_t argPtrs[EventDef::MaxArgs];

    // copy the data into the local argPtrs array and set up pointers
    const EventDef *evdef = event->eventDef;
    const char *formatSpec = evdef->GetArgFormat();
    int numArgs = evdef->GetNumArgs();

    for (int argIndex = 0; argIndex < numArgs; argIndex++) {
        int offset = evdef->GetArgOffset(argIndex);
        byte *data = event->data;

        switch (formatSpec[argIndex]) {
        case VariantArg::IntType:
            *reinterpret_cast<int **>(&argPtrs[argIndex]) = reinterpret_cast<int *>(&data[offset]);
            break;
        case VariantArg::FloatType:
            *reinterpret_cast<float **>(&argPtrs[argIndex]) = reinterpret_cast<float *>(&data[offset]);
            break;
        case VariantArg::PointerType:
            *reinterpret_cast<void **>(&argPtrs[argIndex]) = *reinterpret_cast<void **>(&data[offset]);
            break;
        case VariantArg::PointType:
            *reinterpret_cast<Point **>(&argPtrs[argIndex]) = reinterpret_cast<Point *>(&data[offset]);
            break;
        case VariantArg::RectType:
            *reinterpret_cast<Rect **>(&argPtrs[argIndex]) = reinterpret_cast<Rect *>(&data[offset]);
            break;
        case VariantArg::Vec3Type:
            *reinterpret_cast<Vec3 **>(&argPtrs[argIndex]) = reinterpret_cast<Vec3 *>(&data[offset]);
            break;
        case VariantArg::Mat3x3Type:
            *reinterpret_cast<Mat3 **>(&argPtrs[argIndex]) = reinterpret_cast<Mat3 *>(&data[offset]);
            break;
        case VariantArg::Mat4x4Type:
            *reinterpret_cast<Mat4 **>(&argPtrs[argIndex]) = reinterpret_cast<Mat4 *>(&data[offset]);
            break;
        case VariantArg::GuidType:
            *reinterpret_cast<Guid **>(&argPtrs[argIndex]) = reinterpret_cast<Guid *>(&data[offset]);
            break;
        case VariantArg::StringType:
            *reinterpret_cast<const char **>(&argPtrs[argIndex]) = reinterpret_cast<const char *>(&data[offset]);
            break;
        case VariantArg::WStringType:
            *reinterpret_cast<const wchar_t **>(&argPtrs[argIndex]) = reinterpret_cast<const wchar_t *>(&data[offset]);
            break;
        default:
            BE_ERRLOG(L"EventSystem::ServiceEvent : Invalid arg format '%hs' string for '%hs' event.\n", formatSpec, evdef->GetName());
        }
    }

    // the event is removed from its list so that if then object
    // is deleted, the event won't be freed twice
    event->node.Remove();

    assert(event->sender);
    event->sender->ProcessEventArgPtr(evdef, argPtrs);

    // return the event to the free list
    FreeEvent(event);
}

void EventSystem::ServiceEvents() {
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

void EventSystem::ServiceGuiEvents() {
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
