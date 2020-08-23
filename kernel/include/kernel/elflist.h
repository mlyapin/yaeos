#ifndef _KERNEL_ELFLIST_H
#define _KERNEL_ELFLIST_H

#include "kernel/cppdefs.h"

/**
 * Store pointers to custom information inside the kernel's binary.
 */

/**
  * Dummy structure used to mark edges of a list.
  * Note that in C, empty structure results in UB.
  * Also, in C++ sizeof(struct{}) == 1. Sure hope that I won't use C++ in the kernel.
  * Although GCC allows it as an extension.
  * See: https://gcc.gnu.org/onlinedocs/gcc/Empty-Structures.html#Empty-Structures
  */
struct elflist_mark {};

/**
 * The macro places the address of a symbol in the list's section and ensures that there are edge marks.
 * Note that it's important how sections are named, as they are sorted by a linker.
 */
#define ELFLIST_NEWDATA(listname, symbol)                                                          \
	const __weak struct elflist_mark __kernel_elflist_##listname##_begin __section(            \
		".elflist_" #listname "_begin");                                                   \
	const void *__kernel_elflist_##listname##_##symbol __section(".elflist_" #listname "_dat"  \
											   "a") =  \
		&(symbol);                                                                         \
	const __weak struct elflist_mark __kernel_elflist_##listname##_end __section(".elflist"    \
										     "_" #listname \
										     "_end")

#define ELFLIST_EXTERN(ptype, listname)                                                           \
	extern ptype *__kernel_elflist_##listname##_begin; /*NOLINT(bugprone-macro-parentheses)*/ \
	extern ptype *__kernel_elflist_##listname##_end    /*NOLINT(bugprone-macro-parentheses)*/

#define ELFLIST_BEGIN(listname) (&__kernel_elflist_##listname##_begin)
#define ELFLIST_END(listname)   (&__kernel_elflist_##listname##_end)
#define ELFLIST_COUNT(listname) (ELFLIST_END(listname) - ELFLIST_BEGIN(listname))
#define ELFLIST_FOREACH(listname, iterv) \
	for ((iterv) = ELFLIST_BEGIN(listname); (iterv) < ELFLIST_END(listname); (iterv)++)

#endif // _KERNEL_ELFLIST_H
