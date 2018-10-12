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

MetaObject::MetaObject(const char *visualname, const char *classname, const char *superclassname, Object *(*CreateInstance)(const Guid &guid), EventInfo<Object> *eventMap) {
    this->visualname            = visualname;
    this->classname             = classname;
    this->superclassname        = superclassname;
    this->super                 = Object::FindMetaObject(superclassname);
    this->hierarchyIndex        = 0;
    this->lastChildIndex        = 0;
    this->funcCreateInstance    = CreateInstance;
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
    assert(EventSystem::initialized);
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
            if (!Str::Icmp(t->propertyInfoList[i].GetName(), name)) {
                propertyInfo = t->propertyInfoList[i];
                return true;
            }
        }
    }

    return false;
}

void MetaObject::GetPropertyInfoList(Array<PropertyInfo> &propertyInfoList) const {
    Array<Str> names;

    for (const MetaObject *t = this; t != nullptr; t = t->super) {
        for (int index = 0; index < t->propertyInfoList.Count(); index++) {
            const PropertyInfo &propInfo = t->propertyInfoList[index];
            const char *propName = propInfo.GetName();

            if (!names.Find(propName)) {
                names.Append(propName);
                propertyInfoList.Append(propInfo);
            }
        }
    }
}

PropertyInfo &MetaObject::RegisterProperty(const PropertyInfo &propInfo) {
    int index = propertyInfoList.Append(propInfo);
    int hash = propertyInfoHash.GenerateHash(propInfo.GetName(), false);
    propertyInfoHash.Add(hash, index);
    return propertyInfoList[index];
}

//-----------------------------------------------------------------------------------------------

const EventDef EV_ImmediateDestroy("<immediatedestroy>");
const EventDef EV_Destroy("destroy");

ABSTRACT_DECLARATION("Object", Object, nullptr)
BEGIN_EVENTS(Object)
    EVENT(EV_Destroy, Object::Event_Destroy),
    EVENT(EV_ImmediateDestroy, Object::Event_ImmediateDestroy),
END_EVENTS

bool                Object::initialized = false;
Array<MetaObject *> Object::types;  // alphabetical order

static HashTable<Guid, Object *> instanceHash;
static PlatformAtomic instanceCounter(0);

void Object::RegisterProperties() {
    REGISTER_MIXED_ACCESSOR_PROPERTY("classname", "Classname", Str, ClassName, SetClassName, "", "", PropertyInfo::ReadOnlyFlag),
    REGISTER_PROPERTY("guid", "GUID", Guid, guid, Guid::zero, "", PropertyInfo::ReadOnlyFlag);
}

bool Object::InitInstance(Guid guid) {
    if (guid.IsZero()) {
        Object *sameGuidObject;

        do {
            guid = Guid::CreateGuid();
        } while (instanceHash.Get(guid, &sameGuidObject));
    }

#if 1
    Object *sameGuidObject;
    if (instanceHash.Get(guid, &sameGuidObject)) {
        BE_WARNLOG(L"Conflicts GUID (%hs) for object type '%hs'\n", guid.ToString(), sameGuidObject->ClassName().c_str());
        assert(0);
        return false;
    }
#endif

    this->guid = guid;
    this->instanceID = instanceCounter.GetValue();

    instanceCounter++;

    Object *object = this;
    instanceHash.Set(guid, object);

    return true;
}

