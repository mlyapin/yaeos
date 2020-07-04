#ifndef CPPDEFS_H
#define CPPDEFS_H

#define __unused		__attribute__((__unused__))
#define __used			__attribute__((__used__))
#define __dead_code		__attribute__((__noreturn__))
#define __packed		__attribute__((__packed__))
#define __section(target)	__attribute__((__section__(target)))
#define __aligned(b)		__attribute__((__aligned__(b)))

#endif /* CPPDEFS_H */