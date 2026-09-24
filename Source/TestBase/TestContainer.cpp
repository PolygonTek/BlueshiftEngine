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

#include "BlueshiftEngine.h"
#include "Test.h"
#include <deque>
#include <random>

class Bucket {
public:
    int a, b, c;
    BE1::LinkList<Bucket> node;
    Bucket(int a, int b, int c);
};

BE_INLINE Bucket::Bucket(int a, int b, int c) {
    this->a = a;
    this->b = b;
    this->c = c;
    node.SetOwner(this);
}

typedef BE1::HashMap<BE1::Str, BE1::LinkList<Bucket>*, BE1::HashCompareStrIcmp, BE1::HashGeneratorIHash> testHashMap_t;

static void TestHashLinkMap() {
    testHashMap_t testHashMap;

    Bucket aa(1, 1, 1);
    Bucket bb(1, 2, 3);
    Bucket cc(3, 1, 2);
    Bucket dd(2, 1, 1);

    testHashMap.Set("aa", &aa.node);
    testHashMap.Set("bb", &bb.node);
    testHashMap.Set("cc", &cc.node);

    testHashMap_t::KV *kv = testHashMap.Get("cc");
    if (kv) {
        dd.node.AddToEnd(*kv->second);
    }

    for (int i = 0; i < testHashMap.Count(); i++) {
        const auto *entry = testHashMap.GetByIndex(i);
        const BE1::LinkList<Bucket> *node = entry->second;
        for (Bucket *bucket = node->Owner(); bucket; bucket = bucket->node.Next()) {
            BE_LOG("%d %d %d\n", bucket->a, bucket->b, bucket->c);
        }
    }

    kv = testHashMap.Get("cc");
    for (Bucket *bucket = kv->second->Owner(); bucket; bucket = bucket->node.Next()) {
        if (bucket->a == 2) {
            BE_LOG("Found %d %d %d\n", bucket->a, bucket->b, bucket->c);
        }
    }
}

#define CHECK(expression) do { if (!(expression)) { \
    std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expression); \
    std::abort(); \
} } while (false)

static void TestWrapAndGrowth() {
    BE1::RingBuffer<int> queue;
    CHECK(queue.IsEmpty());
    CHECK(queue.Capacity() == 0);
    CHECK(!queue.TryPushBack(1));
    CHECK(queue.Capacity() == 0);

    queue.Reserve(3);
    for (int value = 1; value <= 3; ++value) {
        CHECK(queue.TryPushBack(value));
    }
    CHECK(queue.IsFull());
    CHECK(!queue.TryPushBack(4));
    CHECK(queue.PopFront() == 1);
    CHECK(queue.TryPushBack(4));
    CHECK(queue[0] == 2 && queue[1] == 3 && queue[2] == 4);

    // Growth must also accept an argument referring to the wrapped buffer itself.
    queue.PushBack(queue.Front());
    CHECK(queue.Capacity() == 6);
    CHECK(queue.Count() == 4);
    const auto &view = queue;
    CHECK(view.Front() == 2 && view.Back() == 2);
    CHECK(view[1] == 3 && view[2] == 4);
    CHECK(queue.PopFront() == 2);
    CHECK(queue.PopFront() == 3);
    CHECK(queue.PopFront() == 4);
    CHECK(queue.PopFront() == 2);

    queue.Reserve(2);
    CHECK(queue.Capacity() == 6);
    queue.Reset();
    CHECK(queue.IsEmpty() && queue.Capacity() == 6);
    queue.Clear();
    CHECK(queue.IsEmpty() && queue.Capacity() == 0);

    BE1::RingBuffer<int> single(1);
    for (int value = 0; value < 100; ++value) {
        CHECK(single.TryPushBack(value));
        CHECK(!single.TryPushBack(-1));
        CHECK(single.PopFront() == value);
    }
}

static void TestPersistentBacklog() {
    int frames[2] = {};
    BE1::RingBuffer<int *> queue(2);
    queue.PushBack(&frames[0]);
    queue.PushBack(&frames[1]);
    for (int i = 0; i < 100000; ++i) {
        int *frame = queue.PopFront();
        CHECK(frame == &frames[i & 1]);
        queue.PushBack(frame);
        CHECK(queue.Count() == 2 && queue.Capacity() == 2);
    }
}

