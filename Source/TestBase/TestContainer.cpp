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

#define CHECK(expression) do { if (!(expression)) { \
    std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expression); \
    std::abort(); \
} } while (false)

static void TestLinkList() {
    struct Item {
        Item() { node.SetOwner(this); }
        BE1::LinkList<Item> node;
    };
    BE1::LinkList<Item> list;
    Item first, second, third;
    CHECK(list.IsListEmpty() && list.Count() == 0);
    CHECK(list.Next() == nullptr && list.Prev() == nullptr);
    first.node.AddToEnd(list);
    second.node.AddToEnd(list);
    third.node.AddToFront(list);
    CHECK(list.Count() == 3 && list.Next() == &third);
    CHECK(third.node.Next() == &first && first.node.Prev() == &third);
    CHECK(first.node.Next() == &second && second.node.Prev() == &first);
    CHECK(third.node.Prev() == nullptr && second.node.Next() == nullptr);
    first.node.Remove();
    CHECK(!first.node.InList() && list.Count() == 2);
    CHECK(third.node.Next() == &second && second.node.Prev() == &third);
    list.Clear();
    CHECK(list.IsListEmpty() && list.Count() == 0);
    CHECK(!second.node.InList() && !third.node.InList());
    std::puts("LinkList tests passed");
}

template <typename Map>
static void TestStringHashLookup(bool caseSensitive, int hashSize) {
    Map map;
    const Map &view = map;
    static_assert(std::is_same<decltype(map.Get("key")), typename Map::KV *>::value, "Mutable lookup must return mutable entries");
    static_assert(std::is_same<decltype(view.Get("key")), const typename Map::KV *>::value, "Const lookup must return const entries");
    CHECK(map.Get("missing") == nullptr);
    CHECK(view.Get(nullptr) == nullptr);

    // Test both a forced collision chain and normal multi-bucket hash selection.
    map.Init(hashSize, 16, 16);
    const char *longKey = "Textures/Characters/VeryLongCharacterName/Body_Diffuse.texture";
    const char *otherCase = "textures/characters/verylongcharactername/body_diffuse.texture";
    const char *missingKey = "Textures/Characters/VeryLongCharacterName/Body_Normal.texture";
    map.Set(longKey, 10);
    map.Set("", 20);
    map.Set("short", 30);

    const BE1::Str storedKey(longKey);
    const auto *expected = view.Get(storedKey);
    CHECK(expected && expected->second == 10);
    CHECK(map.Get(longKey) == expected);
    CHECK(view.Get(longKey) == expected);
    CHECK(map.Get(otherCase) == (caseSensitive ? nullptr : expected));
    CHECK(view.Get(otherCase) == (caseSensitive ? nullptr : expected));
    CHECK(map.Get(missingKey) == nullptr);
    CHECK(view.Get(missingKey) == nullptr);
    CHECK(map.Get("")->second == 20);
    CHECK(view.Get(nullptr) == view.Get(""));
    CHECK(map.Get(nullptr) == map.Get(""));
    const char *nullKey = nullptr;
    char *mutableNullKey = nullptr;
    CHECK(map.Get(nullKey) == map.Get(""));
    CHECK(view.Get(mutableNullKey) == view.Get(""));

    char mutableKey[] = "Textures/Characters/VeryLongCharacterName/Body_Diffuse.texture";
    CHECK(map.Get(mutableKey) == expected);
    CHECK(view.Get(mutableKey) == expected);
    map.Get(mutableKey)->second = 11;
    mutableKey[0] = 'X';
    CHECK(map.Get(mutableKey) == nullptr);
    CHECK(view.Get(storedKey)->second == 11);
    CHECK(view.Get(longKey)->first == storedKey);
    CHECK(map.Count() == 3);

    map.Clear();
    CHECK(map.Get(longKey) == nullptr);
    CHECK(view.Get("") == nullptr);
}

struct StringLengthHash {
    static int Hash(const BE1::HashIndex &hasher, const BE1::Str &key) {
        return hasher.GenerateHash(key.Length());
    }
};

struct CaseInsensitiveStringCompare {
    static bool Compare(const BE1::Str &lhs, const BE1::Str &rhs) {
        return lhs.Icmp(rhs) == 0;
    }
};

static void TestHashMapKeyTypes() {
    BE1::HashMap<int, int> integers;
    integers.Set(7, 70);
    CHECK(integers.Get(7)->second == 70);
    CHECK(integers.Get(8) == nullptr);

    BE1::HashMap<int64_t, int> wideIntegers;
    wideIntegers.Set(INT64_C(0x100000001), 42);
    CHECK(wideIntegers.Get(INT64_C(0x100000001))->second == 42);
    CHECK(wideIntegers.Get(1) == nullptr);

    int key = 0;
    BE1::HashMap<int *, int> pointers;
    pointers.Set(&key, 12);
    CHECK(pointers.Get(&key)->second == 12);
    CHECK(pointers.Get(nullptr) == nullptr);

    // Custom policies must agree on equality, including when distinct keys collide.
    BE1::HashMap<BE1::Str, int, CaseInsensitiveStringCompare, StringLengthHash> custom;
    custom.Set("Alpha", 10);
    custom.Set("Bravo", 20);
    custom.Set("BRAVO", 30);
    const auto &view = custom;
    CHECK(custom.Count() == 2);
    CHECK(view.Get("ALPHA")->second == 10);
    CHECK(view.Get("bravo")->second == 30);
    CHECK(view.Get("Gamma") == nullptr);
}