Object::~Object() {
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
    for (MetaObject *t = classHierarchy.GetNext(); t != nullptr; t = t->node.GetNext(), num++) {
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
    const MetaObject *meta = GetMetaObject();
    return meta->classname;
}

void Object::SetClassName(const Str &classname) {
    BE_ERRLOG(L"not allowed to call SetClassName()");
}

Str Object::SuperClassName() const {
    const MetaObject *meta = GetMetaObject();
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
    assert(EventSystem::initialized);
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
    EventSystem::CancelEvents(this, evdef);
}

bool Object::PostEventArgs(const EventDef *evdef, int time, int numArgs, ...) {
    if (!EventSystem::initialized) {
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
    Event *event = EventSystem::AllocEvent(evdef, numArgs, args);
    va_end(args);

    EventSystem::ScheduleEvent(event, this, time);

    return true;
}

bool Object::ProcessEventArgs(const EventDef *evdef, int numArgs, ...) {
    assert(evdef);
    assert(EventSystem::initialized);

    MetaObject *meta = GetMetaObject();
    int num = evdef->GetEventNum();
    if (!meta->eventCallbacks[num]) {
        // we don't respond to this event, so ignore it
        return false;
    }

    intptr_t argPtrs[EventDef::MaxArgs];
    
    // Copy arguments to array of intptr_t
    va_list args;
    va_start(args, numArgs);
    EventSystem::CopyArgPtrs(evdef, numArgs, args, argPtrs);
    va_end(args);

    ProcessEventArgPtr(evdef, argPtrs);

    return true;
}

bool Object::ProcessEventArgPtr(const EventDef *evdef, intptr_t *data) {
    assert(evdef);
    assert(EventSystem::initialized);

    if (evdef == &EV_ImmediateDestroy) {
        EventSystem::CancelEvents(this);
    }
    
    const MetaObject *meta = GetMetaObject();
    int num = evdef->GetEventNum();
    if (!meta->eventCallbacks[num]) {
        // we don't respond to this event, so ignore it
        return false;
    }

    EventCallback callback = meta->eventCallbacks[num];

    // formatSpecBits = 0bNNNFFFFFF
    // N means number of arguments
    // F means float bit mask
    switch (evdef->GetFormatSpecBits()) {
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
        using EventCallback_i = void (Object::*)(const intptr_t);
        (this->*(EventCallback_i)callback)(data[0]);
        return true;
    case 0b001000001:
        using EventCallback_f = void (Object::*)(const float);
        (this->*(EventCallback_f)callback)(*(float *)&data[0]);
        return true;
        //----------------------------------------------------------------------------------------------
        // 2 args
        //----------------------------------------------------------------------------------------------
    case 0b010000000:
        using EventCallback_ii = void (Object::*)(const intptr_t, const intptr_t);
        (this->*(EventCallback_ii)callback)(data[0], data[1]);
        return true;
    case 0b010000001:
        using EventCallback_fi = void (Object::*)(const float, const intptr_t);
        (this->*(EventCallback_fi)callback)(*(float *)&data[0], data[1]);
        return true;
    case 0b010000010:
        using EventCallback_if = void (Object::*)(const intptr_t, const float);
        (this->*(EventCallback_if)callback)(data[0], *(float *)&data[1]);
        return true;
    case 0b010000011:
        using EventCallback_ff = void (Object::*)(const float, const float);
        (this->*(EventCallback_ff)callback)(*(float *)&data[0], *(float *)&data[1]);
        return true;
        // 3 args
    case 0b011000000:
        using EventCallback_iii = void (Object::*)(const intptr_t, const intptr_t, const intptr_t);
        (this->*(EventCallback_iii)callback)(data[0], data[1], data[2]);
        return true;
    case 0b011000001:
        using EventCallback_fii = void (Object::*)(const float, const intptr_t, const intptr_t);
        (this->*(EventCallback_fii)callback)(*(float *)&data[0], data[1], data[2]);
        return true;
    case 0b011000010:
        using EventCallback_ifi = void (Object::*)(const intptr_t, const float, const intptr_t);
        (this->*(EventCallback_ifi)callback)(data[0], *(float *)&data[1], data[2]);
        return true;
    case 0b011000011:
        using EventCallback_ffi = void (Object::*)(const float, const float, const intptr_t);
        (this->*(EventCallback_ffi)callback)(*(float *)&data[0], *(float *)&data[1], data[2]);
        return true;
    case 0b011000100:
        using EventCallback_iif = void (Object::*)(const intptr_t, const intptr_t, const float);
        (this->*(EventCallback_iif)callback)(data[0], data[1], *(float *)&data[2]);
        return true;
    case 0b011000101:
        using EventCallback_fif = void (Object::*)(const float, const intptr_t, const float);
        (this->*(EventCallback_fif)callback)(*(float *)&data[0], data[1], *(float *)&data[2]);
        return true;
    case 0b011000110:
        using EventCallback_iff = void (Object::*)(const intptr_t, const float, const float);
        (this->*(EventCallback_iff)callback)(data[0], *(float *)&data[1], *(float *)&data[2]);
        return true;
    case 0b011000111:
        using EventCallback_fff = void (Object::*)(const float, const float, const float);
        (this->*(EventCallback_fff)callback)(*(float *)&data[0], *(float *)&data[1], *(float *)&data[2]);
        return true;
        //----------------------------------------------------------------------------------------------
        // 4 args
        //----------------------------------------------------------------------------------------------
    case 0b100000000:
        using EventCallback_iiii = void (Object::*)(const intptr_t, const intptr_t, const intptr_t, const intptr_t);
        (this->*(EventCallback_iiii)callback)(data[0], data[1], data[2], data[3]);
        return true;
    case 0b100000001:
        using EventCallback_fiii = void (Object::*)(const float, const intptr_t, const intptr_t, const intptr_t);
        (this->*(EventCallback_fiii)callback)(*(float *)&data[0], data[1], data[2], data[3]);
        return true;
    case 0b100000010:
        using EventCallback_ifii = void (Object::*)(const intptr_t, const float, const intptr_t, const intptr_t);
        (this->*(EventCallback_ifii)callback)(data[0], *(float *)&data[1], data[2], data[3]);
        return true;
    case 0b100000011:
        using EventCallback_ffii = void (Object::*)(const float, const float, const intptr_t, const intptr_t);
        (this->*(EventCallback_ffii)callback)(*(float *)&data[0], *(float *)&data[1], data[2], data[3]);
        return true;
    case 0b100000100:
        using EventCallback_iifi = void (Object::*)(const intptr_t, const intptr_t, const float, const intptr_t);
        (this->*(EventCallback_iifi)callback)(data[0], data[1], *(float *)&data[2], data[3]);
        return true;
    case 0b100000101:
        using EventCallback_fifi = void (Object::*)(const float, const intptr_t, const float, const intptr_t);
        (this->*(EventCallback_fifi)callback)(*(float *)&data[0], data[1], *(float *)&data[2], data[3]);
        return true;
    case 0b100000110:
        using EventCallback_iffi = void (Object::*)(const intptr_t, const float, const float, const intptr_t);
        (this->*(EventCallback_iffi)callback)(data[0], *(float *)&data[1], *(float *)&data[2], data[3]);
        return true;
    case 0b100000111:
        using EventCallback_fffi = void (Object::*)(const float, const float, const float, const intptr_t);
        (this->*(EventCallback_fffi)callback)(*(float *)&data[0], *(float *)&data[1], *(float *)&data[2], data[3]);
        return true;
    case 0b100001000:
        using EventCallback_iiif = void (Object::*)(const intptr_t, const intptr_t, const intptr_t, const float);
        (this->*(EventCallback_iiif)callback)(data[0], data[1], data[2], *(float *)&data[3]);
        return true;
    case 0b100001001:
        using EventCallback_fiif = void (Object::*)(const float, const intptr_t, const intptr_t, const float);
        (this->*(EventCallback_fiif)callback)(*(float *)&data[0], data[1], data[2], *(float *)&data[3]);
        return true;
    case 0b100001010:
        using EventCallback_ifif = void (Object::*)(const intptr_t, const float, const intptr_t, const float);
        (this->*(EventCallback_ifif)callback)(data[0], *(float *)&data[1], data[2], *(float *)&data[3]);
        return true;
    case 0b100001011:
        using EventCallback_ffif = void (Object::*)(const float, const float, const intptr_t, const float);
        (this->*(EventCallback_ffif)callback)(*(float *)&data[0], *(float *)&data[1], data[2], *(float *)&data[3]);
        return true;
    case 0b100001100:
        using EventCallback_iiff = void (Object::*)(const intptr_t, const intptr_t, const float, const float);
        (this->*(EventCallback_iiff)callback)(data[0], data[1], *(float *)&data[2], *(float *)&data[3]);
        return true;
    case 0b100001101:
        using EventCallback_fiff = void (Object::*)(const float, const intptr_t, const float, const float);
        (this->*(EventCallback_fiff)callback)(*(float *)&data[0], data[1], *(float *)&data[2], *(float *)&data[3]);
        return true;
    case 0b100001110:
        using EventCallback_ifff = void (Object::*)(const intptr_t, const float, const float, const float);
        (this->*(EventCallback_ifff)callback)(data[0], *(float *)&data[1], *(float *)&data[2], *(float *)&data[3]);
        return true;
    case 0b100001111:
        using EventCallback_ffff = void (Object::*)(const float, const float, const float, const float);
        (this->*(EventCallback_ffff)callback)(*(float *)&data[0], *(float *)&data[1], *(float *)&data[2], *(float *)&data[3]);
        return true;
        //----------------------------------------------------------------------------------------------
        // 5 args
        //----------------------------------------------------------------------------------------------
    case 0b101000000:
        using EventCallback_iiiii = void (Object::*)(const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t);
        (this->*(EventCallback_iiiii)callback)(data[0], data[1], data[2], data[3], data[4]);
        return true;
    case 0b101000001:
        using EventCallback_fiiii = void (Object::*)(const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t);
        (this->*(EventCallback_fiiii)callback)(*(float *)&data[0], data[1], data[2], data[3], data[4]);
        return true;
    case 0b101000010:
        using EventCallback_ifiii = void (Object::*)(const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t);
        (this->*(EventCallback_ifiii)callback)(data[0], *(float *)&data[1], data[2], data[3], data[4]);
        return true;
    case 0b101000011:
        using EventCallback_ffiii = void (Object::*)(const float, const float, const intptr_t, const intptr_t, const intptr_t);
        (this->*(EventCallback_ffiii)callback)(*(float *)&data[0], *(float *)&data[1], data[2], data[3], data[4]);
        return true;
    case 0b101000100:
        using EventCallback_iifii = void (Object::*)(const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t);
        (this->*(EventCallback_iifii)callback)(data[0], data[1], *(float *)&data[2], data[3], data[4]);
        return true;
    case 0b101000101:
        using EventCallback_fifii = void (Object::*)(const float, const intptr_t, const float, const intptr_t, const intptr_t);
        (this->*(EventCallback_fifii)callback)(*(float *)&data[0], data[1], *(float *)&data[2], data[3], data[4]);
        return true;
    case 0b101000110:
        using EventCallback_iffii = void (Object::*)(const intptr_t, const float, const float, const intptr_t, const intptr_t);
        (this->*(EventCallback_iffii)callback)(data[0], *(float *)&data[1], *(float *)&data[2], data[3], data[4]);
        return true;
    case 0b101000111:
        using EventCallback_fffii = void (Object::*)(const float, const float, const float, const intptr_t, const intptr_t);
        (this->*(EventCallback_fffii)callback)(*(float *)&data[0], *(float *)&data[1], *(float *)&data[2], data[3], data[4]);
        return true;
    case 0b101001000:
        using EventCallback_iiifi = void (Object::*)(const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t);
        (this->*(EventCallback_iiifi)callback)(data[0], data[1], data[2], *(float *)&data[3], data[4]);
        return true;
    case 0b101001001:
        using EventCallback_fiifi = void (Object::*)(const float, const intptr_t, const intptr_t, const float, const intptr_t);
        (this->*(EventCallback_fiifi)callback)(*(float *)&data[0], data[1], data[2], *(float *)&data[3], data[4]);
        return true;
    case 0b101001010:
        using EventCallback_ififi = void (Object::*)(const intptr_t, const float, const intptr_t, const float, const intptr_t);
        (this->*(EventCallback_ififi)callback)(data[0], *(float *)&data[1], data[2], *(float *)&data[3], data[4]);
        return true;
    case 0b101001011:
        using EventCallback_ffifi = void (Object::*)(const float, const float, const intptr_t, const float, const intptr_t);
        (this->*(EventCallback_ffifi)callback)(*(float *)&data[0], *(float *)&data[1], data[2], *(float *)&data[3], data[4]);
        return true;
    case 0b101001100:
        using EventCallback_iiffi = void (Object::*)(const intptr_t, const intptr_t, const float, const float, const intptr_t);
        (this->*(EventCallback_iiffi)callback)(data[0], data[1], *(float *)&data[2], *(float *)&data[3], data[4]);
        return true;
    case 0b101001101:
        using EventCallback_fiffi = void (Object::*)(const float, const intptr_t, const float, const float, const intptr_t);
        (this->*(EventCallback_fiffi)callback)(*(float *)&data[0], data[1], *(float *)&data[2], *(float *)&data[3], data[4]);
        return true;
    case 0b101001110:
        using EventCallback_ifffi = void (Object::*)(const intptr_t, const float, const float, const float, const intptr_t);
        (this->*(EventCallback_ifffi)callback)(data[0], *(float *)&data[1], *(float *)&data[2], *(float *)&data[3], data[4]);
        return true;
    case 0b101001111:
        using EventCallback_ffffi = void (Object::*)(const float, const float, const float, const float, const intptr_t);
        (this->*(EventCallback_ffffi)callback)(*(float *)&data[0], *(float *)&data[1], *(float *)&data[2], *(float *)&data[3], data[4]);
        return true;
    case 0b101010000:
        using EventCallback_iiiif = void (Object::*)(const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float);
        (this->*(EventCallback_iiiif)callback)(data[0], data[1], data[2], data[3], *(float *)&data[4]);
        return true;
    case 0b101010001:
        using EventCallback_fiiif = void (Object::*)(const float, const intptr_t, const intptr_t, const intptr_t, const float);
        (this->*(EventCallback_fiiif)callback)(*(float *)&data[0], data[1], data[2], data[3], *(float *)&data[4]);
        return true;
    case 0b101010010:
        using EventCallback_ifiif = void (Object::*)(const intptr_t, const float, const intptr_t, const intptr_t, const float);
        (this->*(EventCallback_ifiif)callback)(data[0], *(float *)&data[1], data[2], data[3], *(float *)&data[4]);
        return true;
    case 0b101010011:
        using EventCallback_ffiif = void (Object::*)(const float, const float, const intptr_t, const intptr_t, const float);
        (this->*(EventCallback_ffiif)callback)(*(float *)&data[0], *(float *)&data[1], data[2], data[3], *(float *)&data[4]);
        return true;
    case 0b101010100:
        using EventCallback_iifif = void (Object::*)(const intptr_t, const intptr_t, const float, const intptr_t, const float);
        (this->*(EventCallback_iifif)callback)(data[0], data[1], *(float *)&data[2], data[3], *(float *)&data[4]);
        return true;
    case 0b101010101:
        using EventCallback_fifif = void (Object::*)(const float, const intptr_t, const float, const intptr_t, const float);
        (this->*(EventCallback_fifif)callback)(*(float *)&data[0], data[1], *(float *)&data[2], data[3], *(float *)&data[4]);
        return true;
    case 0b101010110:
        using EventCallback_iffif = void (Object::*)(const intptr_t, const float, const float, const intptr_t, const float);
        (this->*(EventCallback_iffif)callback)(data[0], *(float *)&data[1], *(float *)&data[2], data[3], *(float *)&data[4]);
        return true;
    case 0b101010111:
        using EventCallback_fffif = void (Object::*)(const float, const float, const float, const intptr_t, const float);
        (this->*(EventCallback_fffif)callback)(*(float *)&data[0], *(float *)&data[1], *(float *)&data[2], data[3], *(float *)&data[4]);
        return true;
    case 0b101011000:
        using EventCallback_iiiff = void (Object::*)(const intptr_t, const intptr_t, const intptr_t, const float, const float);
        (this->*(EventCallback_iiiff)callback)(data[0], data[1], data[2], *(float *)&data[3], *(float *)&data[4]);
        return true;
    case 0b101011001:
        using EventCallback_fiiff = void (Object::*)(const float, const intptr_t, const intptr_t, const float, const float);
        (this->*(EventCallback_fiiff)callback)(*(float *)&data[0], data[1], data[2], *(float *)&data[3], *(float *)&data[4]);
        return true;
    case 0b101011010:
        using EventCallback_ififf = void (Object::*)(const intptr_t, const float, const intptr_t, const float, const float);
        (this->*(EventCallback_ififf)callback)(data[0], *(float *)&data[1], data[2], *(float *)&data[3], *(float *)&data[4]);
        return true;
    case 0b101011011:
        using EventCallback_ffiff = void (Object::*)(const float, const float, const intptr_t, const float, const float);
        (this->*(EventCallback_ffiff)callback)(*(float *)&data[0], *(float *)&data[1], data[2], *(float *)&data[3], *(float *)&data[4]);
        return true;
    case 0b101011100:
        using EventCallback_iifff = void (Object::*)(const intptr_t, const intptr_t, const float, const float, const float);
        (this->*(EventCallback_iifff)callback)(data[0], data[1], *(float *)&data[2], *(float *)&data[3], *(float *)&data[4]);
        return true;
    case 0b101011101:
        using EventCallback_fifff = void (Object::*)(const float, const intptr_t, const float, const float, const float);
        (this->*(EventCallback_fifff)callback)(*(float *)&data[0], data[1], *(float *)&data[2], *(float *)&data[3], *(float *)&data[4]);
        return true;
    case 0b101011110:
        using EventCallback_iffff = void (Object::*)(const intptr_t, const float, const float, const float, const float);
        (this->*(EventCallback_iffff)callback)(data[0], *(float *)&data[1], *(float *)&data[2], *(float *)&data[3], *(float *)&data[4]);
        return true;
    case 0b101011111:
        using EventCallback_fffff = void (Object::*)(const float, const float, const float, const float, const float);
        (this->*(EventCallback_fffff)callback)(*(float *)&data[0], *(float *)&data[1], *(float *)&data[2], *(float *)&data[3], *(float *)&data[4]);
        return true;
        //----------------------------------------------------------------------------------------------
        // 6 args
        //----------------------------------------------------------------------------------------------
    case 0b110000000:
        using EventCallback_iiiiii = void (Object::*)(const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t);
        (this->*(EventCallback_iiiiii)callback)(data[0], data[1], data[2], data[3], data[4], data[5]);
        return true;
    case 0b110000001:
        using EventCallback_fiiiii = void (Object::*)(const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t);
        (this->*(EventCallback_fiiiii)callback)(*(float *)&data[0], data[1], data[2], data[3], data[4], data[5]);
        return true;
    case 0b110000010:
        using EventCallback_ifiiii = void (Object::*)(const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t);
        (this->*(EventCallback_ifiiii)callback)(data[0], *(float *)&data[1], data[2], data[3], data[4], data[5]);
        return true;
    case 0b110000011:
        using EventCallback_ffiiii = void (Object::*)(const float, const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t);
        (this->*(EventCallback_ffiiii)callback)(*(float *)&data[0], *(float *)&data[1], data[2], data[3], data[4], data[5]);
        return true;
    case 0b110000100:
        using EventCallback_iifiii = void (Object::*)(const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t);
        (this->*(EventCallback_iifiii)callback)(data[0], data[1], *(float *)&data[2], data[3], data[4], data[5]);
        return true;
    case 0b110000101:
        using EventCallback_fifiii = void (Object::*)(const float, const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t);
        (this->*(EventCallback_fifiii)callback)(*(float *)&data[0], data[1], *(float *)&data[2], data[3], data[4], data[5]);
        return true;
    case 0b110000110:
        using EventCallback_iffiii = void (Object::*)(const intptr_t, const float, const float, const intptr_t, const intptr_t, const intptr_t);
        (this->*(EventCallback_iffiii)callback)(data[0], *(float *)&data[1], *(float *)&data[2], data[3], data[4], data[5]);
        return true;
    case 0b110000111:
        using EventCallback_fffiii = void (Object::*)(const float, const float, const float, const intptr_t, const intptr_t, const intptr_t);
        (this->*(EventCallback_fffiii)callback)(*(float *)&data[0], *(float *)&data[1], *(float *)&data[2], data[3], data[4], data[5]);
        return true;
    case 0b110001000:
        using EventCallback_iiifii = void (Object::*)(const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t);
        (this->*(EventCallback_iiifii)callback)(data[0], data[1], data[2], *(float *)&data[3], data[4], data[5]);
        return true;
    case 0b110001001:
        using EventCallback_fiifii = void (Object::*)(const float, const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t);
        (this->*(EventCallback_fiifii)callback)(*(float *)&data[0], data[1], data[2], *(float *)&data[3], data[4], data[5]);
        return true;
    case 0b110001010:
        using EventCallback_ififii = void (Object::*)(const intptr_t, const float, const intptr_t, const float, const intptr_t, const intptr_t);
        (this->*(EventCallback_ififii)callback)(data[0], *(float *)&data[1], data[2], *(float *)&data[3], data[4], data[5]);
        return true;
    case 0b110001011:
        using EventCallback_ffifii = void (Object::*)(const float, const float, const intptr_t, const float, const intptr_t, const intptr_t);
        (this->*(EventCallback_ffifii)callback)(*(float *)&data[0], *(float *)&data[1], data[2], *(float *)&data[3], data[4], data[5]);
        return true;
    case 0b110001100:
        using EventCallback_iiffii = void (Object::*)(const intptr_t, const intptr_t, const float, const float, const intptr_t, const intptr_t);
        (this->*(EventCallback_iiffii)callback)(data[0], data[1], *(float *)&data[2], *(float *)&data[3], data[4], data[5]);
        return true;
    case 0b110001101:
        using EventCallback_fiffii = void (Object::*)(const float, const intptr_t, const float, const float, const intptr_t, const intptr_t);
        (this->*(EventCallback_fiffii)callback)(*(float *)&data[0], data[1], *(float *)&data[2], *(float *)&data[3], data[4], data[5]);
        return true;
    case 0b110001110:
        using EventCallback_ifffii = void (Object::*)(const intptr_t, const float, const float, const float, const intptr_t, const intptr_t);
        (this->*(EventCallback_ifffii)callback)(data[0], *(float *)&data[1], *(float *)&data[2], *(float *)&data[3], data[4], data[5]);
        return true;
    case 0b110001111:
        using EventCallback_ffffii = void (Object::*)(const float, const float, const float, const float, const intptr_t, const intptr_t);
        (this->*(EventCallback_ffffii)callback)(*(float *)&data[0], *(float *)&data[1], *(float *)&data[2], *(float *)&data[3], data[4], data[5]);
        return true;
    case 0b110010000:
        using EventCallback_iiiifi = void (Object::*)(const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t);
        (this->*(EventCallback_iiiifi)callback)(data[0], data[1], data[2], data[3], *(float *)&data[4], data[5]);
        return true;
    case 0b110010001:
        using EventCallback_fiiifi = void (Object::*)(const float, const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t);
        (this->*(EventCallback_fiiifi)callback)(*(float *)&data[0], data[1], data[2], data[3], *(float *)&data[4], data[5]);
        return true;
    case 0b110010010:
        using EventCallback_ifiifi = void (Object::*)(const intptr_t, const float, const intptr_t, const intptr_t, const float, const intptr_t);
        (this->*(EventCallback_ifiifi)callback)(data[0], *(float *)&data[1], data[2], data[3], *(float *)&data[4], data[5]);
        return true;
    case 0b110010011:
        using EventCallback_ffiifi = void (Object::*)(const float, const float, const intptr_t, const intptr_t, const float, const intptr_t);
        (this->*(EventCallback_ffiifi)callback)(*(float *)&data[0], *(float *)&data[1], data[2], data[3], *(float *)&data[4], data[5]);
        return true;
    case 0b110010100:
        using EventCallback_iififi = void (Object::*)(const intptr_t, const intptr_t, const float, const intptr_t, const float, const intptr_t);
        (this->*(EventCallback_iififi)callback)(data[0], data[1], *(float *)&data[2], data[3], *(float *)&data[4], data[5]);
        return true;
    case 0b110010101:
        using EventCallback_fififi = void (Object::*)(const float, const intptr_t, const float, const intptr_t, const float, const intptr_t);
        (this->*(EventCallback_fifiii)callback)(*(float *)&data[0], data[1], *(float *)&data[2], data[3], *(float *)&data[4], data[5]);
        return true;
    case 0b110010110:
        using EventCallback_iffifi = void (Object::*)(const intptr_t, const float, const float, const intptr_t, const float, const intptr_t);
        (this->*(EventCallback_iffifi)callback)(data[0], *(float *)&data[1], *(float *)&data[2], data[3], *(float *)&data[4], data[5]);
        return true;
    case 0b110010111:
        using EventCallback_fffifi = void (Object::*)(const float, const float, const float, const intptr_t, const float, const intptr_t);
        (this->*(EventCallback_fffifi)callback)(*(float *)&data[0], *(float *)&data[1], *(float *)&data[2], data[3], *(float *)&data[4], data[5]);
        return true;
    case 0b110011000:
        using EventCallback_iiiffi = void (Object::*)(const intptr_t, const intptr_t, const intptr_t, const float, const float, const intptr_t);
        (this->*(EventCallback_iiiffi)callback)(data[0], data[1], data[2], *(float *)&data[3], *(float *)&data[4], data[5]);
        return true;
    case 0b110011001:
        using EventCallback_fiiffi = void (Object::*)(const float, const intptr_t, const intptr_t, const float, const float, const intptr_t);
        (this->*(EventCallback_fiiffi)callback)(*(float *)&data[0], data[1], data[2], *(float *)&data[3], *(float *)&data[4], data[5]);
        return true;
    case 0b110011010:
        using EventCallback_ififfi = void (Object::*)(const intptr_t, const float, const intptr_t, const float, const float, const intptr_t);
        (this->*(EventCallback_ififfi)callback)(data[0], *(float *)&data[1], data[2], *(float *)&data[3], *(float *)&data[4], data[5]);
        return true;
    case 0b110011011:
        using EventCallback_ffiffi = void (Object::*)(const float, const float, const intptr_t, const float, const float, const intptr_t);
        (this->*(EventCallback_ffiffi)callback)(*(float *)&data[0], *(float *)&data[1], data[2], *(float *)&data[3], *(float *)&data[4], data[5]);
        return true;
    case 0b110011100:
        using EventCallback_iifffi = void (Object::*)(const intptr_t, const intptr_t, const float, const float, const float, const intptr_t);
        (this->*(EventCallback_iifffi)callback)(data[0], data[1], *(float *)&data[2], *(float *)&data[3], *(float *)&data[4], data[5]);
        return true;
    case 0b110011101:
        using EventCallback_fifffi = void (Object::*)(const float, const intptr_t, const float, const float, const float, const intptr_t);
        (this->*(EventCallback_fifffi)callback)(*(float *)&data[0], data[1], *(float *)&data[2], *(float *)&data[3], *(float *)&data[4], data[5]);
        return true;
    case 0b110011110:
        using EventCallback_iffffi = void (Object::*)(const intptr_t, const float, const float, const float, const float, const intptr_t);
        (this->*(EventCallback_iffffi)callback)(data[0], *(float *)&data[1], *(float *)&data[2], *(float *)&data[3], *(float *)&data[4], data[5]);
        return true;
    case 0b110011111:
        using EventCallback_fffffi = void (Object::*)(const float, const float, const float, const float, const float, const intptr_t);
        (this->*(EventCallback_fffffi)callback)(*(float *)&data[0], *(float *)&data[1], *(float *)&data[2], *(float *)&data[3], *(float *)&data[4], data[5]);
        return true;
    case 0b110100000:
        using EventCallback_iiiiif = void (Object::*)(const intptr_t, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float);
        (this->*(EventCallback_iiiiif)callback)(data[0], data[1], data[2], data[3], data[4], *(float *)&data[5]);
        return true;
    case 0b110100001:
        using EventCallback_fiiiif = void (Object::*)(const float, const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float);
        (this->*(EventCallback_fiiiif)callback)(*(float *)&data[0], data[1], data[2], data[3], data[4], *(float *)&data[5]);
        return true;
    case 0b110100010:
        using EventCallback_ifiiif = void (Object::*)(const intptr_t, const float, const intptr_t, const intptr_t, const intptr_t, const float);
        (this->*(EventCallback_ifiiif)callback)(data[0], *(float *)&data[1], data[2], data[3], data[4], *(float *)&data[5]);
        return true;
    case 0b110100011:
        using EventCallback_ffiiif = void (Object::*)(const float, const float, const intptr_t, const intptr_t, const intptr_t, const float);
        (this->*(EventCallback_ffiiif)callback)(*(float *)&data[0], *(float *)&data[1], data[2], data[3], data[4], *(float *)&data[5]);
        return true;
    case 0b110100100:
        using EventCallback_iifiif = void (Object::*)(const intptr_t, const intptr_t, const float, const intptr_t, const intptr_t, const float);
        (this->*(EventCallback_iifiif)callback)(data[0], data[1], *(float *)&data[2], data[3], data[4], *(float *)&data[5]);
        return true;
    case 0b110100101:
        using EventCallback_fifiif = void (Object::*)(const float, const intptr_t, const float, const intptr_t, const intptr_t, const float);
        (this->*(EventCallback_fifiif)callback)(*(float *)&data[0], data[1], *(float *)&data[2], data[3], data[4], *(float *)&data[5]);
        return true;
    case 0b110100110:
        using EventCallback_iffiif = void (Object::*)(const intptr_t, const float, const float, const intptr_t, const intptr_t, const float);
        (this->*(EventCallback_iffiif)callback)(data[0], *(float *)&data[1], *(float *)&data[2], data[3], data[4], *(float *)&data[5]);
        return true;
    case 0b110100111:
        using EventCallback_fffiif = void (Object::*)(const float, const float, const float, const intptr_t, const intptr_t, const float);
        (this->*(EventCallback_fffiif)callback)(*(float *)&data[0], *(float *)&data[1], *(float *)&data[2], data[3], data[4], *(float *)&data[5]);
        return true;
    case 0b110101000:
        using EventCallback_iiifif = void (Object::*)(const intptr_t, const intptr_t, const intptr_t, const float, const intptr_t, const float);
        (this->*(EventCallback_iiifif)callback)(data[0], data[1], data[2], *(float *)&data[3], data[4], *(float *)&data[5]);
        return true;
    case 0b110101001:
        using EventCallback_fiifif = void (Object::*)(const float, const intptr_t, const intptr_t, const float, const intptr_t, const float);
        (this->*(EventCallback_fiifif)callback)(*(float *)&data[0], data[1], data[2], *(float *)&data[3], data[4], *(float *)&data[5]);
        return true;
    case 0b110101010:
        using EventCallback_ififif = void (Object::*)(const intptr_t, const float, const intptr_t, const float, const intptr_t, const float);
        (this->*(EventCallback_ififif)callback)(data[0], *(float *)&data[1], data[2], *(float *)&data[3], data[4], *(float *)&data[5]);
        return true;
    case 0b110101011:
        using EventCallback_ffifif = void (Object::*)(const float, const float, const intptr_t, const float, const intptr_t, const float);
        (this->*(EventCallback_ffifif)callback)(*(float *)&data[0], *(float *)&data[1], data[2], *(float *)&data[3], data[4], *(float *)&data[5]);
        return true;
    case 0b110101100:
        using EventCallback_iiffif = void (Object::*)(const intptr_t, const intptr_t, const float, const float, const intptr_t, const float);
        (this->*(EventCallback_iiffif)callback)(data[0], data[1], *(float *)&data[2], *(float *)&data[3], data[4], *(float *)&data[5]);
        return true;
    case 0b110101101:
        using EventCallback_fiffif = void (Object::*)(const float, const intptr_t, const float, const float, const intptr_t, const float);
        (this->*(EventCallback_fiffif)callback)(*(float *)&data[0], data[1], *(float *)&data[2], *(float *)&data[3], data[4], *(float *)&data[5]);
        return true;
    case 0b110101110:
        using EventCallback_ifffif = void (Object::*)(const intptr_t, const float, const float, const float, const intptr_t, const float);
        (this->*(EventCallback_ifffif)callback)(data[0], *(float *)&data[1], *(float *)&data[2], *(float *)&data[3], data[4], *(float *)&data[5]);
        return true;
    case 0b110101111:
        using EventCallback_ffffif = void (Object::*)(const float, const float, const float, const float, const intptr_t, const float);
        (this->*(EventCallback_ffffif)callback)(*(float *)&data[0], *(float *)&data[1], *(float *)&data[2], *(float *)&data[3], data[4], *(float *)&data[5]);
        return true;
    case 0b110110000:
        using EventCallback_iiiiff = void (Object::*)(const intptr_t, const intptr_t, const intptr_t, const intptr_t, const float, const float);
        (this->*(EventCallback_iiiiff)callback)(data[0], data[1], data[2], data[3], *(float *)&data[4], *(float *)&data[5]);
        return true;
    case 0b110110001:
        using EventCallback_fiiiff = void (Object::*)(const float, const intptr_t, const intptr_t, const intptr_t, const float, const float);
        (this->*(EventCallback_fiiiff)callback)(*(float *)&data[0], data[1], data[2], data[3], *(float *)&data[4], *(float *)&data[5]);
        return true;
    case 0b110110010:
        using EventCallback_ifiiff = void (Object::*)(const intptr_t, const float, const intptr_t, const intptr_t, const float, const float);
        (this->*(EventCallback_ifiiff)callback)(data[0], *(float *)&data[1], data[2], data[3], *(float *)&data[4], *(float *)&data[5]);
        return true;
    case 0b110110011:
        using EventCallback_ffiiff = void (Object::*)(const float, const float, const intptr_t, const intptr_t, const float, const float);
        (this->*(EventCallback_ffiiff)callback)(*(float *)&data[0], *(float *)&data[1], data[2], data[3], *(float *)&data[4], *(float *)&data[5]);
        return true;
    case 0b110110100:
        using EventCallback_iififf = void (Object::*)(const intptr_t, const intptr_t, const float, const intptr_t, const float, const float);
        (this->*(EventCallback_iififf)callback)(data[0], data[1], *(float *)&data[2], data[3], *(float *)&data[4], *(float *)&data[5]);
        return true;
    case 0b110110101:
        using EventCallback_fififf = void (Object::*)(const float, const intptr_t, const float, const intptr_t, const float, const float);
        (this->*(EventCallback_fifiif)callback)(*(float *)&data[0], data[1], *(float *)&data[2], data[3], *(float *)&data[4], *(float *)&data[5]);
        return true;
    case 0b110110110:
        using EventCallback_iffiff = void (Object::*)(const intptr_t, const float, const float, const intptr_t, const float, const float);
        (this->*(EventCallback_iffiff)callback)(data[0], *(float *)&data[1], *(float *)&data[2], data[3], *(float *)&data[4], *(float *)&data[5]);
        return true;
    case 0b110110111:
        using EventCallback_fffiff = void (Object::*)(const float, const float, const float, const intptr_t, const float, const float);
        (this->*(EventCallback_fffiff)callback)(*(float *)&data[0], *(float *)&data[1], *(float *)&data[2], data[3], *(float *)&data[4], *(float *)&data[5]);
        return true;
    case 0b110111000:
        using EventCallback_iiifff = void (Object::*)(const intptr_t, const intptr_t, const intptr_t, const float, const float, const float);
        (this->*(EventCallback_iiifff)callback)(data[0], data[1], data[2], *(float *)&data[3], *(float *)&data[4], *(float *)&data[5]);
        return true;
    case 0b110111001:
        using EventCallback_fiifff = void (Object::*)(const float, const intptr_t, const intptr_t, const float, const float, const float);
        (this->*(EventCallback_fiifff)callback)(*(float *)&data[0], data[1], data[2], *(float *)&data[3], *(float *)&data[4], *(float *)&data[5]);
        return true;
    case 0b110111010:
        using EventCallback_ififff = void (Object::*)(const intptr_t, const float, const intptr_t, const float, const float, const float);
        (this->*(EventCallback_ififff)callback)(data[0], *(float *)&data[1], data[2], *(float *)&data[3], *(float *)&data[4], *(float *)&data[5]);
        return true;
    case 0b110111011:
        using EventCallback_ffifff = void (Object::*)(const float, const float, const intptr_t, const float, const float, const float);
        (this->*(EventCallback_ffifff)callback)(*(float *)&data[0], *(float *)&data[1], data[2], *(float *)&data[3], *(float *)&data[4], *(float *)&data[5]);
        return true;
    case 0b110111100:
        using EventCallback_iiffff = void (Object::*)(const intptr_t, const intptr_t, const float, const float, const float, const float);
        (this->*(EventCallback_iiffff)callback)(data[0], data[1], *(float *)&data[2], *(float *)&data[3], *(float *)&data[4], *(float *)&data[5]);
        return true;
    case 0b110111101:
        using EventCallback_fiffff = void (Object::*)(const float, const intptr_t, const float, const float, const float, const float);
        (this->*(EventCallback_fiffff)callback)(*(float *)&data[0], data[1], *(float *)&data[2], *(float *)&data[3], *(float *)&data[4], *(float *)&data[5]);
        return true;
    case 0b110111110:
        using EventCallback_ifffff = void (Object::*)(const intptr_t, const float, const float, const float, const float, const float);
        (this->*(EventCallback_ifffff)callback)(data[0], *(float *)&data[1], *(float *)&data[2], *(float *)&data[3], *(float *)&data[4], *(float *)&data[5]);
        return true;
    case 0b110111111:
        using EventCallback_ffffff = void (Object::*)(const float, const float, const float, const float, const float, const float);
        (this->*(EventCallback_ffffff)callback)(*(float *)&data[0], *(float *)&data[1], *(float *)&data[2], *(float *)&data[3], *(float *)&data[4], *(float *)&data[5]);
        return true;
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
