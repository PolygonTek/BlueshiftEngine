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

//
// The Impossibly Fast C++ Delegates, Fixed
// by Sergey Alexandrovich Kryukov
// https://www.codeproject.com/script/Articles/ViewDownloads.aspx?aid=1170503
//

#include <list>

BE_NAMESPACE_BEGIN

template <typename T>
class DelegateBase;

template <typename Ret, typename ...Parms>
class DelegateBase<Ret(Parms...)> {
protected:
    using Stub = Ret(*)(void *thisPtr, Parms...);

    struct InvocationElement {
        InvocationElement() = default;
        InvocationElement(void *thisPtr, Stub aStub) : object(thisPtr), stub(aStub) {}

        void Clone(InvocationElement &target) const {
            target.stub = stub;
            target.object = object;
        }
        bool operator==(const InvocationElement &rhs) const {
            return rhs.stub == stub && rhs.object == object;
        }
        bool operator!=(const InvocationElement &rhs) const {
            return rhs.stub != stub || rhs.object != object;
        }

        void *object = nullptr;
        Stub stub = nullptr;
    };
};

template <typename T>
class Delegate;

template <typename T>
class MulticastDelegate;

template <typename Ret, typename ...Parms>
class Delegate<Ret(Parms...)> final : private DelegateBase<Ret(Parms...)> {
public:
    Delegate() = default;

    Delegate(const Delegate &another) {
        another.invocation.Clone(invocation);
    }

    template <typename Lambda>
    Delegate(const Lambda &lambda) {
        invocation.object = (void *)(&lambda);
        invocation.stub = lambda_stub<Lambda>;
    }

    Delegate &operator=(const Delegate &rhs) {
        rhs.invocation.Clone(invocation);
        return *this;
    }

    // Template instantiation is not needed, will be deduced (inferred)
    template <typename Lambda>
    Delegate &operator=(const Lambda &instance) {
        invocation.object = (void *)(&instance);
        invocation.stub = lambda_stub<Lambda>;
        return *this;
    }

    bool IsNull() const {
        return invocation.stub == nullptr;
    }

    bool operator==(void *ptr) const {
        return ptr == nullptr && IsNull();this
    }

    bool operator!=(void *ptr) const {
        return ptr != nullptr || !IsNull();
    }

    bool operator==(const Delegate &rhs) const {
        return invocation == rhs.invocation;
    }

    bool operator!=(const Delegate &rhs) const {
        return invocation != rhs.invocation;
    }

    Ret operator()(Parms... args) const {
        return (*invocation.stub)(invocation.object, args...);
    }

    template <Ret(*Func)(Parms...)>
    static Delegate FromFunc() {
        return Delegate(nullptr, function_stub<Func>);
    }

    template <typename T, Ret(T::*MemberFunc)(Parms...)>
    static Delegate FromMemberFunc(T *instance) {
        return Delegate(instance, method_stub<T, MemberFunc>);
    }

    template <typename T, Ret(T::*ConstMemberFunc)(Parms...) const>
    static Delegate FromMemberFunc(const T *instance) {
        return Delegate(const_cast<T *>(instance), const_method_stub<T, ConstMemberFunc>);
    }

    template <typename Lambda>
    static Delegate FromLambda(const Lambda &instance) {
        return Delegate((void *)(&instance), lambda_stub<Lambda>);
    }

private:
    Delegate(void *object, typename DelegateBase<Ret(Parms...)>::Stub stub) {
        invocation.object = object;
        invocation.stub = stub;
    }

    template <Ret(*Func)(Parms...)>
    static Ret function_stub(void *thisPtr, Parms... params) {
        return (Func)(params...);
    }

    template <typename T, Ret(T:: *MemberFunc)(Parms...)>
    static Ret method_stub(void *thisPtr, Parms... params) {
        T *p = static_cast<T *>(thisPtr);
        return (p->*MemberFunc)(params...);
    }

    template <typename T, Ret(T:: *ConstMemberFunc)(Parms...) const>
    static Ret const_method_stub(void *thisPtr, Parms... params) {
        T *const p = static_cast<T *>(thisPtr);
        return (p->*ConstMemberFunc)(params...);
    }