static void TestAgainstDeque() {
    std::mt19937 random(73471);
    std::deque<int> expected;
    BE1::RingBuffer<int> queue(3);

    for (int i = 0; i < 100000; ++i) {
        const unsigned operation = random() % 10;
        if (operation < 4) {
            queue.PushBack(i);
            expected.push_back(i);
        } else if (operation < 7 && !expected.empty()) {
            CHECK(queue.PopFront() == expected.front());
            expected.pop_front();
        } else if (operation == 7) {
            const int capacity = queue.Capacity();
            const bool accepted = queue.TryPushBack(i);
            CHECK(accepted == (expected.size() < static_cast<size_t>(capacity)));
            CHECK(queue.Capacity() == capacity);
            if (accepted) {
                expected.push_back(i);
            }
        } else if (operation == 8 && queue.Capacity() < 256) {
            queue.Reserve(queue.Capacity() + 7);
        } else if (operation == 9 && i % 100 == 0) {
            const int capacity = queue.Capacity();
            queue.Reset();
            expected.clear();
            CHECK(queue.Capacity() == capacity);
        }

        CHECK(queue.Count() == static_cast<int>(expected.size()));
        CHECK(queue.IsEmpty() == expected.empty());
        for (int index = 0; index < queue.Count(); ++index) {
            CHECK(queue[index] == expected[index]);
        }
    }
}

struct Lifetime {
    static int live;
    int value;

    explicit Lifetime(int value) : value(value) { ++live; }
    Lifetime(const Lifetime &) = delete;
    Lifetime &operator=(const Lifetime &) = delete;
    Lifetime(Lifetime &&rhs) noexcept : value(rhs.value) { rhs.value = -1; ++live; }
    ~Lifetime() { --live; }
};

int Lifetime::live = 0;

static void TestLifetimeAndMove() {
    {
        BE1::RingBuffer<Lifetime> queue(3);
        CHECK(Lifetime::live == 0);
        CHECK(queue.TryEmplaceBack(1));
        CHECK(queue.TryEmplaceBack(2));
        CHECK(queue.TryEmplaceBack(3));
        CHECK(!queue.TryEmplaceBack(4));
        CHECK(Lifetime::live == 3);
        {
            Lifetime value = queue.PopFront();
            CHECK(value.value == 1 && Lifetime::live == 3);
        }
        CHECK(Lifetime::live == 2);
        queue.EmplaceBack(4);
        queue.Reserve(7);
        CHECK(Lifetime::live == 3);
        CHECK(queue[0].value == 2 && queue[1].value == 3 && queue[2].value == 4);

        BE1::RingBuffer<Lifetime> moved(std::move(queue));
        CHECK(queue.IsEmpty() && queue.Capacity() == 0);
        CHECK(moved.Count() == 3);
        BE1::RingBuffer<Lifetime> assigned;
        assigned.EmplaceBack(9);
        assigned = std::move(moved);
        CHECK(Lifetime::live == 3);
        CHECK(moved.IsEmpty() && moved.Capacity() == 0);
        assigned = std::move(assigned);
        CHECK(assigned.Front().value == 2);
        assigned.Reset();
        CHECK(Lifetime::live == 0 && assigned.Capacity() == 7);
        assigned.EmplaceBack(5);
    }
    CHECK(Lifetime::live == 0);
}

static void TestOwnershipOnRejection() {
    BE1::RingBuffer<std::unique_ptr<int>> queue(1);
    queue.PushBack(std::make_unique<int>(1));
    auto next = std::make_unique<int>(2);
    CHECK(!queue.TryPushBack(std::move(next)));
    CHECK(next && *next == 2);
    CHECK(*queue.Front() == 1);
    queue.PushBack(std::move(next));
    CHECK(!next && queue.Count() == 2);
    CHECK(*queue.PopFront() == 1);
    CHECK(*queue.PopFront() == 2);
}

struct alignas(128) AlignedValue {
    int value;
    explicit AlignedValue(int value) : value(value) {}
};

static void TestAlignment() {
    BE1::RingBuffer<AlignedValue> queue(3);
    for (int i = 0; i < 50; ++i) {
        queue.EmplaceBack(i);
    }
    for (int i = 0; i < queue.Count(); ++i) {
        CHECK(reinterpret_cast<uintptr_t>(&queue[i]) % alignof(AlignedValue) == 0);
        CHECK(queue[i].value == i);
    }
}

