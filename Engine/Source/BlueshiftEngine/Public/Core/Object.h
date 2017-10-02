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

/*
-------------------------------------------------------------------------------

    Object

    Provides fast run-time type checking and run-time instancing of objects.

-------------------------------------------------------------------------------
*/

#include "Core/Guid.h"
#include "Math/Math.h"
#include "Containers/Hierarchy.h"
#include "Event.h"
#include "SignalObject.h"
#include "Property.h"
#include "Serializable.h"

BE_NAMESPACE_BEGIN

class CmdArgs;
class PropertySpec;
class Object;
class EventDef;

using EventCallback = void (Object::*)();

template <typename T>
struct EventInfo {
    const EventDef *    eventDef;
    EventCallback       function;
};

extern const EventDef   EV_Destroy;
extern const EventDef   EV_ImmediateDestroy;

// 이 매크로는 Object class 를 상속받는 추상 클래스의 prototype 에 선언해야 한다.
// 객체화하는데 필요한 type 정보와 run-time type checking 기능을 제공한다. 
// 반드시 단일 상속 abstract class 에만 사용할 것
#define ABSTRACT_PROTOTYPE(classname) \
    using Class = classname; \
    classname(const classname &rhs) = delete; \
    classname &operator=(const classname &rhs) = delete; \
    static void RegisterProperties(); \
    virtual BE1::MetaObject *GetMetaObject() const; \
    static BE1::Object *CreateInstance(const BE1::Guid &guid = BE1::Guid()); \
    static BE1::MetaObject metaObject; \
    static BE1::PropertySpec pspecMap[]; \
    static BE1::EventInfo<classname> eventMap[]

// 이 매크로는 Object class 를 상속받는 자식 클래스의 prototype 에 선언해야 한다.
// 객체화하는데 필요한 type 정보와 run-time type checking 기능을 제공한다.
// 반드시 단일 상속 concrete class 에만 사용할 것
#define OBJECT_PROTOTYPE(classname) \
    using Class = classname; \
    classname(const classname &rhs) = delete; \
    classname &operator=(const classname &rhs) = delete; \
    static void RegisterProperties(); \
    virtual BE1::MetaObject *GetMetaObject() const override; \
    static BE1::Object *CreateInstance(const BE1::Guid &guid = BE1::Guid()); \
    static BE1::MetaObject metaObject; \
    static BE1::PropertySpec pspecMap[]; \
    static BE1::EventInfo<classname> eventMap[]

