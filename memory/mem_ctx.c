#include "mem_ctx.h"

#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>


#define MEM_MAP(size) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0)
#define MEM_PTR(pool) (*(void **) (pool))


static struct mem_page *mem_page_create() {
    struct mem_page *res = MEM_MAP(sizeof(struct mem_page));
    if (res == MAP_FAILED) return NULL;
    memset(res, 0, sizeof(struct mem_page));
    res->data = MEM_MAP(POOL_SIZE * POOL_NUMBER);
    if (res->data == MAP_FAILED) {
        munmap(res, sizeof(struct mem_page));
        return NULL;
    }

    for (int i = 0; i < POOL_NUMBER; i++)
        res->pools[i].data_pos = res->data + POOL_SIZE * i;


    return res;
}

static uint16_t get_pool_size(const uint64_t size) {
    uint16_t pool_size = 2;
    while (size > (1ULL << ++pool_size)) {}
    return pool_size;
}

// Page Tree

static struct mem_page *mem_page_init(struct mem_ctx *ctx) {
    struct mem_page *page = ctx->free_pages.first;

    if (page == NULL) page = mem_page_create();
    else mem_list_take(&ctx->free_pages, &page->list_node);

    if (page == NULL) return NULL;

    page->tree_node.color = RBTREE_RED;
    page->tree_node.childs[0] = NULL;
    page->tree_node.childs[1] = NULL;

    mem_tree_page_insert(ctx, page);
    return page;
}

static void mem_page_free(struct mem_ctx *ctx, struct mem_page *page) {
    mem_tree_page_delete(ctx, page);
    mem_list_put(&ctx->free_pages, &page->list_node);
}


struct mem_pool *mem_alloc_pool(struct mem_ctx *ctx) {
    struct mem_page *page = ctx->pages_list.first;
    if (page == NULL || page->allocator.filled == POOL_NUMBER) {
        page = mem_page_init(ctx);
        if (page == NULL) return NULL;

        mem_list_put(&ctx->pages_list, &page->list_node);
    }

    struct mem_pool *pool = &page->pools[page->allocator.filled];
    if (page->allocator.first_free != NULL)
        page->allocator.first_free = (pool = page->allocator.first_free)->allocator.first_free;

    pool->allocator.first_free = NULL;
    if (++page->allocator.filled == POOL_NUMBER) mem_list_spin(&ctx->pages_list);
    return pool;
}

void mem_free_pool(struct mem_ctx *ctx, struct mem_page *page, struct mem_pool *pool) {
    pool->allocator.first_free = page->allocator.first_free;
    page->allocator.first_free = pool;

    mem_list_take(&ctx->pages_list, &page->list_node);

    if (--page->allocator.filled == 0) mem_page_free(ctx, page);
    else mem_list_put(&ctx->pages_list, &page->list_node);
}


// Pool Tree
void *mem_malloc(struct mem_ctx *ctx, const uint64_t size) {
    const uint64_t pool_size = get_pool_size(size);
    if (pool_size > 12) return malloc(size);

    struct mem_pool *pool = ctx->pools_map[pool_size - 3].first;
    if (pool == NULL || pool->allocator.filled == POOL_SIZE) {
        pool = mem_alloc_pool(ctx);
        if (pool == NULL) return NULL;

        pool->pool_size = pool_size;
        mem_list_put(&ctx->pools_map[pool_size - 3], &pool->list_node);
    }

    void *res = pool->data_pos + pool->allocator.filled;
    if (pool->allocator.first_free != NULL) pool->allocator.first_free = MEM_PTR(res = pool->allocator.first_free);

    if ((pool->allocator.filled += 1 << pool->pool_size) == POOL_SIZE) mem_list_spin(&ctx->pools_map[pool_size - 3]);
    return res;
}

void *mem_calloc(struct mem_ctx *ctx, const uint64_t num, const uint64_t size) {
    const uint64_t full_size = num * size;
    const uint64_t pool_size = get_pool_size(full_size);
    if (pool_size > 12) return calloc(num, size);

    struct mem_pool *pool = ctx->pools_map[pool_size - 3].first;
    if (pool == NULL || pool->allocator.filled == POOL_SIZE) {
        pool = mem_alloc_pool(ctx);
        if (pool == NULL) return NULL;

        pool->pool_size = pool_size;
        mem_list_put(&ctx->pools_map[pool_size - 3], &pool->list_node);
    }

    void *res = pool->data_pos + pool->allocator.filled;
    if (pool->allocator.first_free != NULL) pool->allocator.first_free = MEM_PTR(res = pool->allocator.first_free);
    memset(res, 0, full_size);

    if ((pool->allocator.filled += 1 << pool->pool_size) == POOL_SIZE) mem_list_spin(&ctx->pools_map[pool_size - 3]);
    return res;
}

void *mem_realloc(struct mem_ctx *ctx, void *old_ptr, const uint64_t new_size) {
    struct mem_page *page = mem_tree_page_find(ctx, old_ptr);
    if (page == NULL) return realloc(old_ptr, new_size);

    struct mem_pool *pool = &page->pools[((uint64_t) old_ptr - (uint64_t) page->data) / POOL_SIZE];
    const uint64_t pool_size = get_pool_size(new_size);

    if (pool_size <= pool->pool_size) return old_ptr;

    void *res = pool_size > 12 ? malloc(new_size) : mem_malloc(ctx, new_size);
    if (res == NULL) return NULL;

    memcpy(res, old_ptr, 1 << pool->pool_size);

    MEM_PTR(old_ptr) = pool->allocator.first_free;
    pool->allocator.first_free = old_ptr;

    mem_list_take(&ctx->pools_map[pool->pool_size - 3], &pool->list_node);

    if ((pool->allocator.filled -= 1 << pool->pool_size) == 0) mem_free_pool(ctx, page, pool);
    else mem_list_put(&ctx->pools_map[pool->pool_size - 3], &pool->list_node);

    return res;
}

void mem_free(struct mem_ctx *ctx, void *data) {
    struct mem_page *page = mem_tree_page_find(ctx, data);
    if (page == NULL) return free(data);

    struct mem_pool *pool = &page->pools[((uint64_t) data - (uint64_t) page->data) / POOL_SIZE];

    MEM_PTR(data) = pool->allocator.first_free;
    pool->allocator.first_free = data;

    mem_list_take(&ctx->pools_map[pool->pool_size - 3], &pool->list_node);

    if ((pool->allocator.filled -= 1 << pool->pool_size) == 0) mem_free_pool(ctx, page, pool);
    else mem_list_put(&ctx->pools_map[pool->pool_size - 3], &pool->list_node);
}