struct CopyOnGrow {
    static int live;
    static int copiesBeforeThrow;
    int value;

    explicit CopyOnGrow(int value) : value(value) {
        if (value < 0) {
            throw std::runtime_error("construction failure");
        }
        ++live;
    }
    CopyOnGrow(const CopyOnGrow &rhs) : value(rhs.value) {
        if (copiesBeforeThrow == 0) {
            throw std::runtime_error("copy failure");
        }
        if (copiesBeforeThrow > 0) {
            --copiesBeforeThrow;
        }
        ++live;
    }
    CopyOnGrow(CopyOnGrow &&rhs) noexcept(false) : value(rhs.value) { ++live; }
    ~CopyOnGrow() { --live; }
};

int CopyOnGrow::live = 0;
int CopyOnGrow::copiesBeforeThrow = -1;

static void TestConstructionFailures() {
    {
        BE1::RingBuffer<CopyOnGrow> queue(2);
        queue.EmplaceBack(1);
        queue.EmplaceBack(2);
        CHECK(queue.PopFront().value == 1);
        queue.EmplaceBack(3);

        CopyOnGrow::copiesBeforeThrow = 1;
        bool failed = false;
        try {
            queue.Reserve(8);
        } catch (const std::runtime_error &) {
            failed = true;
        }
        CHECK(failed && CopyOnGrow::live == 2);
        CHECK(queue.Capacity() == 2 && queue.Count() == 2);
        CHECK(queue.Front().value == 2 && queue.Back().value == 3);

        CopyOnGrow::copiesBeforeThrow = 2;
        failed = false;
        try {
            queue.PushBack(queue.Front());
        } catch (const std::runtime_error &) {
            failed = true;
        }
        CHECK(failed && CopyOnGrow::live == 2);
        CHECK(queue.Capacity() == 2 && queue.Count() == 2);
        CHECK(queue.Front().value == 2 && queue.Back().value == 3);

        CopyOnGrow::copiesBeforeThrow = -1;
        failed = false;
        try {
            queue.EmplaceBack(-1);
        } catch (const std::runtime_error &) {
            failed = true;
        }
        CHECK(failed && CopyOnGrow::live == 2);
        CHECK(queue.Capacity() == 2 && queue.Count() == 2);
        queue.PushBack(queue.Back());
        CHECK(queue.Count() == 3 && CopyOnGrow::live == 3);
        CHECK(queue.PopFront().value == 2);
        CHECK(queue.PopFront().value == 3);
        CHECK(queue.PopFront().value == 3);
    }
    CHECK(CopyOnGrow::live == 0);
}

static void TestRingBuffer() {
    TestWrapAndGrowth();
    TestPersistentBacklog();
    TestAgainstDeque();
    TestLifetimeAndMove();
    TestOwnershipOnRejection();
    TestAlignment();
    TestConstructionFailures();
    std::puts("RingBuffer tests passed");
}

static void TestStaticWrapAndCapacity() {
    BE1::StaticRingBuffer<int, 3> queue;
    CHECK(queue.IsEmpty() && !queue.IsFull() && queue.Capacity() == 3);
    for (int value = 1; value <= 3; ++value) {
        CHECK(queue.TryPushBack(value));
    }
    CHECK(queue.IsFull());
    CHECK(!queue.TryPushBack(4));
    CHECK(queue.PopFront() == 1);
    int *second = &queue.Front();
    queue.PushBack(queue.Front());
    CHECK(&queue.Front() == second);

    const auto &view = queue;
    CHECK(view.Front() == 2 && view.Back() == 2);
    CHECK(view[0] == 2 && view[1] == 3 && view[2] == 2);
    CHECK(queue.PopFront() == 2);
    CHECK(queue.PopFront() == 3);
    CHECK(queue.PopFront() == 2);

    queue.PushBack(5);
    queue.Reset();
    CHECK(queue.IsEmpty() && queue.Capacity() == 3);
    queue.PushBack(6);
    queue.Clear();
    CHECK(queue.IsEmpty() && queue.Capacity() == 3);

    BE1::StaticRingBuffer<int, 1> single;
    for (int value = 0; value < 100; ++value) {
        CHECK(single.TryPushBack(value));
        CHECK(single.IsFull() && !single.TryPushBack(-1));
        CHECK(single.PopFront() == value);
    }
}