// 이 매크로는 static 멤버 변수를 초기화할 수 있는 곳에 정의되어야 한다.
// abstract 클래스에만 사용할 것
#define ABSTRACT_DECLARATION(visualname, classname, superclassname) \
    BE1::MetaObject *classname::GetMetaObject() const { return &(classname::metaObject); } \
    BE1::Object *classname::CreateInstance(const BE1::Guid &guid) { \
        BE_LOG(L"Cannot instanciate abstract class %hs.", #classname); \
        return nullptr; \
    } \
    BE1::MetaObject classname::metaObject(visualname, #classname, #superclassname, \
        classname::CreateInstance, classname::pspecMap, (BE1::EventInfo<BE1::Object> *)classname::eventMap);

// 이 매크로는 static 멤버 변수를 초기화할 수 있는 곳에 정의되어야 한다.
// concrete 클래스에만 사용할 것
#define OBJECT_DECLARATION(visualname, classname, superclassname) \
    BE1::MetaObject *classname::GetMetaObject() const { return &(classname::metaObject); } \
    BE1::Object *classname::CreateInstance(const BE1::Guid &guid) { \
        classname *ptr = new classname; \
        ptr->InitInstance(guid); \
        return ptr; \
    } \
    BE1::MetaObject classname::metaObject(visualname, #classname, #superclassname, \
        classname::CreateInstance, classname::pspecMap, (BE1::EventInfo<BE1::Object> *)classname::eventMap);

// event definition
#define BEGIN_EVENTS(classname) BE1::EventInfo<classname> classname::eventMap[] = {
#define EVENT(event, function) { &(event), (void (Object::*)())(&function) }
#define END_EVENTS { nullptr, nullptr } };

/// Run-time type information
class BE_API MetaObject {
    friend class Object;

public:
    MetaObject(const char *visualname, const char *classname, const char *superclassname, 
        Object *(*CreateInstance)(const Guid &guid), PropertySpec *pspecMap, EventInfo<Object> *eventMap);
    ~MetaObject();

    void                        Init();
    void                        Shutdown();

                                /// Returns visual name.
    const char *                VisualName() const { return visualname; }

                                /// Returns class name.
    const char *                ClassName() const { return classname; }

                                /// Returns super class name.
    const char *                SuperClassName() const { return superclassname; }

                                /// Create object instance with GUID.
                                /// If guid is given default, new GUID will be created with
    Object *                    CreateInstance(const Guid &guid = Guid::zero) const { return funcCreateInstance(guid); }

                                /// Tests if this meta object is compatible with supermeta.
    bool                        IsTypeOf(const MetaObject &supermeta) const { return ((hierarchyIndex >= supermeta.hierarchyIndex) && (hierarchyIndex <= supermeta.lastChildIndex)); }

                                /// Tests if corresponding event def is registered.
    bool                        IsRespondsTo(const EventDef &evdef) const;

                                /// Finds property spec including parent meta object.
    const PropertySpec *        FindPropertySpec(const char *name) const;

                                /// Returns property spec list including parent meta object.
    void                        GetPropertySpecList(Array<const PropertySpec *> &pspecs) const;

                                /// 
    PropertySpec &              RegisterProperty(const PropertySpec &propertySpec);

private:
    const char *                visualname;
    const char *                classname;
    const char *                superclassname;
    Object *                    (*funcCreateInstance)(const Guid &guid);
    
    Hierarchy<MetaObject>       node;               // hierarchy node
    int                         hierarchyIndex;     // hierarchy index (depth-first-order): Hierarchy::GetNext() 참고
    int                         lastChildIndex;     // 마지막 grandchild node hierarchy index
    MetaObject *                super;
    MetaObject *                next;

    PropertySpec *              pspecMap;
    Array<PropertySpec>         pspecs;
    HashIndex                   pspecHash;

    EventInfo<Object> *         eventMap;
    EventCallback *             eventCallbacks;
    bool                        freeEventCallbacks;
};

class BE_API Object : public SignalObject {
public:
    ABSTRACT_PROTOTYPE(Object);

    Object() {}
    virtual ~Object();

    const char *                ClassName() const;
    const char *                SuperClassName() const;

    virtual const Str           ToString() const { return ""; }
    
    template <typename T>
    T *                         Cast();
    template <typename T> 
    const T *                   Cast() const;

                                // superclass 의 grand child class 인지 체크
    bool                        IsTypeOf(const MetaObject &superclass) const;
    template <typename T>
    bool                        IsTypeOf() const { return IsTypeOf(T::metaObject); }

                                // klass 의 instance 인지 체크
    bool                        IsInstanceOf(const MetaObject &klass) const;
    template <typename T>
    bool                        IsInstanceOf() const { return IsInstanceOf(T::metaObject); }

                                // eventDef 에 해당하는 Callback 이 등록되어 있는지 체크
    bool                        IsRespondsTo(const EventDef &evdef) const;

                                /// Returns GUID of the object
    const Guid &                GetGuid() const { return guid; }

                                /// Returns instance ID of the object
    int                         GetInstanceID() const { return instanceID; }

                                /// Post event
    template <typename... Args>
    bool                        PostEvent(const EventDef *evdef, Args&&... args);

                                /// Post event that will be executed after given milliseconds
    template <typename... Args>
    bool                        PostEventMS(const EventDef *evdef, int milliseconds, Args&&... args);

                                /// Post event that will be executed after given seconds
    template <typename... Args>
    bool                        PostEventSec(const EventDef *evdef, int seconds, Args&&... args);

    template <typename... Args>
    bool                        ProcessEvent(const EventDef *evdef, Args&&... args);
    bool                        ProcessEventArgPtr(const EventDef *evdef, intptr_t *data);

    void                        CancelEvents(const EventDef *evdef);

    const PropertySpec *        FindPropertySpec(const char *name) const;

    Properties *                GetProperties() const { return props; }

    virtual void                GetPropertySpecList(Array<const PropertySpec *> &pspecs) const { GetMetaObject()->GetPropertySpecList(pspecs); }
    
    static void                 Init();
    static void                 Shutdown();

    static MetaObject *         GetMetaObject(const char *name);

    static Object *             CreateInstance(const char *name, const Guid &guid = Guid::zero);
    static void                 DestroyInstance(Object *instance, bool immediate = false);
    static void                 DestroyInstanceImmediate(Object *instance) { DestroyInstance(instance, true); }
    static Object *             FindInstance(const Guid &guid);
   
    static void                 ListClasses(const CmdArgs &args);

protected:
    void                        InitInstance(Guid guid);

    virtual void                Event_ImmediateDestroy();

    Properties *                props;
    Guid                        guid;
    int                         instanceID;

private:
    bool                        PostEventArgs(const EventDef *evdef, int time, int numArgs, ...);
    bool                        ProcessEventArgs(const EventDef *evdef, int numArgs, ...);

    void                        Event_Destroy();

    static bool                 initialized;
    static Array<MetaObject *>  types; // alphabetical order
};

BE_INLINE bool Object::IsTypeOf(const MetaObject &superclass) const {
    MetaObject *selfclass = GetMetaObject();
    return selfclass->IsTypeOf(superclass);
}

BE_INLINE bool Object::IsInstanceOf(const MetaObject &klass) const {
    MetaObject *selfclass = GetMetaObject();
    return selfclass == &klass;
}

template <typename T>
BE_INLINE T *Object::Cast() {
    if (!GetMetaObject()->IsTypeOf(T::metaObject)) {
        return nullptr;
    }

    return static_cast<T *>(this);
}

template <typename T>
BE_INLINE const T *Object::Cast() const {
    if (!GetMetaObject()->IsTypeOf(T::metaObject)) {
        return nullptr;
    }

    return static_cast<const T *>(this);
}

template <typename... Args>
BE_INLINE bool Object::PostEvent(const EventDef *evdef, Args&&... args) {
    static_assert(is_assignable_all<EventArg, Args...>::value, "args is not assignable to EventArg");
    return PostEventArgs(evdef, 0, sizeof...(args), address_of(EventArg(std::forward<Args>(args)))...);
}

template <typename... Args>
BE_INLINE bool Object::PostEventMS(const EventDef *evdef, int milliseconds, Args&&... args) {
    static_assert(is_assignable_all<EventArg, Args...>::value, "args is not assignable to EventArg");
    return PostEventArgs(evdef, milliseconds, sizeof...(args), address_of(EventArg(std::forward<Args>(args)))...);
}

template <typename... Args>
BE_INLINE bool Object::PostEventSec(const EventDef *evdef, int seconds, Args&&... args) {
    static_assert(is_assignable_all<EventArg, Args...>::value, "args is not assignable to EventArg");
    return PostEventArgs(evdef, SEC2MS(seconds), sizeof...(args), address_of(EventArg(std::forward<Args>(args)))...);
}

template <typename... Args>
BE_INLINE bool Object::ProcessEvent(const EventDef *evdef, Args&&... args) {
    static_assert(is_assignable_all<EventArg, Args...>::value, "args is not assignable to EventArg");
    return ProcessEventArgs(evdef, sizeof...(args), address_of(EventArg(std::forward<Args>(args)))...);
}

BE_NAMESPACE_END