    template <typename Lambda>
    static Ret lambda_stub(void *thisPtr, Parms... args) {
        Lambda *p = static_cast<Lambda *>(thisPtr);
        return (p->operator())(args...);
    }

    friend class MulticastDelegate<Ret(Parms...)>;
    typename DelegateBase<Ret(Parms...)>::InvocationElement invocation;
};

template <typename Ret, typename ...Parms>
class MulticastDelegate<Ret(Parms...)> final : private DelegateBase<Ret(Parms...)> {
public:
    MulticastDelegate() = default;
    MulticastDelegate(const MulticastDelegate &) = delete;

    ~MulticastDelegate() {
        Clear();
    }

    const std::list<typename DelegateBase<Ret(Parms...)>::InvocationElement *> &GetInvocationList() const {
        return invocationList;
    }

    bool IsEmpty() const {
        return invocationList.size() == 0;
    }

    void Clear() {
        for (auto &element : invocationList) {
            delete element;
        }
        invocationList.clear();
    }

    MulticastDelegate &operator=(const MulticastDelegate &) = delete;

    MulticastDelegate &operator+=(const MulticastDelegate &rhs) {
        for (auto &item : rhs.invocationList) { // Clone, not copy; flattens hierarchy:
            invocationList.push_back(new typename DelegateBase<Ret(Parms...)>::InvocationElement(item->object, item->stub));
        }
        return *this;
    }

    // Template instantiation is not needed, will be deduced/inferred:
    template <typename Lambda>
    MulticastDelegate &operator+=(const Lambda &lambda) {
        Delegate<Ret(Parms...)> d = Delegate<Ret(Parms...)>::template FromLambda<Lambda>(lambda);
        return *this += d;
    }

    MulticastDelegate &operator+=(const Delegate<Ret(Parms...)> &rhs) {
        if (rhs.IsNull()) {
            return *this;
        }
        invocationList.push_back(new typename DelegateBase<Ret(Parms...)>::InvocationElement(rhs.invocation.object, rhs.invocation.stub));
        return *this;
    }

    bool operator==(void *ptr) const {
        return ptr == nullptr && IsNull();
    }

    bool operator==(const MulticastDelegate &rhs) const {
        if (invocationList.size() != rhs.invocationList.size()) {
            return false;
        }
        auto anotherIt = rhs.invocationList.begin();
        for (auto it = invocationList.begin(); it != invocationList.end(); ++it) {
            if (**it != **anotherIt) return false;
        }
        return true;
    }
        
    bool operator!=(void *ptr) const {
        return ptr != nullptr || !IsNull();
    }

    bool operator!=(const MulticastDelegate &rhs) const {
        return *this != rhs;
    }

    // Will work even if Ret is void, return values are ignored:
    // (for handling return values, see operator(..., handler))
    void operator()(Parms... args) const {
        for (auto &item : invocationList) {
            (*(item->stub))(item->object, args...);
        }
    }

    template <typename Handler>
    void operator()(Parms... args, Handler handler) const {
        size_t index = 0;
        for (auto &item : invocationList) {
            Ret value = (*(item->stub))(item->object, args...);
            handler(index, &value);
            ++index;
        }
    }

    void operator()(Parms... args, Delegate<void(size_t, Ret *)> handler) const {
        operator()<decltype(handler)>(args..., handler);
    }

    void operator()(Parms... args, std::function<void(size_t, Ret *)> handler) const {
        operator()<decltype(handler)>(args..., handler);
    }

private:
    std::list<typename DelegateBase<Ret(Parms...)>::InvocationElement *> invocationList;
};

#define DELEGATE_FROM_FUNCTION(func, ret, ...)                  Delegate<ret(__VA_ARGS__)>::FromFunc<func>()
#define DELEGATE_FROM_METHOD(object, cls, memberFunc, ret, ...) Delegate<ret(__VA_ARGS__)>::FromMemberFunc<cls, &cls::memberFunc>(&object)
#define DELEGATE_FROM_LAMBDA(lambda, ret, ...)                  Delegate<ret(__VA_ARGS__)>::FromLambda(lambda)

BE_NAMESPACE_END
