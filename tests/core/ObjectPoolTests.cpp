#include <doctest/doctest.h>

#include "core/ObjectPool.h"

using horde::ObjectPool;

namespace {

template <std::size_t Capacity>
int Sum(const ObjectPool<int, Capacity>& pool) {
    int sum = 0;
    for (const int value : pool) {
        sum += value;
    }
    return sum;
}

} // namespace

TEST_CASE("a new pool is empty and knows its capacity") {
    const ObjectPool<int, 8> pool;
    CHECK(pool.Empty());
    CHECK(pool.Size() == 0);
    CHECK(ObjectPool<int, 8>::MaxSize() == 8);
    CHECK(pool.begin() == pool.end());
}

TEST_CASE("spawning stores a copy and returns it") {
    ObjectPool<int, 8> pool;
    int* spawned = pool.Spawn(42);
    REQUIRE(spawned != nullptr);
    CHECK(*spawned == 42);
    CHECK(pool.Size() == 1);
    CHECK(pool[0] == 42);
}

TEST_CASE("a full pool refuses to spawn instead of growing") {
    ObjectPool<int, 2> pool;
    CHECK(pool.Spawn(1) != nullptr);
    CHECK(pool.Spawn(2) != nullptr);
    CHECK(pool.Spawn(3) == nullptr);
    CHECK(pool.Size() == 2);
}

TEST_CASE("RemoveIf removes exactly the matching objects and visits each live one once") {
    ObjectPool<int, 8> pool;
    for (int i = 1; i <= 6; ++i) {
        pool.Spawn(i);
    }
    int visits = 0;
    const std::size_t removed = pool.RemoveIf([&visits](int value) {
        ++visits;
        return value % 2 == 0;
    });
    CHECK(visits == 6);
    CHECK(removed == 3);
    CHECK(pool.Size() == 3);
    CHECK(Sum(pool) == 1 + 3 + 5);
}

TEST_CASE("RemoveIf can change the objects it keeps") {
    ObjectPool<int, 8> pool;
    pool.Spawn(1);
    pool.Spawn(2);
    pool.RemoveIf([](int& value) {
        value *= 10;
        return false;
    });
    CHECK(Sum(pool) == 30);
}

TEST_CASE("a slot freed by a removal is used by the next spawn") {
    ObjectPool<int, 2> pool;
    pool.Spawn(1);
    pool.Spawn(2);
    pool.RemoveIf([](int value) { return value == 1; });
    CHECK(pool.Spawn(3) != nullptr);
    CHECK(Sum(pool) == 5);
}

TEST_CASE("clearing empties the pool") {
    ObjectPool<int, 8> pool;
    pool.Spawn(1);
    pool.Spawn(2);
    pool.Clear();
    CHECK(pool.Empty());
    CHECK(Sum(pool) == 0);
}

TEST_CASE("a copied pool has the same live objects and its own storage") {
    ObjectPool<int, 8> original;
    original.Spawn(7);
    ObjectPool<int, 8> copy = original;
    copy.Spawn(8);
    CHECK(original.Size() == 1);
    CHECK(copy.Size() == 2);
    CHECK(Sum(copy) == 15);
}
