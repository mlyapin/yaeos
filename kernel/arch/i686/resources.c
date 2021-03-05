#include "arch_i686/resources.h"

#include "arch_i686/kernel.h"

#include "kernel/kernel.h"
#include "kernel/klog.h"
#include "kernel/mm/addr.h"
#include "kernel/platform_consts.h"
#include "kernel/resources.h"

#include "lib/align.h"
#include "lib/cppdefs.h"
#include "lib/cstd/nonstd.h"

#include <multiboot.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static uintptr_t max_addr(void)
{
        unsigned width = sizeof(void *) * 8;
        uintptr_t mask = 0;
        for (unsigned i = 0; i < width; i++) {
                mask |= (uintptr_t)(0x1) << i;
        }
        return ((uintptr_t)(-0x1) & mask);
}

typedef void (*iter_available_fn_t)(uintptr_t start, uintptr_t end, uint32_t type);

static void iter_without_unavail(struct multiboot_mmap_entry *mmap, iter_available_fn_t fn)
{
        /* Skip unavailable memory. */
        if (mmap->type == MULTIBOOT_MEMORY_BADRAM || mmap->type == MULTIBOOT_MEMORY_RESERVED) {
                return;
        }

        /* ... if the chunk is entirely out of our reach, ignore it. */
        if (mmap->addr > max_addr()) {
                return;
        }

        uintptr_t kstart = (uintptr_t)addr_to_low(kernel_start);
        uintptr_t kend = (uintptr_t)addr_to_low(kernel_end);
        uintptr_t memstart = mmap->addr;
        uintptr_t memend = memstart + mmap->len;

        /*
         * There are following cases:
         * 1. the kernel before the region => Do Nothing
         * 2. the kernel after the region => Do Nothing
         * 3. the kernel overlap with the begginning of the region => Update: memstart = kend
         * 4. the kernel overlap with the end of the region => Update: memend = kstart
         * 5. the region fully in the kernel => Drop the region altogether
         * 6. the kernel fully in the region => Split the region:
         *     memstart_1 = memstart;
         *     memend_1 = kstart
         *     memstart_2 = kend
         *     memend_2 = memend
         */

        /*
         * Keep in mind that `kstart` points at the actual beginning of the kernel's data.
         * But `kend` actually points at linker's location counter, so we are free to use this byte.
         * This is not really important then to try to set correct boundaries, because
         * in the worst-case scenario only multiboot header would be damaged, and it's no use to us.
         * So the boundaries checks there may be a bit incorrect (1 byte) and this is fine.
         */

        /* Case 1. */
        if (kend <= memstart) {
                fn(memstart, memend, mmap->type);
                return;
        }

        /* Case 2. */
        if (kstart >= memend) {
                fn(memstart, memend, mmap->type);
                return;
        }

        /* Case 3. */
        if (kend > memstart && kstart <= memstart) {
                fn(kend, memend, mmap->type);
                return;
        }

        /* Case 4. */
        if (kstart < memend && kend >= memend) {
                fn(memend, kstart, mmap->type);
                return;
        }

        /* Case 5. */
        if (kstart <= memstart && kend >= memend) {
                /* Ignore the region. */
                return;
        }

        /* Case 6. */
        if (kstart >= memstart && kend <= memend) {
                fn(memstart, kstart, mmap->type);
                fn(kend, memend, mmap->type);
                return;
        }

        /* Unhandled case */
        kassert(false);
}

static void iter_available_regions(iter_available_fn_t fn)
{
        multiboot_info_t *info = I686_INFO.multiboot;
        struct multiboot_mmap_entry *c = (void *)info->mmap_addr;

        /* mmap_* variables are not valid. There is nothing we can do. */
        if (!(info->flags & MULTIBOOT_INFO_MEM_MAP)) {
                return;
        }

        while ((uintptr_t)c < info->mmap_addr + info->mmap_length) {
                iter_without_unavail(c, fn);
                c = (void *)((uintptr_t)c + c->size + sizeof(c->size));
        }
}

static void register_mem_region(uintptr_t start, uintptr_t end, uint32_t type __unused)
{
        const size_t length = end - start;

        if (length <= PLATFORM_PAGE_SIZE) {
                return;
        }

        struct resource r = {
                .device_id = "platform",
                .type = RESOURCE_TYPE_MEMORY,
                .data.mem_reg.base = (void *)start,
                /* ... if we can address some part of the chunk, cut remainders out. */
                .data.mem_reg.len = MIN(length, max_addr() - start),
        };
        resources_register_res(r);
}

static void register_bios_vga(void)
{
        struct resource r = {
                .device_id = "video",
                .type = RESOURCE_TYPE_DEV_BUFFER,
        };
        resources_register_res(r);
}

void i686_register_resources(void)
{
        iter_available_regions(register_mem_region);
        register_bios_vga();
}
