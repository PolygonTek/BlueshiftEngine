// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

//
// The Impossibly Fast C++ Delegates, Fixed
// by Sergey Alexandrovich Kryukov
// https://www.codeproject.com/script/Articles/ViewDownloads.aspx?aid=1170503
//

#include <cstdint>
#include <list>

BE_NAMESPACE_BEGIN

struct DelegateHandle {
    DelegateHandle() = default;
    explicit DelegateHandle(uint64_t id) : id(id) {}

    bool IsValid() const { return id != 0; }
    void Clear() { id = 0; }

    bool operator==(const DelegateHandle &rhs) const { return id == rhs.id; }
    bool operator!=(const DelegateHandle &rhs) const { return id != rhs.id; }

    uint64_t                id = 0;
};

template <typename T>
class DelegateBase;

template <typename Ret, typename ...Parms>
class DelegateBase<Ret(Parms...)> {
protected:
    using Stub = Ret(*)(void *thisPtr, Parms...);

    struct Invocation {
        Invocation() = default;
        Invocation(void *object, Stub stub, DelegateHandle handle = DelegateHandle()) : object(object), stub(stub), handle(handle) {}

        bool IsNull() const { return stub == nullptr; }
        bool Matches(const Invocation &rhs) const { return object == rhs.object && stub == rhs.stub; }

        void *              object = nullptr;
        Stub                stub = nullptr;
        DelegateHandle      handle;
    };

    template <Ret(*Func)(Parms...)>
    static Ret FunctionStub(void *thisPtr, Parms... params) {
        return Func(params...);
    }

    template <typename T, Ret(T::*MemberFunc)(Parms...)>
    static Ret MethodStub(void *thisPtr, Parms... params) {
        T *object = static_cast<T *>(thisPtr);
        return (object->*MemberFunc)(params...);
    }

    template <typename T, Ret(T::*ConstMemberFunc)(Parms...) const>
    static Ret ConstMethodStub(void *thisPtr, Parms... params) {
        const T *object = static_cast<const T *>(thisPtr);
        return (object->*ConstMemberFunc)(params...);
    }
};

template <typename T>
class Delegate;

template <typename T>
class MulticastDelegate;

template <typename Ret, typename ...Parms>
class Delegate<Ret(Parms...)> final : private DelegateBase<Ret(Parms...)> {
private:
    using Base = DelegateBase<Ret(Parms...)>;

public:
    Delegate() = default;

    Delegate(const Delegate &another) {
        invocation = another.invocation;
    }

    Delegate &operator=(const Delegate &rhs) {
        invocation = rhs.invocation;
        return *this;
    }

    bool IsNull() const {
        return invocation.IsNull();
    }

    bool operator==(const Delegate &rhs) const {
        return invocation.Matches(rhs.invocation);
    }

    bool operator!=(const Delegate &rhs) const {
        return !(*this == rhs);
    }

    Ret operator()(Parms... args) const {
        return invocation.stub(invocation.object, args...);
    }

    template <Ret(*Func)(Parms...)>
    static Delegate FromFunction() {
        return Delegate(nullptr, &Base::template FunctionStub<Func>);
    }

    template <typename T, Ret(T::*MemberFunc)(Parms...)>
    static Delegate FromMemberFunc(T *instance) {
        return Delegate(instance, &Base::template MethodStub<T, MemberFunc>);
    }

    template <typename T, Ret(T::*ConstMemberFunc)(Parms...) const>
    static Delegate FromMemberFunc(const T *instance) {
        return Delegate(const_cast<T *>(instance), &Base::template ConstMethodStub<T, ConstMemberFunc>);
    }

private:
    Delegate(void *object, typename Base::Stub stub) : invocation(object, stub) {}

    friend class MulticastDelegate<Ret(Parms...)>;
    typename Base::Invocation invocation;
};

template <typename Ret, typename ...Parms>
class MulticastDelegate<Ret(Parms...)> final : private DelegateBase<Ret(Parms...)> {
private:
    using Base = DelegateBase<Ret(Parms...)>;
    using Invocation = typename Base::Invocation;

public:
    using Handle = DelegateHandle;

    MulticastDelegate() = default;
    MulticastDelegate(const MulticastDelegate &) = delete;

    ~MulticastDelegate() = default;

    bool IsEmpty() const {
        return invocationList.empty();
    }

    int Count() const {
        return (int)invocationList.size();
    }

    void Clear() {
        invocationList.clear();
    }

    MulticastDelegate &operator=(const MulticastDelegate &) = delete;

    Handle Add(const Delegate<Ret(Parms...)> &delegate) {
        if (delegate.IsNull()) {
            return Handle();
        }
        return Add(delegate.invocation.object, delegate.invocation.stub);
    }

    template <Ret(*Func)(Parms...)>
    Handle Add() {
        return Add(nullptr, &Base::template FunctionStub<Func>);
    }

    template <typename T, Ret(T::*MemberFunc)(Parms...)>
    Handle Add(T *instance) {
        return Add(instance, &Base::template MethodStub<T, MemberFunc>);
    }

    template <typename T, Ret(T::*ConstMemberFunc)(Parms...) const>
    Handle Add(const T *instance) {
        return Add(const_cast<T *>(instance), &Base::template ConstMethodStub<T, ConstMemberFunc>);
    }

    bool Remove(Handle handle) {
        if (!handle.IsValid()) {
            return false;
        }

        for (auto it = invocationList.begin(); it != invocationList.end(); ++it) {
            if (it->handle == handle) {
                invocationList.erase(it);
                return true;
            }
        }

        return false;
    }

    bool Remove(const Delegate<Ret(Parms...)> &delegate) {
        if (delegate.IsNull()) {
            return false;
        }

        for (auto it = invocationList.begin(); it != invocationList.end(); ++it) {
            if (it->Matches(delegate.invocation)) {
                invocationList.erase(it);
                return true;
            }
        }

        return false;
    }

    int RemoveAll(const void *object) {
        int removedCount = 0;

        for (auto it = invocationList.begin(); it != invocationList.end();) {
            if (it->object == object) {
                it = invocationList.erase(it);
                removedCount++;
            } else {
                ++it;
            }
        }

        return removedCount;
    }

    bool Contains(Handle handle) const {
        if (!handle.IsValid()) {
            return false;
        }

        for (const Invocation &invocation : invocationList) {
            if (invocation.handle == handle) {
                return true;
            }
        }

        return false;
    }

    bool Contains(const Delegate<Ret(Parms...)> &delegate) const {
        if (delegate.IsNull()) {
            return false;
        }

        for (const Invocation &invocation : invocationList) {
            if (invocation.Matches(delegate.invocation)) {
                return true;
            }
        }

        return false;
    }

    void Broadcast(Parms... args) const {
        for (const Invocation &invocation : invocationList) {
            invocation.stub(invocation.object, args...);
        }
    }

    void operator()(Parms... args) const {
        Broadcast(args...);
    }

private:
    Handle Add(void *object, typename Base::Stub stub) {
        if (!stub) {
            return Handle();
        }

        Handle handle(AllocHandleId());
        invocationList.emplace_back(object, stub, handle);
        return handle;
    }

    uint64_t AllocHandleId() {
        uint64_t id = nextHandleId++;
        if (nextHandleId == 0) {
            nextHandleId = 1;
        }
        return id;
    }

    std::list<Invocation>   invocationList;
    uint64_t                nextHandleId = 1;
};

BE_NAMESPACE_END
