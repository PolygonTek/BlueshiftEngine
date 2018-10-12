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
#include "Containers/Hierarchy.h"
#include "Containers/HashIndex.h"
#include "Event.h"
#include "Property.h"
#include "Serializable.h"

BE_NAMESPACE_BEGIN

class CmdArgs;
class PropertyInfo;
class Object;
class EventDef;

using ObjectPtr = Object*;
using ObjectPtrArray = Array<ObjectPtr>;

using EventCallback = void (Object::*)();

template <typename T>
struct EventInfo {
    const EventDef *eventDef;
    EventCallback function;
};

extern const EventDef EV_Destroy;
extern const EventDef EV_ImmediateDestroy;

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
    static BE1::EventInfo<classname> eventMap[]

// 이 매크로는 static 멤버 변수를 초기화할 수 있는 곳에 정의되어야 한다.
// abstract 클래스에만 사용할 것
#define ABSTRACT_DECLARATION(visualname, classname, superclassname) \
    BE1::MetaObject *classname::GetMetaObject() const { return &classname::metaObject; } \
    BE1::Object *classname::CreateInstance(const BE1::Guid &guid) { \
        BE_LOG(L"Cannot instanciate abstract class %hs.", #classname); \
        return nullptr; \
    } \
    BE1::MetaObject classname::metaObject(visualname, #classname, #superclassname, \
        classname::CreateInstance, (BE1::EventInfo<BE1::Object> *)classname::eventMap);

// 이 매크로는 static 멤버 변수를 초기화할 수 있는 곳에 정의되어야 한다.
// concrete 클래스에만 사용할 것
#define OBJECT_DECLARATION(visualname, classname, superclassname) \
    BE1::MetaObject *classname::GetMetaObject() const { return &classname::metaObject; } \
    BE1::Object *classname::CreateInstance(const BE1::Guid &guid) { \
        classname *ptr = new classname; \
        if (!ptr->InitInstance(guid)) return nullptr;\
        return ptr; \
    } \
    BE1::MetaObject classname::metaObject(visualname, #classname, #superclassname, \
        classname::CreateInstance, (BE1::EventInfo<BE1::Object> *)classname::eventMap);

// event definition
#define BEGIN_EVENTS(classname) BE1::EventInfo<classname> classname::eventMap[] = {
#define EVENT(event, function) { &(event), (void (Object::*)())(&function) }
#define END_EVENTS { nullptr, nullptr } };

/// Run-time type information
class BE_API MetaObject {
    friend class Object;

public:
    MetaObject(const char *visualname, const char *classname, const char *superclassname, Object *(*CreateInstance)(const Guid &guid), EventInfo<Object> *eventMap);
    ~MetaObject();

    void                        Init();
    void                        Shutdown();

                                /// Returns visual name.
    const char *                VisualName() const { return visualname; }

                                /// Returns class name.
    const char *                ClassName() const { return classname; }

                                /// Returns super class name.
    const char *                SuperClassName() const { return superclassname; }

                                /// Create object instance with given GUID.
    Object *                    CreateInstance(const Guid &guid) const { return funcCreateInstance(guid); }

                                /// Create object instance with new GUID.
    Object *                    CreateInstance() const { return funcCreateInstance(Guid::zero); }

                                /// Tests if this meta object is compatible with supermeta.
    bool                        IsTypeOf(const MetaObject &supermeta) const { return ((hierarchyIndex >= supermeta.hierarchyIndex) && (hierarchyIndex <= supermeta.lastChildIndex)); }

                                /// Tests if corresponding event def is registered.
    bool                        IsRespondsTo(const EventDef &evdef) const;

                                /// Finds property info including parent meta object.
    bool                        GetPropertyInfo(const char *name, PropertyInfo &propertyInfo) const;

                                /// Returns property info list including parent meta object.
    void                        GetPropertyInfoList(Array<PropertyInfo> &propertyInfoList) const;

                                /// 
    PropertyInfo &              RegisterProperty(const PropertyInfo &propertyInfo);

private:
    const char *                visualname;
    const char *                classname;
    const char *                superclassname;
    Object *                    (*funcCreateInstance)(const Guid &guid);
    
    Hierarchy<MetaObject>       node;               // hierarchy node
    int                         hierarchyIndex;     // hierarchy index (depth-first-order): Hierarchy::GetNext() 참고
    int                         lastChildIndex;     // last grandchild node hierarchy index
    MetaObject *                super;
    MetaObject *                next;

    Array<PropertyInfo>         propertyInfoList;
    HashIndex                   propertyInfoHash;

    EventInfo<Object> *         eventMap;
    EventCallback *             eventCallbacks;
    bool                        freeEventCallbacks;
};

class BE_API Object : public Serializable {
public:
    ABSTRACT_PROTOTYPE(Object);

    Object() = default;
    virtual ~Object();

                                /// Returns class name.
    Str                         ClassName() const;

                                /// Returns super class name.
    Str                         SuperClassName() const;

    virtual Str                 ToString() const { return ""; }
    
    template <typename T>
    T *                         Cast();
    template <typename T> 
    const T *                   Cast() const;

                                /// Returns true if meta object of this object is a meta object metaObject.
    bool                        IsTypeOf(const MetaObject &metaObject) const;
    template <typename T>
    bool                        IsTypeOf() const { return IsTypeOf(T::metaObject); }

                                /// Returns true if this object is a instance of meta object metaObject.
    bool                        IsInstanceOf(const MetaObject &metaObject) const;
    template <typename T>
    bool                        IsInstanceOf() const { return IsInstanceOf(T::metaObject); }

                                /// Returns true if this object can be respondable event evdef.
    bool                        IsRespondsTo(const EventDef &evdef) const;

                                /// Returns GUID of the object.
    const Guid &                GetGuid() const { return guid; }

                                /// Returns instance ID of the object.
    int                         GetInstanceID() const { return instanceID; }

                                /// Post a event.
    template <typename... Args>
    bool                        PostEvent(const EventDef *evdef, Args&&... args);

                                /// Post a event that will be executed after given milliseconds.
    template <typename... Args>
    bool                        PostEventMS(const EventDef *evdef, int milliseconds, Args&&... args);

                                /// Post a event that will be executed after given seconds.
    template <typename... Args>
    bool                        PostEventSec(const EventDef *evdef, int seconds, Args&&... args);

    template <typename... Args>
    bool                        ProcessEvent(const EventDef *evdef, Args&&... args);
    bool                        ProcessEventArgPtr(const EventDef *evdef, intptr_t *data);

                                /// Cancels a event in event queue.
    void                        CancelEvents(const EventDef *evdef);

                                /// Gets full list of property info.
    virtual void                GetPropertyInfoList(Array<PropertyInfo> &propertyInfoList) const { GetMetaObject()->GetPropertyInfoList(propertyInfoList); }
    
    static void                 Init();
    static void                 Shutdown();

                                /// Finds meta object with the given name.
    static MetaObject *         FindMetaObject(const char *name);

    static Object *             CreateInstance(const char *name, const Guid &guid = Guid::zero);
    static void                 DestroyInstance(Object *instance, bool immediate = false);
    static void                 DestroyInstanceImmediate(Object *instance) { DestroyInstance(instance, true); }
    static Object *             FindInstance(const Guid &guid);
   
    static void                 ListClasses(const CmdArgs &args);

protected:
    bool                        InitInstance(Guid guid);

    virtual void                Event_ImmediateDestroy();

private:
    void                        SetClassName(const Str &classname);

    bool                        PostEventArgs(const EventDef *evdef, int time, int numArgs, ...);
    bool                        ProcessEventArgs(const EventDef *evdef, int numArgs, ...);

    void                        Event_Destroy();

    Guid                        guid;
    int                         instanceID;

    static bool                 initialized;
    static Array<MetaObject *>  types;          // in alphabetical order
};

BE_INLINE bool Object::IsTypeOf(const MetaObject &metaObject) const {
    return GetMetaObject()->IsTypeOf(metaObject);
}

BE_INLINE bool Object::IsInstanceOf(const MetaObject &metaObject) const {
    return GetMetaObject() == &metaObject;
}

template <typename T>
BE_INLINE T *Object::Cast() {
    if (this == nullptr) {
        return nullptr;
    }
    if (!GetMetaObject()->IsTypeOf(T::metaObject)) {
        return nullptr;
    }
    return static_cast<T *>(this);
}

template <typename T>
BE_INLINE const T *Object::Cast() const {
    if (this == nullptr) {
        return nullptr;
    }
    if (!GetMetaObject()->IsTypeOf(T::metaObject)) {
        return nullptr;
    }
    return static_cast<const T *>(this);
}

template <typename... Args>
BE_INLINE bool Object::PostEvent(const EventDef *evdef, Args&&... args) {
    static_assert(is_assignable_all<VariantArg, Args...>::value, "args is not assignable to VariantArg");
    return PostEventArgs(evdef, 0, sizeof...(args), address_of(VariantArg(std::forward<Args>(args)))...);
}

template <typename... Args>
BE_INLINE bool Object::PostEventMS(const EventDef *evdef, int milliseconds, Args&&... args) {
    static_assert(is_assignable_all<VariantArg, Args...>::value, "args is not assignable to VariantArg");
    return PostEventArgs(evdef, milliseconds, sizeof...(args), address_of(VariantArg(std::forward<Args>(args)))...);
}

template <typename... Args>
BE_INLINE bool Object::PostEventSec(const EventDef *evdef, int seconds, Args&&... args) {
    static_assert(is_assignable_all<VariantArg, Args...>::value, "args is not assignable to VariantArg");
    return PostEventArgs(evdef, SEC2MS(seconds), sizeof...(args), address_of(VariantArg(std::forward<Args>(args)))...);
}

template <typename... Args>
BE_INLINE bool Object::ProcessEvent(const EventDef *evdef, Args&&... args) {
    static_assert(is_assignable_all<VariantArg, Args...>::value, "args is not assignable to VariantArg");
    return ProcessEventArgs(evdef, sizeof...(args), address_of(VariantArg(std::forward<Args>(args)))...);
}

BE_NAMESPACE_END
