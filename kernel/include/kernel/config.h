#ifndef _KERNEL_CONFIG_H
#define _KERNEL_CONFIG_H

#ifndef __ASSEMBLER__
#include "kernel/platform_consts.h"
#endif /* __ASSEMBLER__ */

#define CONF_STACK_SIZE         (16 << 10)
#define CONF_TIMER_QUEUE_LENGTH (100)
#define CONF_STATIC_SLAB_SPACE  (16384)
#define CONF_MALLOC_MIN_POW     (5)
#define CONF_MALLOC_MAX_POW     (11)

#define CONF_HEAP_MAX_CHUNK_SIZE ((size_t)32 * 1024 * 1024)
#define CONF_DEV_MAX_AREA_SIZE   ((size_t)32 * 1024 * 1024)

#define CONF_VM_RECURSIVE_PAGE (PLATFORM_PAGEDIR_PAGES - 1 - 1)
#define CONF_VM_ERRORS_PAGE    (PLATFORM_PAGEDIR_PAGES - 1)
#define CONF_VM_AVAILABLE_PAGES (1022)

#endif /* _KERNEL_CONFIG_H */
