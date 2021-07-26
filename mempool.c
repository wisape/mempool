#include "mempool.h"
#include <stdio.h>

#define dlinklist_insert_front(head,x) do { \
	x->prev = NULL; \
	x->next = head; \
	if (head) \
		head->prev = x; \
	head = x; \
} while(0)

#define dlinklist_delete(head,x) do { \
	if (!x->prev) { \
		head = x->next; \
		if (x->next) x->next->prev = NULL; \
	} else { \
		x->prev->next = x->next; \
		if (x->next) x->next->prev = x->prev; \
	} \
} while(0)

static void mem_space_init(mem_pool_t *mp, mem_space_t *mm)
{
	if ((mm == NULL) || (mp == NULL)) {
		return;
	}

	mm->alloc_mem = 0;
	mm->alloc_prog_mem = 0;
	mm->free_list->is_free = 1;
	mm->free_list->start = mm->start;
	mm->free_list->alloc_mem = mp->mem_pool_size;
	mm->free_list->prev = NULL;
	mm->free_list->next = NULL;
	mm->alloc_list = NULL;
}

void get_memory_list_count(mem_pool_t *mp, m_size_t *mlist_len)
{
	m_size_t mlist_l = 0;
	mem_space_t *mm = mp->mlist;

	while (mm != NULL) {
		mlist_l++;
		mm = mm->next;
	}

	*mlist_len = mlist_l;
}

void get_memory_info(mem_space_t *mm, m_size_t *free_list_len, m_size_t *alloc_list_len)
{
	m_size_t free_l = 0, alloc_l = 0;
	mem_block_t *p = mm->free_list;

	while (p != NULL) {
		free_l++;
		p = p->next;
	}

	p = mm->alloc_list;
	while (p != NULL) {
		alloc_l++;
		p = p->next;
	}

	*free_list_len = free_l;
	*alloc_list_len = alloc_l;
}

uint32_t get_memory_id(mem_space_t *mm)
{
	return mm->id;
}

static mem_space_t *extend_memory_list(mem_pool_t *mp)
{
	mem_space_t *mm = (mem_space_t *)malloc(sizeof(mem_space_t));
	if (mm == NULL) {
		return NULL;
	}

	mm->start = (uint8_t *)malloc(mp->mem_pool_size * sizeof(uint8_t));
	if (mm->start == NULL) {
		return NULL;
	}

	mem_space_init(mp, mm);
	mm->id = mp->last_id++;
	mm->next = mp->mlist;
	mp->mlist = mm;
	return mm;
}

static mem_space_t *find_memory_list(mem_pool_t *mp, void *p)
{
	mem_space_t *tmp = mp->mlist;

	while (tmp != NULL) {
		if ((tmp->start <= p) && (tmp->start + mp->mem_pool_size > p)) {
			break;
		}
		tmp = tmp->next;
	}

	return tmp;
}

static bool merge_free_mem_block(mem_pool_t *mp, mem_space_t *mm, mem_block_t *c)
{
	if ((mp == NULL) || (mm == NULL) || (c == NULL)) {
		return false;
	}

	mem_block_t *p0 = c->prev, *p1 = c;
	while (p0 && p0->is_free && p1) {
		if (p0->start + p0->alloc_mem == p1->start) {
			p0->alloc_mem += p1->alloc_mem;
			dlinklist_delete(mm->free_list, p1);
			free(p1);

			p1 = p0;
			p0 = p1->prev;
		} else {
			break;
		}
	}

	mem_block_t *p2 = p1->next;

	while (p2 && p2->is_free && p1) {
		if (p1->start + p1->alloc_mem == p2->start) {
			p1->alloc_mem += p2->alloc_mem;
			dlinklist_delete(mm->free_list, p2);
			free(p2);

			p2 = p1->next;
		} else {
			break;
		}
	}

	return true;
}

mem_pool_t *mem_pool_init(m_size_t size, bool auto_extend)
{
	mem_pool_t *mp;
	uint8_t *space;

	if (size > MAX_MEM_SIZE) {
		return NULL;
	}

	mp = (mem_pool_t *)malloc(sizeof(mem_pool_t));
	if (mp == NULL) {
		return NULL;
	}

	mp->last_id = 0;
	mp->total_mem_pool_size = size;
	mp->mem_pool_size = size;
	mp->auto_extend = auto_extend;

	mp->mlist = (mem_space_t *)malloc(sizeof(mem_space_t));
	if (mp->mlist == NULL) {
		free(mp);
		return NULL;
	}

	/* alloc mem pool first mem space*/
	space = (uint8_t *)malloc(sizeof(uint8_t) * mp->mem_pool_size);
	if (space == NULL) {
		free(mp->mlist);
		free(mp);
		return NULL;
	}

	mp->mlist->start = space;
	mp->mlist->is_self_create = true;
	mem_space_init(mp, mp->mlist);
	mp->mlist->next = NULL;
	mp->mlist->id = mp->last_id;
	mp->last_id++;

	return mp;
}

mem_pool_t *mem_pool_init_restrict_space(void *start, m_size_t size, uint32_t align)
{
	mem_pool_t *mp;

	if (size > MAX_MEM_SIZE) {
		return NULL;
	}

	if (start == NULL) {
		return NULL;
	}

	mp = (mem_pool_t *)malloc(sizeof(mem_pool_t));
	if (mp == NULL) {
		return NULL;
	}

	mp->last_id = 0;
	mp->total_mem_pool_size = size;
	mp->mem_pool_size = size;
	mp->auto_extend = false;

	mp->mlist = (mem_space_t *)malloc(sizeof(mem_space_t));
	if (mp->mlist == NULL) {
		free(mp);
		return NULL;
	}

	mp->mlist->free_list = (mem_block_t *)malloc(sizeof(mem_block_t));
	if (mp->mlist->free_list == NULL) {
		free(mp->mlist);
		free(mp);
		return NULL;
	}

	mp->mlist->start = start;
	mp->mlist->is_self_create = false;
	mem_space_init(mp, mp->mlist);
	mp->mlist->next = NULL;
	mp->align = align;
	mp->mlist->id = mp->last_id;
	mp->last_id++;

	return mp;
}

