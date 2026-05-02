#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <memory.h>

#include "memory/mem_ctx.h"

char prefix[100];
void *printing[100];
size_t printing_pos;

#define PRINT_PREF for(int _i=0;_i<size;_i++)printf("%c",prefix[_i]);
#define PRINT_NEXT(expr) if (expr) {printf("\t├- ");prefix[size + 1] = '|';}else{printf("\t└- ");prefix[size + 1] = ' ';}prefix[size] = '\t';


// Print the inorder traversal of the tree
// void print_pool_tree(const struct mem_pool *res, int size) {
//     if (res == NULL) {
//         printf("NULL\n");
//         return;
//     }
//     printf("%d(%c)\n", res->pool_size, (res->tree_node.color ? 'r' : 'b'));
//
//     PRINT_PREF
//     PRINT_NEXT(1)
//     print_pool_tree(res->tree_node.childs[0], size + 2);
//
//     PRINT_PREF
//     PRINT_NEXT(0)
//     print_pool_tree(res->tree_node.childs[1], size + 2);
// }

void perf_test(struct mem_ctx *ctx, const size_t size) {
    void *map[1000];
    clock_t my_time_alloc = 0, my_time_free = 0;
    clock_t time_alloc = 0, time_free = 0;

    for (size_t i = 0; i < 1000 * 10; i++) {
        {
            clock_t s = clock();
            for (int j = 0; j < 1000; j++) {
                map[j] = mem_malloc(ctx, size);
                memset(map[j], 0, size);
            }
            my_time_alloc += clock() - s;

            s = clock();
            for (int j = 0; j < 1000; j++)
                mem_free(ctx, map[j]);
            my_time_free += clock() - s;
        }
        {
            clock_t s = clock();
            for (int j = 0; j < 1000; j++) {
                map[j] = malloc(size);
                memset(map[j], 0, size);
            }
            time_alloc += clock() - s;

            s = clock();
            for (int j = 0; j < 1000; j++)
                free(map[j]);
            time_free += clock() - s;
        }
    }
    printf("          malloc   free\n");
    printf("my_time : %f %f\n", (double) my_time_alloc / (double) CLOCKS_PER_SEC, (double) my_time_free / (double) CLOCKS_PER_SEC);
    printf("time    : %f %f\n", (double) time_alloc / (double) CLOCKS_PER_SEC, (double) time_free / (double) CLOCKS_PER_SEC);
}

int main() {
    struct mem_ctx ctx = {0};
    mem_malloc(&ctx, 1 << 1);
    mem_malloc(&ctx, 1 << 2);
    mem_malloc(&ctx, 1 << 3);
    mem_malloc(&ctx, 1 << 4);
    mem_malloc(&ctx, 1 << 5);
    mem_malloc(&ctx, 1 << 6);
    mem_malloc(&ctx, 1 << 7);
    mem_malloc(&ctx, 1 << 8);
    mem_malloc(&ctx, 1 << 9);
    mem_malloc(&ctx, 1 << 10);
    mem_malloc(&ctx, 1 << 11);
    mem_malloc(&ctx, 1 << 12);
    perf_test(&ctx, 1024 * 4);
    // void *vals[100];
    // for (int i = 1; i < 16; i++) {
    //     vals[i] = mem_malloc(&ctx, i);
    // }
    // print_pool_tree(ctx.pools_root, 0);
    //
    // for (int i = 1; i < 16; i++) {
    //     mem_free(&ctx, vals[i]);
    // }
    // print_pool_tree(ctx.pools_root, 0);

    return 0;
}


// Implementing Red-Black Tree in C
