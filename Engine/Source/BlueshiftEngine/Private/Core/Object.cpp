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
#include "Platform/PlatformAtomic.h"
#include "Core/Object.h"
#include "Core/Heap.h"
#include "Containers/HashTable.h"
#include "Core/Cmds.h"

BE_NAMESPACE_BEGIN

static MetaObject *             staticTypeList = nullptr;
static Hierarchy<MetaObject>    classHierarchy;
static int                      eventCallbackMemory = 0;

#ifdef NEW_PROPERTY_SYSTEM
MetaObject::MetaObject(const char *visualname, const char *classname, const char *superclassname, 
    Object *(*CreateInstance)(const Guid &guid), EventInfo<Object> *eventMap) {
#else
MetaObject::MetaObject(const char *visualname, const char *classname, const char *superclassname,
    Object *(*CreateInstance)(const Guid &guid), PropertyInfo *propertyInfos, EventInfo<Object> *eventMap) {
#endif
    this->visualname            = visualname;
    this->classname             = classname;
    this->superclassname        = superclassname;
    this->super                 = Object::FindMetaObject(superclassname);
    this->hierarchyIndex        = 0;
    this->lastChildIndex        = 0;
    this->funcCreateInstance    = CreateInstance;
#ifndef NEW_PROPERTY_SYSTEM
    this->propertyInfos         = propertyInfos;
#endif
    this->eventMap              = eventMap;
    this->eventCallbacks        = nullptr;
    this->freeEventCallbacks    = false;

    // 자식 class 의 MetaObject 가 먼저 생성되었다면 자식 클래스를 찾아서 super 를 지정해준다
    for (MetaObject *type = staticTypeList; type; type = type->next) {
        if (!type->super && !Str::Cmp(type->superclassname, classname)) {
            if (Str::Cmp(type->classname, "Object")) {
                type->super = this;
            }
        }
    }

    // classname 을 알파벳 순서로 링크드리스트에 소팅하면서 insert
    MetaObject **insert;
    for (insert = &staticTypeList; *insert; insert = &(*insert)->next) {
        int cmpResult = Str::Cmp(classname, (*insert)->classname);
        
        // classname conflicts
        assert(cmpResult != 0);

        if (cmpResult < 0) {
            next = *insert;
            *insert = this;
            break;
        }
    }

    if (!*insert) {
        *insert = this;
        next = nullptr;
    }
}

MetaObject::~MetaObject() {
    Shutdown();
}

bool MetaObject::IsRespondsTo(const EventDef &ev) const {
    assert(Event::initialized);
    if (!eventCallbacks[ev.GetEventNum()]) {
        return false;
    }
    return true;
}

// MetaObject 초기화 함수. Object::Init 에서 불린다.
void MetaObject::Init() {
    if (eventCallbacks) {
        return;
    }

    // 부모 클래스 먼저 Init
    if (super && !super->node.Owner()) {
        super->Init();
    }

    // hierarchy node 세팅
    if (super) {
        node.SetParent(super->node);
    } else {
        node.SetParent(classHierarchy);
    }
    
    node.SetOwner(this);

#ifndef NEW_PROPERTY_SYSTEM
    // property info array 의 hash 를 작성
    PropertyInfo *propInfo = propertyInfos;
    for (int i = 0; propInfo->flags != PropertyInfo::Empty; i++, propInfo++) {
        int hash = propertyInfoHash.GenerateHash(propInfo->name, false);
        propertyInfoHash.Add(hash, i);
    }
#endif

    // 각 클래스별로 child 노드 개수를 lastChildIndex 에 담는다
    for (MetaObject *t = super; t != nullptr; t = t->super) {
        t->lastChildIndex++;
    }

    // 따로 event callback 이 없다면 부모것으로 세팅 후 리턴
    if ((!eventMap || !eventMap->eventDef) && super) {
        eventCallbacks = super->eventCallbacks;
        return;
    }

    // 여기까지 왔다면 event map 을 새로 할당 할거니까, 나중에 삭제할거라고 표시해둔다
    freeEventCallbacks = true;

    // 전체 event 개수만큼 event callback 함수 메모리 할당
    int num = EventDef::NumEvents();
    eventCallbacks = (EventCallback *)Mem_Alloc(num * sizeof(EventCallback));
    memset(eventCallbacks, 0, sizeof(EventCallback) * num);

    eventCallbackMemory += sizeof(EventCallback) * num;

    // allocate temporary memory for flags so that the subclass's event callbacks
    // override the superclass's event callback
    bool *set = (bool *)Mem_Alloc(num * sizeof(bool));
    memset(set, 0, sizeof(bool) * num);

    // go through the inheritence order and copies the event callback function into
    // a list indexed by the event number.  This allows fast lookups of event functions.
    for (MetaObject *t = this; t != nullptr; t = t->super) {
        EventInfo<Object> *info = t->eventMap;
        if (!info) {
            continue;
        }

        for (int i = 0; info[i].eventDef != nullptr; i++) {
            int ev = info[i].eventDef->GetEventNum();
            if (set[ev]) {
                continue;
            }

            set[ev] = true;
            eventCallbacks[ev] = info[i].function;
        }
    }

    Mem_Free(set);
}

void MetaObject::Shutdown() {
    // free up the memory used for event lookups
    if (eventCallbacks) {
        if (freeEventCallbacks) {
            Mem_Free(eventCallbacks);
        }
        eventCallbacks = nullptr;
    }
    
    node.SetOwner(nullptr);
    node.RemoveFromHierarchy();

    hierarchyIndex = 0;
    lastChildIndex = 0;
}

bool MetaObject::GetPropertyInfo(const char *name, PropertyInfo &propertyInfo) const {
    if (!name || !name[0]) {
        return false;
    }

    int hash = propertyInfoHash.GenerateHash(name, false);
    
    for (const MetaObject *t = this; t != nullptr; t = t->super) {
        for (int i = t->propertyInfoHash.First(hash); i != -1; i = t->propertyInfoHash.Next(i)) {
            if (!Str::Icmp(t->propertyInfos[i].name, name)) {
                propertyInfo = t->propertyInfos[i];
                return true;
            }
        }
    }

    return false;
}

void MetaObject::GetPropertyInfoList(Array<PropertyInfo> &propertyInfos) const {
    Array<Str> names;

    for (const MetaObject *t = this; t != nullptr; t = t->super) {
#ifdef NEW_PROPERTY_SYSTEM
        for (int index = 0; index < t->propertyInfos.Count(); index++) {
            const PropertyInfo &propInfo = t->propertyInfos[index];
            const char *propName = propInfo.GetName();

            if (!names.Find(propName)) {
                names.Append(propName);
                propertyInfos.Append(propInfo);
            }
        }
#else
        for (const PropertyInfo *propInfo = t->propertyInfos; propInfo->flags != PropertyInfo::Empty; propInfo++) {
            const char *propName = propInfo->GetName();

            if (!names.Find(propName)) {
                names.Append(propName);
                propertyInfos.Append(*propInfo);
            }
        }
#endif
    }
}

#ifdef NEW_PROPERTY_SYSTEM 

PropertyInfo &MetaObject::RegisterProperty(const PropertyInfo &propInfo) {
    int index = propertyInfos.Append(propInfo);
    int hash = propertyInfoHash.GenerateHash(propInfo.GetName(), false);
    propertyInfoHash.Add(hash, index);
    return propertyInfos[index];
}

#endif

//-----------------------------------------------------------------------------------------------

const EventDef EV_ImmediateDestroy("<immediatedestroy>");
const EventDef EV_Destroy("destroy");

ABSTRACT_DECLARATION("Object", Object, nullptr)

BEGIN_PROPERTIES(Object)
    PROPERTY_STRING("classname", "Classname", "", "", PropertyInfo::ReadWrite | PropertyInfo::Hidden),
    PROPERTY_STRING("guid", "GUID", "GUID", Guid::zero, PropertyInfo::ReadWrite | PropertyInfo::Hidden),
END_PROPERTIES

BEGIN_EVENTS(Object)
    EVENT(EV_Destroy, Object::Event_Destroy),
    EVENT(EV_ImmediateDestroy, Object::Event_ImmediateDestroy),
END_EVENTS

bool                Object::initialized = false;
Array<MetaObject *> Object::types;  // alphabetical order

static HashTable<Guid, Object *> instanceHash;
static PlatformAtomic instanceCounter(0);

#ifdef NEW_PROPERTY_SYSTEM
void Object::RegisterProperties() {
    REGISTER_MIXED_ACCESSOR_PROPERTY("Classname", Str, ClassName, SetClassName, "Object", "", PropertyInfo::ReadWrite | PropertyInfo::Hidden),
    REGISTER_PROPERTY("GUID", Guid, guid, Guid::zero, "", PropertyInfo::ReadWrite | PropertyInfo::Hidden);
}
#endif

void Object::InitInstance(Guid guid) {
    if (guid.IsZero()) {
        Object *sameGuidObject;

        do {
            guid = Guid::CreateGuid();
        } while (instanceHash.Get(guid, &sameGuidObject));
    }

#if 1
    Object *sameGuidObject;
    if (instanceHash.Get(guid, &sameGuidObject)) {
        BE_WARNLOG(L"Conflicts GUID (%hs) for object type '%hs'\n", guid.ToString(), sameGuidObject->ClassName());
        assert(0);
    }
#endif

    this->guid = guid;
    this->instanceID = instanceCounter.GetValue();
    this->props = new Properties(this);

    instanceCounter++;

    Object *object = this;
    instanceHash.Set(guid, object);
}

Object::~Object() {
    SAFE_DELETE(props);
}

void Object::Init() {
    cmdSystem.AddCommand(L"listClasses", Object::ListClasses);

    BE_LOG(L"Initializing class hierarchy\n");

    if (initialized) {
        BE_LOG(L"...already initialized\n");
        return;
    }

    eventCallbackMemory = 0;

    int num = 0; 
    for (MetaObject *t = staticTypeList; t != nullptr; t = t->next, num++) {
        t->Init();
    }

    // number the types according to the class hierarchy so we can quickly determine if a class is a subclass of another
    num = 0;
    for (MetaObject *t = classHierarchy.GetNext(); t != nullptr; t = t->node.GetNext(), num++)	{
        t->hierarchyIndex = num;
        t->lastChildIndex += num;
    }
    
    types.SetGranularity(1);
    types.SetCount(num);

    num = 0;
    for (MetaObject *t = staticTypeList; t != nullptr; t = t->next, num++) {
        types[num] = t;
    }

    initialized = true;

    BE_LOG(L"...%i classes, %i bytes for event callbacks\n", types.Count(), eventCallbackMemory);
}

void Object::Shutdown() {
    cmdSystem.RemoveCommand(L"listClasses");
    
    for (int i = 0; i < types.Count(); i++) {
        types[i]->Shutdown();
    }

    types.Clear();

    instanceHash.Clear();

    initialized = false;
}

Str Object::ClassName() const {
    MetaObject *meta = GetMetaObject();
    return meta->classname;
}

void Object::SetClassName(const Str &classname) {
    BE_ERRLOG(L"not allowed to call SetClassName()");
}

Str Object::SuperClassName() const {
    MetaObject *meta = GetMetaObject();
    return meta->superclassname;
}

MetaObject *Object::FindMetaObject(const char *name) {
    if (!initialized) {
        // Object::Init hasn't been called yet, so do a slow lookup
        for (MetaObject *c = staticTypeList; c != nullptr; c = c->next) {
            if (!Str::Cmp(c->classname, name)) {
                return c;
            }
        }
    } else {
        // do a binary search through the list of types
        int min = 0;
        int max = types.Count() - 1;
        while (min <= max) {
            int mid = (min + max) / 2;
            MetaObject *c = types[mid];
            int order = Str::Cmp(c->classname, name);
            if (!order) {
                return c;
            } else if (order > 0) {
                max = mid - 1;
            } else {
                min = mid + 1;
            }
        }
    }

    return nullptr;
}

bool Object::IsRespondsTo(const EventDef &ev) const {
    assert(Event::initialized);
    const MetaObject *meta = GetMetaObject();
    return meta->IsRespondsTo(ev);
}

Object *Object::CreateInstance(const char *name, const Guid &guid) {
    const MetaObject *metaObject = Object::FindMetaObject(name);
    if (!metaObject) {
        return nullptr;
    }

    Object *instance = metaObject->CreateInstance(guid);
    return instance;
}

void Object::DestroyInstance(Object *instance, bool immediate) {
    if (immediate) {
        instance->Event_ImmediateDestroy();
    } else {
        instance->Event_Destroy();
    }
}

Object *Object::FindInstance(const Guid &guid) {
    Object *instance;
    if (!instanceHash.Get(guid, &instance)) {
        return nullptr;
    }

    return instance;
}

bool Object::GetPropertyInfo(const char *name, PropertyInfo &propertyInfo) const {
    Array<PropertyInfo> propertyInfos;
    GetPropertyInfoList(propertyInfos);

    for (int i = 0; i < propertyInfos.Count(); i++) {
        if (!Str::Cmp(propertyInfos[i].GetName(), name)) {
            propertyInfo = propertyInfos[i];
            return true;
        }
    }

    return false;
}

void Object::ListClasses(const CmdArgs &args) {
    BE_LOG(L"%-24hs %-24hs %-6hs %-6hs\n", "ClassName", "SuperClass", "Type", "SubClasses");
    BE_LOG(L"----------------------------------------------------------------------\n");

    for (int i = 0; i < types.Count(); i++) {
        MetaObject *type = types[i];
        BE_LOG(L"%-24hs %-24hs %-6d %-6d\n", type->classname, type->superclassname, type->hierarchyIndex, type->lastChildIndex - type->hierarchyIndex);
    }

    BE_LOG(L"...%d classes\n", types.Count());
}

void Object::CancelEvents(const EventDef *evdef) {
    Event::CancelEvents(this, evdef);
}

bool Object::PostEventArgs(const EventDef *evdef, int time, int numArgs, ...) {
    if (!Event::initialized) {
        return false;
    }

    assert(evdef);

    MetaObject *meta = GetMetaObject();
    if (!meta->eventCallbacks[evdef->GetEventNum()]) {
        // we don't respond to this event, so ignore it
        return false;
    }
    
    va_list args;
    va_start(args, numArgs);
    Event *event = Event::Alloc(evdef, numArgs, args);
    va_end(args);

    event->Schedule(this, time);

    return true;
}

bool Object::ProcessEventArgs(const EventDef *evdef, int numArgs, ...) {
    assert(evdef);
    assert(Event::initialized);

    MetaObject *meta = GetMetaObject();
    int num = evdef->GetEventNum();
    if (!meta->eventCallbacks[num]) {
        // we don't respond to this event, so ignore it
        return false;
    }

    va_list args;
    intptr_t argPtrs[EventArg::MaxArgs];
    
    // Copy EventArgs to array of intptr_t
    va_start(args, numArgs);
    Event::CopyArgPtrs(evdef, numArgs, args, argPtrs);
    va_end(args);

    ProcessEventArgPtr(evdef, argPtrs);

    return true;
}

bool Object::ProcessEventArgPtr(const EventDef *evdef, intptr_t *data) {
    assert(evdef);
    assert(Event::initialized);
    assert(EventArg::MaxArgs == 8);

    if (evdef == &EV_ImmediateDestroy) {
        Event::CancelEvents(this);
    }
    
    MetaObject *meta = GetMetaObject();
    int num = evdef->GetEventNum();
    if (!meta->eventCallbacks[num]) {
        // we don't respond to this event, so ignore it
        return false;
    }

    EventCallback callback = meta->eventCallbacks[num];
        
    switch (evdef->GetNumArgs()) {
    case 0:
        (this->*callback)();
        break;
    case 1:
        using eventCallback_1_t = void (Object::*)(const intptr_t);
        (this->*(eventCallback_1_t)callback)(data[0]);
        break;
    case 2:
        using eventCallback_2_t = void (Object::*)(const intptr_t, const intptr_t);
        (this->*(eventCallback_2_t)callback)(data[0], data[1]);
        break;
    case 3:
        using eventCallback_3_t = void (Object::*)(const intptr_t, const intptr_t, const intptr_t);
        (this->*(eventCallback_3_t)callback)(data[0], data[1], data[2]);
        break;
    case 4:
        using eventCallback_4_t = void (Object::*)(const intptr_t, const intptr_t, const intptr_t, const intptr_t);
        (this->*(eventCallback_4_t)callback)(data[0], data[1], data[2], data[3]);
        break;
    case 5:
        using eventCallback_5_t = void (Object::*)(const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t);
        (this->*(eventCallback_5_t)callback)(data[0], data[1], data[2], data[3], data[4]);
        break;
    case 6:
        using eventCallback_6_t = void (Object::*)(const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t);
        (this->*(eventCallback_6_t)callback)(data[0], data[1], data[2], data[3], data[4], data[5]);
        break;
    case 7:
        using eventCallback_7_t = void (Object::*)(const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t);
        (this->*(eventCallback_7_t)callback)(data[0], data[1], data[2], data[3], data[4], data[5], data[6]);
        break;
    case 8:
        using eventCallback_8_t = void (Object::*)(const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t);
        (this->*(eventCallback_8_t)callback)(data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
        break;
    default:
        BE_WARNLOG(L"Invalid formatSpec on event '%hs'\n", evdef->GetName());
        break;
    }

    return true;
}

void Object::Event_ImmediateDestroy() {
    instanceHash.Remove(guid);

    delete this;
}

void Object::Event_Destroy() {
    // Forces the remove to be done at a safe time
    PostEventMS(&EV_ImmediateDestroy, 0);
}

BE_NAMESPACE_END
