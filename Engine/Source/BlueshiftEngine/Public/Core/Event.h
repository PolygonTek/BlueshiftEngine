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

#include "Core/Str.h"
#include "Core/WStr.h"
#include "Containers/LinkList.h"

BE_NAMESPACE_BEGIN

class Point;
class Rect;
class Vec3;
class Mat3;
class Mat4;
class Object;
class Guid;

class BE_API EventArg {
public:
    enum { MaxArgs = 8 };

    static const char VoidType      = '\0';
    static const char IntType       = 'i';
    static const char BoolType      = 'b';
    static const char FloatType     = 'f';
    static const char PointType     = 'p';
    static const char RectType      = 'r';
    static const char Vec3Type      = 'v';
    static const char Mat3x3Type    = 'm';
    static const char Mat4x4Type    = 'M';
    static const char StringType    = 's';
    static const char WStringType   = 'w';
    static const char PointerType   = 'a';
    static const char GuidType      = 'g';

    EventArg() { type = IntType; pointer = 0; };
    EventArg(const int data) { type = IntType; intValue = data; };
    EventArg(const bool data) { type = BoolType; boolValue = data; };
    EventArg(const float data) { type = FloatType; floatValue = data; };
    EventArg(const void *data) { type = PointerType; pointer = reinterpret_cast<intptr_t>(data); };
    EventArg(const char *data) { type = StringType; pointer = reinterpret_cast<intptr_t>(data); };
    EventArg(const wchar_t *data) { type = WStringType; pointer = reinterpret_cast<intptr_t>(data); };
    EventArg(const Point &data) { type = PointType; pointer = reinterpret_cast<intptr_t>(&data); };
    EventArg(const Rect &data) { type = RectType; pointer = reinterpret_cast<intptr_t>(&data); };
    EventArg(const Vec3 &data) { type = Vec3Type; pointer = reinterpret_cast<intptr_t>(&data); };
    EventArg(const Mat3 &data) { type = Mat3x3Type; pointer = reinterpret_cast<intptr_t>(&data); };
    EventArg(const Mat4 &data) { type = Mat4x4Type; pointer = reinterpret_cast<intptr_t>(&data); };
    EventArg(const Guid &data) { type = GuidType; pointer = reinterpret_cast<intptr_t>(&data); }
    EventArg(const Str &data) { type = StringType; pointer = reinterpret_cast<intptr_t>(data.c_str()); };
    EventArg(const WStr &data) { type = WStringType; pointer = reinterpret_cast<intptr_t>(data.c_str()); };

    int                     type;
    union {
        intptr_t            pointer;        // pointer value
        bool                boolValue;      // boolean value
        int                 intValue;       // integer value
        float               floatValue;     // float value
    };
};

class BE_API EventDef {
public:
    enum { MaxEvents = 4096 };

    explicit EventDef(const char *name, bool guiEvent = false, const char *formatSpec = nullptr, char returnType = 0);
    /// Prevents copy constructor
    EventDef(const EventDef &rhs) = delete;

                            /// Returns event def name
    const char *            GetName() const { return name; }
    const char *            GetArgFormat() const { return formatSpec; }
    char                    GetReturnType() const { return returnType; }
    int                     GetEventNum() const { return eventNum; }
    int                     GetNumArgs() const { return numArgs; }
    size_t                  GetArgSize() const { return argSize; }
    int                     GetArgOffset(int arg) const { assert((arg >= 0) && (arg < EventArg::MaxArgs)); return argOffset[arg]; }
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
    int                     returnType;
    bool                    guiEvent;
    int                     numArgs;
    size_t                  argSize;
    int                     argOffset[EventArg::MaxArgs];
    int                     eventNum;

    static EventDef *       eventDefs[MaxEvents];
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
    static void             Init();
    static void             Shutdown();

    static void             Clear();

                            /// Create a new event with the given event def and arguments
    static Event *          AllocEvent(const EventDef *eventDef, int numArgs, va_list args);
    static void             FreeEvent(Event *event);

    static void             CopyArgPtrs(const EventDef *eventDef, int numArgs, va_list args, intptr_t data[EventArg::MaxArgs]);

    static void             ScheduleEvent(Event *event, Object *sender, int time);

                            /// Cancels a event which is posted by sender in event queue
    static void             CancelEvents(const Object *sender, const EventDef *eventDef = nullptr);

    static void             ServiceEvents();
    static void             ServiceGuiEvents();

    static bool             initialized;

private:
    static void             ServiceEvent(Event *event);

    static Event            eventPool[EventDef::MaxEvents];
    static LinkList<Event>  freeEvents;
    static LinkList<Event>  eventQueue;
    static LinkList<Event>  guiEventQueue;
};

BE_NAMESPACE_END
