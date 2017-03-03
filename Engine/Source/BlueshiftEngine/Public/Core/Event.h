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

class BE_API EventArg {
public:
    enum { MaxArgs = 8 };

    static const char VoidType      = '\0';
    static const char IntType       = 'i';
    static const char FloatType     = 'f';
    static const char PointType     = 'p';
    static const char RectType      = 'r';
    static const char Vec3Type      = 'v';
    static const char Mat3x3Type    = 'm';
    static const char Mat4x4Type    = 'M';
    static const char StringType    = 's';
    static const char WStringType   = 'w';
    static const char PointerType   = 'a';

    EventArg() { type = IntType; pointer = 0; };
    EventArg(const int data) { type = IntType; intValue = data; };
    EventArg(const float data) { type = FloatType; floatValue = data; };
    EventArg(const Point &data) { type = PointType; pointer = reinterpret_cast<intptr_t>(&data); };
    EventArg(const Rect &data) { type = RectType; pointer = reinterpret_cast<intptr_t>(&data); };
    EventArg(const Vec3 &data) { type = Vec3Type; pointer = reinterpret_cast<intptr_t>(&data); };
    EventArg(const Mat3 &data) { type = Mat3x3Type; pointer = reinterpret_cast<intptr_t>(&data); };
    EventArg(const Mat4 &data) { type = Mat4x4Type; pointer = reinterpret_cast<intptr_t>(&data); };
    EventArg(const Str &data) { type = StringType; pointer = reinterpret_cast<intptr_t>(data.c_str()); };
    EventArg(const char *data) { type = StringType; pointer = reinterpret_cast<intptr_t>(data); };
    EventArg(const WStr &data) { type = WStringType; pointer = reinterpret_cast<intptr_t>(data.c_str()); };
    EventArg(const wchar_t *data) { type = WStringType; pointer = reinterpret_cast<intptr_t>(data); };
    EventArg(const void *data) { type = PointerType; pointer = reinterpret_cast<intptr_t>(data); };

    int                     type;
    union {
        intptr_t            pointer;        // pointer value
        int                 intValue;       // integer value
        float               floatValue;     // float value
    };
};

class BE_API EventDef {
public:
    enum { MaxEvents = 4096 };

    explicit EventDef(const char *command, bool guiEvent = false, const char *formatSpec = nullptr, char returnType = 0);

    const char *            GetName() const { return name; }
    const char *            GetArgFormat() const { return formatSpec; }
    bool                    IsGuiEvent() const { return guiEvent; }
    char                    GetReturnType() const { return returnType; }
    int                     GetEventNum() const { return eventnum; }
    int                     GetNumArgs() const { return numArgs; }
    size_t                  GetArgSize() const { return argSize; }
    int                     GetArgOffset(int arg) const { assert((arg >= 0) && (arg < EventArg::MaxArgs)); return argOffset[arg]; }

    static int              NumEvents();
    static const EventDef * GetEvent(int eventnum);
    static const EventDef * FindEvent(const char *name);

private:
    const char *            name;
    const char *            formatSpec;
    int                     returnType;
    bool                    guiEvent;
    int                     numArgs;
    size_t                  argSize;
    int                     argOffset[EventArg::MaxArgs];
    int                     eventnum;

    static EventDef *       eventDefList[MaxEvents];
    static int              numEventDefs;
};

class BE_API Event {
public:
    ~Event();
    
    void                    Free();
    void                    Schedule(Object *sender, int time);
    byte *                  GetData() { return data; }

    static void             Init();
    static void             Shutdown();

    static Event *          Alloc(const EventDef *evdef, int numArgs, va_list args);
    static void             CopyArgPtrs(const EventDef *evdef, int numArgs, va_list args, intptr_t data[EventArg::MaxArgs]);

    static void             CancelEvents(const Object *sender, const EventDef *evdef = nullptr);
    static void             ServiceEvent(Event *event);
    static void             ServiceEvents();
    static void             ServiceGuiEvents();
    static void             ClearEventList();

    static bool             initialized;

private:
    const EventDef *        eventDef;
    byte *                  data;
    int                     time;
    Object *                sender;
    LinkList<Event>         node;

    static LinkList<Event>  freeEvents;
    static LinkList<Event>  eventQueue;
    static LinkList<Event>  guiEventQueue;
    static Event            eventPool[EventDef::MaxEvents];
};

BE_NAMESPACE_END