static void TestStaticAgainstDeque() {
    std::mt19937 random(84631);
    std::deque<int> expected;
    BE1::StaticRingBuffer<int, 7> queue;

    for (int i = 0; i < 100000; ++i) {
        const unsigned operation = random() % 10;
        if (operation < 6) {
            const bool accepted = queue.TryPushBack(i);
            CHECK(accepted == (expected.size() < 7));
            if (accepted) {
                expected.push_back(i);
            }
        } else if (operation < 9 && !expected.empty()) {
            CHECK(queue.PopFront() == expected.front());
            expected.pop_front();
        } else if (operation == 9 && i % 100 == 0) {
            queue.Reset();
            expected.clear();
        }

        CHECK(queue.Count() == static_cast<int>(expected.size()));
        CHECK(queue.IsEmpty() == expected.empty());
        CHECK(queue.IsFull() == (expected.size() == 7));
        CHECK(queue.Capacity() == 7);
        for (int index = 0; index < queue.Count(); ++index) {
            CHECK(queue[index] == expected[index]);
        }
    }
}

static void TestStaticLifetimeAndMove() {
    using Queue = BE1::StaticRingBuffer<Lifetime, 3>;
    static_assert(!std::is_copy_constructible_v<Queue>);
    static_assert(std::is_nothrow_move_constructible_v<Queue>);
    static_assert(std::is_nothrow_move_assignable_v<Queue>);
    {
        Queue queue;
        CHECK(Lifetime::live == 0);
        queue.EmplaceBack(1);
        queue.EmplaceBack(2);
        queue.EmplaceBack(3);
        CHECK(!queue.TryEmplaceBack(4) && Lifetime::live == 3);
        CHECK(queue.PopFront().value == 1);
        queue.EmplaceBack(4);

        Queue moved(std::move(queue));
        CHECK(queue.IsEmpty() && queue.Capacity() == 3);
        CHECK(moved.Count() == 3 && Lifetime::live == 3);
        CHECK(moved[0].value == 2 && moved[1].value == 3 && moved[2].value == 4);
        queue.EmplaceBack(5);
        CHECK(queue.PopFront().value == 5);

        Queue assigned;
        assigned.EmplaceBack(9);
        assigned = std::move(moved);
        CHECK(moved.IsEmpty() && moved.Capacity() == 3);
        CHECK(assigned.Count() == 3 && Lifetime::live == 3);
        assigned = std::move(assigned);
        CHECK(assigned.Front().value == 2 && assigned.Back().value == 4);

        // Reset must destroy a wrapped live range, and only that range.
        CHECK(assigned.PopFront().value == 2);
        assigned.EmplaceBack(6);
        assigned.Reset();
        CHECK(Lifetime::live == 0 && assigned.Capacity() == 3);
        assigned.EmplaceBack(7);
        assigned.Clear();
        CHECK(Lifetime::live == 0 && assigned.IsEmpty());
        assigned.EmplaceBack(8);
    }
    CHECK(Lifetime::live == 0);
}

static void TestStaticOwnershipOnRejection() {
    BE1::StaticRingBuffer<std::unique_ptr<int>, 1> queue;
    queue.PushBack(std::make_unique<int>(1));
    auto next = std::make_unique<int>(2);
    CHECK(!queue.TryPushBack(std::move(next)));
    CHECK(next && *next == 2);

    bool failed = false;
    try {
        queue.PushBack(std::move(next));
    } catch (const std::length_error &) {
        failed = true;
    }
    CHECK(failed && next && *next == 2);
    CHECK(queue.Count() == 1 && *queue.Front() == 1);
    CHECK(*queue.PopFront() == 1);
    queue.PushBack(std::move(next));
    CHECK(!next && *queue.PopFront() == 2);
}

