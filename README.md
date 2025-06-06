# ⚡ Custom Memory Allocator — ~10x Faster Than `malloc`/`free`

This is a fun little project where we built our own memory allocator in C.\
Yes, we know `malloc` and `free` are good — but we made it better (for small sizes, at least).\
The allocator here can be up to 10x faster for small and frequent allocations.

## 📁 What's Inside?
This project is structured like this

```graphql
./
├── main.c                # Compares classic malloc/free vs our custom allocator
├── memory/
│   ├── mem_ctx.c         # Core allocator logic (pool management, tree handling)
│   ├── mem_ctx.h         # Context and allocator interface
│   ├── mem_page.h        # Page structs (holds multiple pools)
│   ├── mem_pool.h        # Pool structs (chunks of memory to serve allocs)
│   └── mem_interfaces.h  # Low-level list/tree utilities
├── Makefile              # Simple build and run rules
├── CMakeLists.txt        # If you prefer CMake
└── .gitignore            # Usual stuff
```

## 🚀 How It Works (Short Version)

- We use pages, which contain pools, which hold actual memory.
- Small allocations (<= 4096 bytes) go through the custom system.
- Bigger ones just fall back to regular `malloc`.
- Internally, there's a **Red-Black Tree** for fast memory page lookup.
- Pools are recycled, and we keep track of used/free chunks manually.

## 🧪 `main.c` - Benchmarking

In `main.c`, we benchmark both allocators by doing a ton of allocations and deallocations:

```c
perf_test(&ctx, 1024 * 4);
```

The results show clear speedups for the custom one when working with lots of small blocks. \
Like... really noticeable improvements.


## 🛠️ Build & Run

### 🔧 With Make:
```bash
make my_memory
make run
./main.a
```

### 🧱 Or with CMake:
```bash
mkdir build && cd build
cmake ..
make
./memory_allocator
```

## ⚠️ Notes

- This is an educational / experimental project.
- It’s tuned for small, frequent allocations — not general-purpose replacement (yet).
- Some parts are still being refined (like tree visualization or advanced reuse logic).

## 💡 Why?

Because `malloc` is a general-purpose beast, and sometimes, **you just need something leaner**.\
Also — building allocators is fun 😄

## 📝 Planning

1. [ ] Add support for realloc
2. [ ] Add leak detection / reporting
3. [ ] Visualize allocator structure (page/pool map)
4. [ ] Add thread-safety (maybe with __thread or lock-free queues)
5. [ ] Integrate into real-world app or game engine
6. [ ] Benchmark with different allocation patterns (not just 4KB blocks)
