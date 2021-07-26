#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define m_size_t		uint64_t

#define KB		(m_size_t)(0x1u << 10u)
#define MB		(m_size_t)(0x1u << 20u)
#define GB		(m_size_t)(0x1u << 30u)

#define MAX_MEM_SIZE	(32 * GB)

/* represent allocated memory block */
typedef struct _mem_block
{
	struct _mem_block *prev;
	struct _mem_block *next;
	void *start;
	void *use_start;;
	m_size_t alloc_mem;
	bool is_free;
} mem_block_t;

/* represent alloc a memory space*/
typedef struct _mem_space
{
	struct _mem_space *next;
	void *start;
	uint32_t id;
	m_size_t alloc_mem;
	m_size_t alloc_prog_mem;
	mem_block_t *free_list;
	mem_block_t *alloc_list;
	bool is_self_create;
} mem_space_t;

/*
 * represent alloc a memory pool
 * which include many memory spaces
 */
typedef struct _mem_pool
{
	uint32_t last_id;
	m_size_t mem_pool_size;
	m_size_t total_mem_pool_size;
	struct _mem_space *mlist;
	uint32_t align;
	bool auto_extend;
} mem_pool_t;

/*
 *	内部工具函数
 */

// 所有Memory的数量
void get_memory_list_count(mem_pool_t *mp, m_size_t *mlist_len);

// 每个Memory的统计信息
void get_memory_info(mem_space_t *mm, m_size_t *free_list_len, m_size_t *alloc_list_len);

uint32_t get_memory_id(mem_space_t *mm);

/*
 *	内存池API
 */

mem_pool_t *mem_pool_init(m_size_t mempoolsize, bool auto_extend);
mem_pool_t *mem_pool_init_restrict_space(void *start_addr, m_size_t mempoolsize, uint32_t align);

void *mem_pool_alloc(mem_pool_t *mp, m_size_t wantsize);

bool mem_pool_free(mem_pool_t *mp, void *p);

mem_pool_t *mem_pool_clear(mem_pool_t *mp);

bool mem_pool_destroy(mem_pool_t *mp);

/*
 *	获取内存池信息
 */

// 实际分配空间
uint64_t get_mempool_usage(mem_pool_t *mp);

// 真实使用空间
uint64_t get_mempool_prog_usage(mem_pool_t *mp);

#endif
