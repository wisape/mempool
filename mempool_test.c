#include <stdio.h>
#include "mempool.h"

#define ALLOC_MEM_NUM		1000
int main(int argc, char const *argv[])
{
	mem_pool_t *pool;
	void *mem[ALLOC_MEM_NUM];
	int i;


	pool = mem_pool_init_restrict_space((void *)0x10000000, 0x1000 * 997, 0x1000);
	if (pool == NULL) {
		printf("mem pool init\n");
		return -1;
	}

	for (i = 0; i < ALLOC_MEM_NUM; i++) {
		//mem[i] = mem_pool_alloc(pool, (i + 1) * 123);
		mem[i] = mem_pool_alloc(pool, 123);
		if (mem[i] != NULL) {
			printf("Get alloc mem[%d] start[%p]\n", i, mem[i]);
		} else {
			printf("Get alloc mem[%d] failed\n", i);
		}

		if (i % 10 == 0) {
			if (i > 0) {
				mem_pool_free(pool, mem[i - 10]);
				mem[i - 10] = NULL;
				printf("Get Free mem[%d]\n", i - 10);
			}
		}
	}

	for (i = 0; i < ALLOC_MEM_NUM; i++) {
		if (mem[i] != NULL) {
			mem_pool_free(pool, mem[i]);
		}
	}

	printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	for (i = 0; i < ALLOC_MEM_NUM / 10; i++) {
		mem[i] = mem_pool_alloc(pool, 4999);
		if (mem[i] != NULL) {
			printf("Get alloc mem[%d] start[%p]\n", i, mem[i]);
		} else {
			printf("Get alloc mem[%d] failed\n", i);
		}
	}

	for (i = 0; i < ALLOC_MEM_NUM / 10; i++) {
		if (mem[i] != NULL) {
			mem_pool_free(pool, mem[i]);
		}
	}

	(void)mem_pool_destroy(pool);
    return 0;
}
