#include <kernel/tty.h>

#include <arch/vga.h>
#include <arch/vm.h>
#include <arch/platform.h>
#include <arch/descriptors.h>

#include <stddef.h>
#include <stdbool.h>
#include <string.h>

// TODO: Maybe add some "smart" recursive macrosses to patch the stack automatically?
// TODOO: Make sure my home address is hidden on GitHub after that. Just in case...
#define PATCH_FRAME(level, offset)                                             \
	do {                                                                   \
		uint32_t *frame = __builtin_frame_address(level);              \
		/* Patch return address */                                     \
		uint32_t *ra = &frame[1];                                     \
		*ra += offset;                                                 \
		/* Patch previous ebp */                                       \
		uint32_t *bp = &frame[0];                                      \
		*bp += offset;                                                 \
	} while (false)

static void map_addr_range(void *page_dir, const void *start, const void *end,
			   enum VM_TABLE_FLAGS flags)
{
	uintptr_t paddr = (uintptr_t)start;
	while (paddr < (uintptr_t)end) {
		uintptr_t vaddr = KERNEL_VMA + paddr;
		vm_map(page_dir, (void *)vaddr, (void *)paddr, flags);
		paddr += PLATFORM_PAGE_SIZE;
	}
}

static void map_kernel(void *page_dir)
{
	map_addr_range(page_dir, (void *)LOW(&kernel_text_start[0]),
		       (void *)LOW(&kernel_text_end[0]), VM_TABLE_FLAG_PRESENT);
	map_addr_range(page_dir, (void *)LOW(&kernel_rodata_start[0]),
		       (void *)LOW(&kernel_rodata_end[0]),
		       VM_TABLE_FLAG_PRESENT);
	map_addr_range(page_dir, (void *)LOW(&kernel_data_start[0]),
		       (void *)LOW(&kernel_data_end[0]),
		       VM_TABLE_FLAG_RW | VM_TABLE_FLAG_PRESENT);
	map_addr_range(page_dir, (void *)LOW(&kernel_bss_start[0]),
		       (void *)LOW(&kernel_bss_end[0]),
		       VM_TABLE_FLAG_PRESENT | VM_TABLE_FLAG_RW);
}

static void map_platform(void *page_dir)
{
	void *paddr = (void *)VGA_BUFFER_ADDR;
	void *vaddr = (void *)HIGH(VGA_BUFFER_ADDR);
	vm_map(page_dir, vaddr, paddr,
	       VM_TABLE_FLAG_PRESENT | VM_TABLE_FLAG_RW);
}

static void setup_boot_paging(void)
{
	void *boot_pt = (void *)LOW(&boot_paging_pt[0]);
	void *boot_pd = (void *)LOW(&boot_paging_pd[0]);

	// Identity mapping
	void *identity = vm_dir_entry_addr(boot_pd, 0x0);
	vm_set_dir_entry(identity, boot_pt,
			 VM_DIR_FLAG_PRESENT | VM_DIR_FLAG_RW);

	// Map the kernel to higher half of address space
	void *hh = vm_dir_entry_addr(boot_pd, (void *)KERNEL_VMA);
	vm_set_dir_entry(hh, boot_pt, VM_DIR_FLAG_PRESENT | VM_DIR_FLAG_RW);

	map_kernel(boot_pd);
	map_platform(boot_pd);

	vm_paging_set(boot_pd);
	vm_paging_enable(KERNEL_VMA);

	// It is important to do it in reverse order!
	// Patch i686_init(...)
	PATCH_FRAME(1, KERNEL_VMA);
	// Patch setup_boot_paging(...)
	PATCH_FRAME(0, KERNEL_VMA);

	// Undo identity mapping
	vm_set_dir_entry(boot_pd, 0x0, 0x0);
	vm_tlb_flush();
}

void i686_init(void)
{
	setup_boot_paging();
	boot_setup_gdt();
	boot_setup_idt();

	tty_init();
	tty_writestring("The platform dependent init has finished\n");
}
