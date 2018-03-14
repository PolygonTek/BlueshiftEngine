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

#include "Containers/LinkList.h"

BE_NAMESPACE_BEGIN

class Object;

class BE_API EventDef {
public:
    static const int MaxArgs = 6;
    static const int MaxEventDefs = 4096;

    explicit EventDef(const char *name, bool guiEvent = false, const char *formatSpec = nullptr, char returnType = 0);
    /// Prevents copy constructor
    EventDef(const EventDef &rhs) = delete;

                            /// Returns event def name
    const char *            GetName() const { return name; }
    const char *            GetArgFormat() const { return formatSpec; }
    char                    GetReturnType() const { return returnType; }
    int                     GetEventNum() const { return eventNum; }
    int                     GetNumArgs() const { return numArgs; }
    unsigned int            GetFormatSpecBits() const { return formatSpecBits; }
    size_t                  GetArgSize() const { return argSize; }
    int                     GetArgOffset(int arg) const { assert((arg >= 0) && (arg < MaxArgs)); return argOffset[arg]; }
    bool                    IsGuiEvent() const { return guiEvent; }

                            /// Returns number of event defs
    static int              NumEvents() { return numEventDefs; }

                            /// Returns event def by event number
    static const EventDef * GetEvent(int eventNum) { return eventDefs[eventNum]; }

                            /// Returns event def by name
    static const EventDef * FindEvent(const char *name);

private:
    const char *            name;
    const char *            formatSpec;
    unsigned int            formatSpecBits;
    int                     returnType;
    bool                    guiEvent;
    int                     numArgs;
    size_t                  argSize;
    int                     argOffset[MaxArgs];
    int                     eventNum;

    static EventDef *       eventDefs[MaxEventDefs];
    static int              numEventDefs;
};

class BE_API Event {
    friend class EventSystem;

public:
    Event() = default;
    ~Event();

    byte *                  GetData() { return data; }

private:
    const EventDef *        eventDef;
    byte *                  data;
    int                     time;
    Object *                sender;
    LinkList<Event>         node;
};

class BE_API EventSystem {
public:
    static const int MaxEvents = 4096;

    static void             Init();
    static void             Shutdown();

    static void             Clear();

                            /// Create a new event with the given event def and arguments
    static Event *          AllocEvent(const EventDef *eventDef, int numArgs, va_list args);
    static void             FreeEvent(Event *event);

    static void             CopyArgPtrs(const EventDef *eventDef, int numArgs, va_list args, intptr_t data[EventDef::MaxArgs]);

    static void             ScheduleEvent(Event *event, Object *sender, int time);

                            /// Cancels a event which is posted by sender in event queue
    static void             CancelEvents(const Object *sender, const EventDef *eventDef = nullptr);

    static void             ServiceEvents();
    static void             ServiceGuiEvents();

    static bool             initialized;

private:
    static void             ServiceEvent(Event *event);

    static Event            eventPool[MaxEvents];
    static LinkList<Event>  freeEvents;
    static LinkList<Event>  eventQueue;
    static LinkList<Event>  guiEventQueue;
};

BE_NAMESPACE_END