void *mem_pool_alloc(mem_pool_t *mp, m_size_t wantsize)
{
	mem_space_t *mm = NULL, *mm1 = NULL;
	mem_block_t *_free = NULL, *_not_free = NULL;
	uint32_t gap = 0;
	m_size_t total_needed_size = wantsize;

	if (total_needed_size > mp->mem_pool_size) {
		return NULL;
	}

	do {
		mm = mp->mlist;
		while (mm != NULL) {
			if (mp->mem_pool_size - mm->alloc_mem < total_needed_size) {
				mm = mm->next;
				continue;
			}

			_free = mm->free_list;
			_not_free = NULL;

			while (_free != NULL) {
				if ((uint64_t)(_free->start) % mp->align != 0) {
					gap = mp->align - ((uint64_t)_free->start % mp->align);
				} else {
					gap = 0;
				}

				total_needed_size += gap;
				if (_free->alloc_mem  >= total_needed_size) {
					if ((_free->alloc_mem - total_needed_size) > 0) {
						_not_free = _free;
						_free = (mem_block_t *)malloc(sizeof(mem_block_t));

						*_free = *_not_free;
						_free->start += total_needed_size;
						_free->alloc_mem -= total_needed_size;


						// update free_list
						if (_free->prev == NULL) {
							mm->free_list = _free;
							if (_free->next != NULL) {
								_free->next->prev = _free;
							}
						} else {
							_free->prev->next = _free;
							if (_free->next != NULL) {
								_free->next->prev = _free;
							}
						}

						_not_free->is_free = 0;
						_not_free->alloc_mem = total_needed_size;

					} else {
						_not_free = _free;
						dlinklist_delete(mm->free_list, _not_free);
						_not_free->is_free = 0;
					}

					dlinklist_insert_front(mm->alloc_list, _not_free);

					mm->alloc_mem += _not_free->alloc_mem;
					mm->alloc_prog_mem += _not_free->alloc_mem;
					_not_free->use_start = (void *)((uint8_t *)_not_free->start + gap);

					return (void *)((uint8_t *)_not_free->use_start);
				}

				_free = _free->next;
			}

			mm = mm->next;
		}

		if (mp->auto_extend) {
			mm1 = extend_memory_list(mp);
			if (mm1 == NULL) {
				return NULL;
			}
			mp->total_mem_pool_size += mp->mem_pool_size;
		}
	} while(mp->auto_extend);

	return NULL;
}

bool mem_pool_free(mem_pool_t *mp, void *p)
{
	if ((p == NULL) || (mp == NULL)) {
		return false;
	}

	mem_space_t *mm = mp->mlist;
	if (mp->auto_extend) {
		mm = find_memory_list(mp, p);
	}

	mem_block_t *ck = NULL;
	for (ck = mm->alloc_list; ck != NULL; ck = ck->next) {
		if (ck->use_start == p)
			break;
	}

	dlinklist_delete(mm->alloc_list, ck);

	mem_block_t *mid;
	for(mid = mm->free_list; mid != NULL; mid = mid->next) {
		if (mid->next) {
			if ((mid->start < ck->start) && mid->next->start > ck->start) {
				ck->next = mid->next;
				mid->next->prev = ck;
				mid->next = ck;
				ck->prev = mid;
				break;
			}
		} else {
			if (mid->start < ck->start) {
				mid->next = ck;
				ck->prev = mid;
				ck->next = NULL;
				break;
			}
		}

		if (mid->prev == NULL) {
			if (mid->start > ck->start) {
				ck->prev = mid->prev;
				ck->next = mid;
				mid->prev = ck;
				if (mid == mm->free_list) {
					mm->free_list = ck;
				}

				break;
			}
		}
	}
	ck->is_free = true;

	mm->alloc_mem -= ck->alloc_mem;
	mm->alloc_prog_mem -= ck->alloc_mem;

	merge_free_mem_block(mp, mm, ck);

	return true;
}

mem_pool_t *mem_pool_clear(mem_pool_t *mp)
{
	mem_space_t *mm;

	if (mp == NULL) {
		return NULL;
	}

	mm = mp->mlist;
	while (mm != NULL) {
		mem_space_init(mp, mm);
		mm = mm->next;
	}

	return mp;
}

bool mem_pool_destroy(mem_pool_t *mp)
{
	mem_space_t *mm;
	mem_space_t *mm_next;

	if (mp == NULL) {
		return false;
	}

	mm = mp->mlist;
	mm_next = NULL;
	while (mm != NULL) {
		if (mm->is_self_create) {
			free(mm->start);
		}
		mm_next = mm;
		mm = mm->next;
		free(mm_next);
	}

	free(mp);

	return true;
}

uint64_t get_mempool_usage(mem_pool_t *mp)
{
	m_size_t total_alloc = 0;
	mem_space_t *mm = mp->mlist;

	while (mm != NULL) {
		total_alloc += mm->alloc_mem;
		mm = mm->next;
	}

	return (uint64_t)total_alloc;
}

uint64_t get_mempool_prog_usage(mem_pool_t *mp)
{
	m_size_t total_alloc_prog = 0;
	mem_space_t *mm = mp->mlist;

	while (mm != NULL) {
		total_alloc_prog += mm->alloc_prog_mem;
		mm = mm->next;
	}

	return (uint64_t)total_alloc_prog;
}
