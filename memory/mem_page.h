//
// Created by 04024 on 06.07.2024.
//

#ifndef MEM_PAGE_H
#define MEM_PAGE_H

#include "mem_pool.h"

struct mem_page {
    struct __list_node list_node;
    struct __tree_node tree_node;
    struct __allocator allocator;

    struct mem_pool pools[POOL_NUMBER];
    void *data;
};

#endif //MEM_PAGE_H