static void TestStaticAlignmentAndStorage() {
    BE1::StaticRingBuffer<AlignedValue, 3> queue;
    const uintptr_t storageBegin = reinterpret_cast<uintptr_t>(&queue);
    const uintptr_t storageEnd = storageBegin + sizeof(queue);
    for (int i = 0; i < 50; ++i) {
        if (queue.IsFull()) {
            CHECK(queue.PopFront().value == i - 3);
        }
        AlignedValue &element = queue.EmplaceBack(i);
        const uintptr_t address = reinterpret_cast<uintptr_t>(&element);
        CHECK(address % alignof(AlignedValue) == 0);
        CHECK(address >= storageBegin && address + sizeof(element) <= storageEnd);
        CHECK(queue.Back().value == i);
    }

    // Reusing a slot must also work for types with const subobjects.
    struct ImmutableValue {
        const int value;
        explicit ImmutableValue(int value) : value(value) {}
    };
    BE1::StaticRingBuffer<ImmutableValue, 1> single;
    for (int i = 0; i < 100; ++i) {
        single.EmplaceBack(i);
        CHECK(single.Front().value == i);
        CHECK(single.PopFront().value == i);
    }
}

struct ThrowingMove {
    static int live;
    static int movesBeforeThrow;
    int value;

    explicit ThrowingMove(int value) : value(value) { ++live; }
    ThrowingMove(const ThrowingMove &) = delete;
    ThrowingMove(ThrowingMove &&rhs) : value(rhs.value) {
        if (movesBeforeThrow == 0) {
            throw std::runtime_error("move failure");
        }
        if (movesBeforeThrow > 0) {
            --movesBeforeThrow;
        }
        rhs.value = -1;
        ++live;
    }
    ~ThrowingMove() { --live; }
};

int ThrowingMove::live = 0;
int ThrowingMove::movesBeforeThrow = -1;

static void TestStaticConstructionFailures() {
    {
        BE1::StaticRingBuffer<CopyOnGrow, 2> queue;
        queue.EmplaceBack(1);
        CHECK(queue.PopFront().value == 1);
        queue.EmplaceBack(2);
        bool failed = false;
        try {
            (void)queue.TryEmplaceBack(-1);
        } catch (const std::runtime_error &) {
            failed = true;
        }
        CHECK(failed && queue.Count() == 1 && CopyOnGrow::live == 1);
        CHECK(queue.Front().value == 2);

        CopyOnGrow::copiesBeforeThrow = 0;
        failed = false;
        try {
            queue.PopFront();
        } catch (const std::runtime_error &) {
            failed = true;
        }
        CHECK(failed && queue.Count() == 1 && queue.Front().value == 2);
        CopyOnGrow::copiesBeforeThrow = -1;
        queue.EmplaceBack(3);
        CHECK(queue.PopFront().value == 2);
        CHECK(queue.PopFront().value == 3);
    }
    CHECK(CopyOnGrow::live == 0);

    using Queue = BE1::StaticRingBuffer<ThrowingMove, 3>;
    static_assert(!std::is_nothrow_move_constructible_v<Queue>);
    static_assert(!std::is_nothrow_move_assignable_v<Queue>);
    {
        Queue source;
        source.EmplaceBack(1);
        source.EmplaceBack(2);
        source.EmplaceBack(3);

        ThrowingMove::movesBeforeThrow = 1;
        bool failed = false;
        try {
            Queue moved(std::move(source));
        } catch (const std::runtime_error &) {
            failed = true;
        }
        CHECK(failed && ThrowingMove::live == 3 && source.Count() == 3);
        CHECK(source[0].value == -1 && source[1].value == 2 && source[2].value == 3);

        Queue assigned;
        assigned.EmplaceBack(9);
        ThrowingMove::movesBeforeThrow = 1;
        failed = false;
        try {
            assigned = std::move(source);
        } catch (const std::runtime_error &) {
            failed = true;
        }
        CHECK(failed && assigned.Count() == 1 && source.Count() == 3);
        CHECK(ThrowingMove::live == 4);
        assigned.Reset();
        source.Reset();
        CHECK(ThrowingMove::live == 0);
        ThrowingMove::movesBeforeThrow = -1;
        CHECK(source.TryEmplaceBack(4));
        CHECK(assigned.TryEmplaceBack(5));
    }
    CHECK(ThrowingMove::live == 0);
}

static void TestStaticRingBuffer() {
    TestStaticWrapAndCapacity();
    TestStaticAgainstDeque();
    TestStaticLifetimeAndMove();
    TestStaticOwnershipOnRejection();
    TestStaticAlignmentAndStorage();
    TestStaticConstructionFailures();
    std::puts("StaticRingBuffer tests passed");
}

#undef CHECK

void Test::TestContainer() {
    TestHashLinkMap();
    TestRingBuffer();
    TestStaticRingBuffer();
}
