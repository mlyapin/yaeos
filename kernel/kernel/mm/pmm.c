#include "kernel/mm/pmm.h"

#include "kernel/cppdefs.h"
#include "kernel/ds/slist.h"
#include "kernel/klog.h"
#include "kernel/mm/kmm.h"

#include "lib/assert.h"

#include <stdbool.h>

SLIST_HEAD(zone_allocators, struct pmm_allocator);

static struct {
	struct zone_allocators normal;
	struct zone_allocators dma;
} ZONES;

static struct {
	struct kmm_cache *allocators;
	struct kmm_cache *pages;
} CACHES;

static void copy_alloc(struct pmm_allocator *src, struct pmm_allocator *restrict dest)
{
	assert(src);
	assert(dest);
	assert(src != dest);

	dest->name = src->name;
	dest->restrict_flags = src->restrict_flags;
	dest->page_alloc = src->page_alloc;
	dest->page_free = src->page_free;
}

void pmm_init(struct pmm_allocator *allocators, size_t alloc_length)
{
	assert(allocators);

	CACHES.allocators = kmm_cache_create("pmm_allocators_cache", sizeof(struct pmm_allocator),
					     0, 0, NULL, NULL);
	CACHES.pages =
		kmm_cache_create("pmm_pages_cache", sizeof(struct pmm_page), 0, 0, NULL, NULL);
	assert(CACHES.allocators);
	assert(CACHES.pages);

	for (int i = 0; i < alloc_length; i++) {
		struct pmm_allocator *a = kmm_cache_alloc(CACHES.allocators, 0);
		copy_alloc(&allocators[i], a);
		if (a->restrict_flags & PMM_RESTRICT_DMA) {
			SLIST_INSERT_HEAD(&ZONES.dma, a, allocators);
		} else {
			SLIST_INSERT_HEAD(&ZONES.normal, a, allocators);
		}
	}
}

static struct pmm_page *try_allocate_from_zone(struct zone_allocators zlist)
{
	struct pmm_alloc_resutl allocres;

	struct pmm_allocator *allocator;
	SLIST_FOREACH (allocator, &zlist, allocators) {
		if (__unlikely(allocator->page_alloc == NULL)) {
			continue;
		}

		allocres = allocator->page_alloc(allocator);
		if (allocres.success) {
			break;
		}
	}
	if (!allocres.success) {
		return (NULL);
	}

	struct pmm_page *page = kmm_cache_alloc(CACHES.pages, 0);
	page->paddr = allocres.paddr;
	page->alloc = allocator;

	return (page);
}

struct pmm_page *pmm_alloc_page(int flags)
{
	// TODO: Multiple pages allocation.
	bool requested_dma = flags & PMM_FLAG_DMA;

	if (!requested_dma) {
		struct pmm_page *p = try_allocate_from_zone(ZONES.normal);
		if (__likely(p != NULL)) {
			return (p);
		}
		LOGF_W("Out of zone::normal physical memory. Trying DMA.");
	}

	struct pmm_page *p = try_allocate_from_zone(ZONES.dma);
	return (p);
}

void pmm_free(struct pmm_page *p)
{
	assert(p);
	assert(p->alloc);

	if (__likely(p->alloc->page_free != NULL)) {
		p->alloc->page_free(p->alloc->data, p->paddr);
	}
}
