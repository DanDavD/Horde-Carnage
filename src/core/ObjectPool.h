#pragma once

#include <array>
#include <cstddef>
#include <utility>

namespace horde {

/// A fixed number of slots for short-lived objects (bullets, enemies), with no memory allocation after
/// construction.
///
/// Live objects are packed at the front of a `std::array`: spawning writes the next free slot, and
/// removing moves the last live object into the gap. Iteration only touches live objects, and they sit
/// next to each other in memory. The cost is that removal changes the order of the survivors and moves
/// them, so a pointer or index into the pool is only valid until the next removal.
template <typename T, std::size_t Capacity>
class ObjectPool {
public:
    /// Copies `value` into a free slot and returns it, or returns nullptr when every slot is taken.
    /// A full pool is the caller's decision: a bullet that finds no slot is simply not fired.
    T* Spawn(const T& value) {
        if (_count == Capacity) {
            return nullptr;
        }
        T& slot = _items[_count++];
        slot = value;
        return &slot;
    }

    /// Calls `predicate` once on every live object, and removes the ones it returns true for. The
    /// predicate may change the object it is given; that is how a system updates and culls in one pass.
    ///
    /// @return How many objects were removed.
    template <typename Predicate>
    std::size_t RemoveIf(Predicate predicate) {
        std::size_t removed = 0;
        std::size_t i = 0;
        while (i < _count) {
            if (predicate(_items[i])) {
                // The last live object fills the gap and is checked next, so each one is visited once.
                _items[i] = std::move(_items[_count - 1]);
                --_count;
                ++removed;
            } else {
                ++i;
            }
        }
        return removed;
    }

    void Clear() {
        _count = 0;
    }

    [[nodiscard]] std::size_t Size() const {
        return _count;
    }

    [[nodiscard]] bool Empty() const {
        return _count == 0;
    }

    [[nodiscard]] static constexpr std::size_t MaxSize() {
        return Capacity;
    }

    [[nodiscard]] T& operator[](std::size_t i) {
        return _items[i];
    }

    [[nodiscard]] const T& operator[](std::size_t i) const {
        return _items[i];
    }

    // Lower case so range-based for loops work.
    [[nodiscard]] T* begin() {
        return _items.data();
    }

    [[nodiscard]] T* end() {
        return _items.data() + _count;
    }

    [[nodiscard]] const T* begin() const {
        return _items.data();
    }

    [[nodiscard]] const T* end() const {
        return _items.data() + _count;
    }

private:
    std::array<T, Capacity> _items{};
    std::size_t _count = 0;
};

} // namespace horde
