#ifndef _KERNEL_MM_VM_H
#define _KERNEL_MM_VM_H

#include "kernel/mm/vm_area.h"
#include "kernel/mm/vm_space.h"

#include <stdbool.h>
#include <stddef.h>

typedef void *phys_addr_t;
typedef void *virt_addr_t;

void vm_pgfault_handle_default(struct vm_area *area, virt_addr_t addr);
/* TODO: Rename to "highmem" */
void vm_pgfault_handle_direct(struct vm_area *area, virt_addr_t addr);

void vm_arch_iter_reserved_vaddresses(void (*fn)(void const *addr, size_t len, void *data),
                                      void *data);

bool vm_arch_is_range_valid(void const *base, size_t len);

void *vm_arch_get_early_pgroot(void);

void *vm_arch_get_phys_page(void const *virt_page);

void vm_arch_pt_map(void *tree_root, const void *phys_addr, const void *at_virt_addr,
                    enum vm_flags flags);

#endif /* _KERNEL_MM_VM_H */
