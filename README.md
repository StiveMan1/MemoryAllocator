# Custom Memory Allocator

A custom memory allocator written in C for experimenting with fast small-object
allocation. The allocator is built around pages, pools, fixed-size blocks, and a
small context object that tracks allocator state.

The benchmark in `main.c` compares this allocator with the standard
`malloc`/`free` path. For workloads with many repeated small allocations, this
pool-based design can be much faster than a general-purpose allocator.

## Features

- Fast allocation for small blocks up to `4096` bytes.
- Size-class selection as part of a best-fit strategy.
- Significantly reduced fragmentation by separating allocations into pools by
  aligned size class.
- Automatic fallback to `malloc`, `calloc`, `realloc`, and `free` for larger
  allocations.
- Per-pool free lists for reusing released blocks.
- Red-black tree lookup for finding the page that owns a pointer.
- Reusable pages: empty pages are kept for future allocations.
- Public API for `mem_malloc`, `mem_calloc`, `mem_realloc`, and `mem_free`.

## Project Layout

```text
.
├── main.c                # Benchmark and allocator demo
├── memory/
│   ├── mem_ctx.c         # Core allocator implementation
│   ├── mem_ctx.h         # Allocator context and public API
│   ├── mem_interfaces.h  # Internal list and tree helpers
│   ├── mem_page.h        # Page metadata
│   └── mem_pool.h        # Pool metadata and constants
├── Makefile              # Simple GCC build
├── CMakeLists.txt        # CMake build configuration
└── README.md
```

## How It Works

The allocator stores memory in this structure:

```text
mem_ctx -> pages -> pools -> fixed-size blocks
```

Each page owns `64` pools. Each pool owns `4096` bytes and serves one block
size. When a small allocation is requested, the allocator rounds the request up
to the nearest supported power-of-two size class and returns a block from a pool
for that class.

Requests larger than `4096` bytes are passed to the standard library allocator.

## Allocation Strategy

This allocator uses size-class selection as part of a best-fit strategy. It does
not scan every free block in the heap. Instead, it chooses the smallest
power-of-two size class that can hold the request.

Examples:

- `13` bytes -> `16` byte block
- `100` bytes -> `128` byte block
- `2000` bytes -> `2048` byte block
- `4097` bytes -> system `malloc`

This significantly reduced fragmentation compared with placing many different
object sizes into one shared region. Small objects stay with small objects,
larger pooled objects stay with their own size class, and freed blocks are
reused by future allocations of the same size.

The tradeoff is internal fragmentation: because requests are rounded up, some
bytes inside each block may be unused.

## Free Lists

When a block is freed, the allocator stores the previous free-list pointer
directly inside that freed block. This avoids extra metadata allocation and
makes allocation/free operations very small.

Because the free-list pointer lives inside freed memory, writing to memory after
`mem_free` can corrupt the allocator's internal state. This allocator does not
currently detect use-after-free, double-free, or invalid-free bugs.

## Pages And Pools

Pools are allocated from pages. A page tracks which pools are currently used and
which pools are available for reuse. When a pool becomes empty, it can be
returned to the page. When a page has no active pools, it is removed from the
active page tree and kept in a free-page list for later reuse.

The allocator also keeps a red-black tree of active pages. During `mem_free` and
`mem_realloc`, this tree is used to find whether a pointer belongs to a managed
page or should be handled by the standard allocator.

## Why There Is No Coalescing

Classic coalescing merges neighboring free blocks into one larger block. That is
useful for variable-size heap allocators, but this allocator is based on fixed
size classes.

Each pool serves exactly one block size, so freed blocks simply return to the
free list for that pool. Merging neighboring blocks would add complexity without
helping the allocator's main workload.

## Why There Are No Boundary Tags

Boundary tags usually store metadata before and after each allocation so an
allocator can find neighboring blocks and coalesce them.

This allocator does not coalesce blocks, and each pool already knows the block
size it serves. Adding headers and footers would increase memory overhead, add
extra writes on the hot path, and could push requests into larger size classes.

## API Example

```c
#include "memory/mem_ctx.h"

int main(void) {
    struct mem_ctx ctx = {0};

    int *values = mem_calloc(&ctx, 16, sizeof(int));
    values = mem_realloc(&ctx, values, 32 * sizeof(int));

    mem_free(&ctx, values);
    return 0;
}
```

Use the same `mem_ctx` for allocations that should share allocator state.

## Build And Run

With Make:

```bash
make my_memory
make run
./main.a
```

With CMake:

```bash
cmake -S . -B build
cmake --build build
./build/memory_allocator
```

## Benchmark

`main.c` runs repeated allocation, zero-fill, and free cycles:

```c
perf_test(&ctx, 1024 * 4);
```

The output compares the custom allocator with standard allocation:

```text
          malloc   free
my_time : ...
time    : ...
```

This benchmark is useful for a quick comparison, but it is not a complete
allocator performance study. Real performance depends on allocation size,
allocation lifetime, reuse patterns, CPU cache behavior, and the system
allocator.

## Limitations

- Educational and experimental code, not production-ready.
- Not thread-safe.
- No built-in detection for double-free, invalid-free, or use-after-free.
- `mem_calloc` does not currently check for multiplication overflow.
- Empty pages are cached for reuse instead of being unmapped immediately.
- Benchmark coverage is narrow and should be expanded.

## Roadmap

- Add leak detection and allocation reports.
- Add allocator visualization for pages, pools, and size classes.
- Add thread-safety or thread-local contexts.
- Expand benchmarks across more allocation patterns.
- Add focused tests for allocation, reuse, large fallback, and `realloc`.
