# Object pool

## What it is

`ObjectPool<T, Capacity>` (`src/core/ObjectPool.h`) holds objects that are created and destroyed all the time, without allocating memory while the game runs. The world keeps three: up to 64 hairballs, up to 128 of the player's bullets, and up to 128 of the boss's projectiles, the strands of its Hair Ring. The SMG fires 10 bullets a second, each one lives less than a second, and one ring is 42 strands at once, so without pools the game would allocate and free memory dozens of times a second, in the middle of a fight.

## How it works

The pool is a `std::array<T, Capacity>` and a count of live objects. The live objects are always the first `Size()` slots.

- **`Spawn(value)`** copies `value` into the first free slot (index `Size()`) and returns a pointer to it. When every slot is taken it returns `nullptr` and changes nothing.
- **`RemoveIf(predicate)`** calls the predicate once on every live object. When it returns true, the last live object is moved into that slot, the count goes down, and the moved object is checked next. The predicate may change the object it is given, so a system can update and cull in one pass. That is how bullets fly: `StepBullets` moves each bullet, resolves its hit, and returns whether it is done.
- **Iteration** (`begin`, `end`, `operator[]`) covers only the live objects, packed next to each other in memory.
- **`Clear()`** sets the count to zero.

```
slots:  [ A ][ B ][ C ][ D ][   ][   ]     Size() == 4
remove B: D moves into its slot
slots:  [ A ][ D ][ C ][   ][   ][   ]     Size() == 3
```

## Design decisions

- **Packed live objects, filled from the end on removal.** Iterating touches only live objects, with no gaps and no "alive" flag to check, and removal is one move. With bullets, the common work is "update every live one", so that is what the layout favours.
  - Rejected: a free list with an `alive` flag per slot. Slots keep a stable index, but every loop walks the dead slots too, and every system has to remember to skip them.
  - Rejected: `std::vector` with `erase` and `push_back`. It allocates when it grows, `erase` in the middle shifts every later element, and a copied vector does not keep its reserved capacity.
- **The order of the survivors changes on removal.** No system depends on the order of enemies or bullets. The simulation stays deterministic, because the same removals always produce the same order.
- **Pointers and indices are only valid until the next removal.** A removal moves the last object into the gap. Nothing in the game keeps a pointer to a bullet or an enemy across steps; `FirstHit` returns a pointer that is used immediately, before anything is removed.
- **A `std::array` inside the object, not a heap buffer.** The capacity is a template parameter, so the storage is part of the `World`. Copying a world (tests, and `Session` replacing it on restart) copies the pool with its full capacity, and no code path can make it allocate.
  - Rejected: a heap buffer sized at construction. It would need a hand-written copy constructor to keep the capacity, and a mistake there would bring allocations back.
- **A full pool refuses instead of growing.** The caller decides what "no room" means: a bullet that finds no slot is not fired, and a hairball that finds no slot is not spawned. Both capacities are well above what the game needs, so this is a safety net, not a rule players will meet.
- **One generic pool for every kind of object.** Hairballs, bullets and the boss's projectiles use the same class today; the markers of its summons and the particles will too.
- **A pool per side, not a flag per projectile.** The player's bullets and the boss's strands are both `Projectile`s, but they live in different pools, so the system that flies each pool only tests what that side can hit.

## How to tune it

| Value | Where | Default |
|---|---|---|
| Enemy pool | `kMaxEnemies` (`src/core/World.h`) | 64 |
| Bullet pool | `kMaxBullets` (`src/core/World.h`) | 128 |
| The boss's projectiles | `kMaxHostileProjectiles` (`src/core/World.h`) | 128: three Hair Rings at once |

A pool costs `Capacity` times the size of its object, inside the `World`: about 5 KB each for the enemies and both kinds of projectile.

## Tests

- `tests/core/ObjectPoolTests.cpp`: a new pool is empty and knows its capacity; spawning stores a copy and returns it; a full pool refuses instead of growing; `RemoveIf` removes exactly the matching objects and visits each live one once; `RemoveIf` can change the objects it keeps; a slot freed by a removal is used by the next spawn; clearing empties the pool; and a copied pool has the same live objects and its own storage.

## Limitations

- The capacity is fixed at compile time. If the boss ever needs more than 64 hairballs or 128 strands at once, the constant has to change. A ring that finds the pool full loses the strands that do not fit.

- No handles: there is no way to refer to a specific bullet or enemy across steps. When something needs that (a homing missile that tracks one enemy), the pool will need generation-checked handles.
