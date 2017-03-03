#include "BlueshiftEngine.h"
#include "TestContainer.h"

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

typedef BE1::HashMap<BE1::Str, BE1::LinkList<Bucket>, BE1::HashCompareStrIcmp, BE1::HashGeneratorIHash> testHashMap_t;

static void TestHashLinkMap() {
    testHashMap_t testHashMap;

    Bucket aa(1, 1, 1);
    Bucket bb(1, 2, 3);
    Bucket cc(3, 1, 2);
    Bucket dd(2, 1, 1);

    testHashMap.Set("aa", aa.node);
    testHashMap.Set("bb", bb.node);
    testHashMap.Set("cc", cc.node);

    testHashMap_t::KV *kv = testHashMap.Get("cc");
    if (kv) {
        dd.node.AddToEnd(kv->second);
    }

    for (int i = 0; i < testHashMap.Count(); i++) {
        const auto *entry = testHashMap.GetByIndex(i);
        const BE1::LinkList<Bucket> *node = &entry->second;
        for (Bucket *bucket = node->Owner(); bucket; bucket = bucket->node.Next()) {
            BE_LOG(L"%d %d %d\n", bucket->a, bucket->b, bucket->c);
        }
    }

    kv = testHashMap.Get("cc");
    for (Bucket *bucket = kv->second.Owner(); bucket; bucket = bucket->node.Next()) {
        if (bucket->a == 2) {
            BE_LOG(L"Found %d %d %d\n", bucket->a, bucket->b, bucket->c);
        }
    }
}

void TestContainer() {
    TestHashLinkMap();
}