static void TestHashMapStringLookup() {
    for (int hashSize : { 1, 32 }) {
        TestStringHashLookup<BE1::StrHashMap<int>>(true, hashSize);
        TestStringHashLookup<BE1::HashMap<BE1::Str, int, BE1::HashCompareStrCmp>>(true, hashSize);
        TestStringHashLookup<BE1::StrIHashMap<int>>(false, hashSize);
    }
    TestHashMapKeyTypes();
    std::puts("HashMap lookup tests passed");
}

template <typename Map>
static void TestHashMapInsertionAndUpdate(int hashSize, bool caseSensitive) {
    Map map;
    // Start unallocated and grow on each new key, with and without hash collisions.
    map.Init(hashSize, 0, 1);
    const BE1::Str firstKey("Textures/Characters/CharacterA/Body_Diffuse.texture");
    const BE1::Str secondKey("Textures/Characters/CharacterA/Body_Normal.texture");
    const BE1::Str thirdKey("Textures/Characters/CharacterA/Body_Roughness.texture");
    const BE1::Str copiedValue("copied value");
    auto *entry = map.Set(firstKey, copiedValue);
    CHECK(map.Count() == 1);
    CHECK(entry->first == firstKey && entry->second == copiedValue);

    const BE1::Str firstQuery(caseSensitive ? firstKey.c_str() : "textures/characters/charactera/body_diffuse.texture");
    const BE1::Str updatedValue("updated copy");
    entry = map.Set(firstQuery, updatedValue);
    CHECK(map.Count() == 1);
    CHECK(entry->first == firstKey && entry->second == updatedValue);

    BE1::Str movedValue("moved value");
    entry = map.Set(secondKey, std::move(movedValue));
    CHECK(map.Count() == 2);
    CHECK(entry->first == secondKey && entry->second == "moved value");

    entry = map.Set(secondKey, BE1::Str("updated move"));
    CHECK(map.Count() == 2);
    CHECK(entry->second == "updated move");

    BE1::Str &inserted = map[thirdKey];
    CHECK(map.Count() == 3);
    CHECK(inserted.IsEmpty());
    inserted = "indexed value";
    BE1::Str &existing = map[thirdKey];
    CHECK(map.Count() == 3);
    CHECK(&existing == &inserted && existing == "indexed value");

    // A shared key/value argument must preserve both stored strings after the move.
    BE1::Str keyAndValue("Textures/Characters/CharacterA/Body_Emissive.texture");
    const BE1::Str expectedKey(keyAndValue);
    entry = map.Set(keyAndValue, std::move(keyAndValue));
    CHECK(map.Count() == 4);
    CHECK(entry->first == expectedKey && entry->second == expectedKey);
    CHECK(map.Get(expectedKey) == entry);
    CHECK(map.Get(firstQuery)->second == updatedValue);
    CHECK(map.Get(secondKey)->second == "updated move");
    CHECK(map.Get(thirdKey)->second == "indexed value");
}

static void TestHashMapInsertion() {
    for (int hashSize : { 1, 32 }) {
        TestHashMapInsertionAndUpdate<BE1::StrHashMap<BE1::Str>>(hashSize, true);
        TestHashMapInsertionAndUpdate<BE1::StrIHashMap<BE1::Str>>(hashSize, false);
    }

    BE1::HashMap<int, std::unique_ptr<int>> moveOnly;
    auto value = std::make_unique<int>(10);
    int *expected = value.get();
    auto *entry = moveOnly.Set(1, std::move(value));
    CHECK(moveOnly.Count() == 1);
    CHECK(!value && entry->second.get() == expected);

    value = std::make_unique<int>(20);
    expected = value.get();
    entry = moveOnly.Set(1, std::move(value));
    CHECK(moveOnly.Count() == 1);
    CHECK(!value && entry->second.get() == expected);

    auto &inserted = moveOnly[2];
    CHECK(moveOnly.Count() == 2 && !inserted);
    inserted = std::make_unique<int>(30);
    CHECK(moveOnly[2].get() == inserted.get());
    CHECK(moveOnly.Count() == 2);
    std::puts("HashMap insertion tests passed");
}

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
        CHECK(failed && ThrowingMove::live == source.Count());
        source.Reset();
        CHECK(ThrowingMove::live == 0);
        source.EmplaceBack(1);
        source.EmplaceBack(2);
        source.EmplaceBack(3);

        Queue assigned;
        assigned.EmplaceBack(9);
        ThrowingMove::movesBeforeThrow = 1;
        failed = false;
        try {
            assigned = std::move(source);
        } catch (const std::runtime_error &) {
            failed = true;
        }
        CHECK(failed);
        CHECK(ThrowingMove::live == assigned.Count() + source.Count());
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
    TestLinkList();
    TestHashMapStringLookup();
    TestHashMapInsertion();
    TestRingBuffer();
    TestStaticRingBuffer();
}
