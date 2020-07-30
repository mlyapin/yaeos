#ifndef _KERNEL_KLOG_H
#define _KERNEL_KLOG_H

#include <kernel/console.h>
#include <kernel/cppdefs.h>
#include <stdarg.h>

#define LOGF(level, message, ...)                                                                  \
	klog_logf_at((level), __FILE__, __func__, TO_SSTR_MACRO(__LINE__), (message), ##__VA_ARGS__)

#define LOGF_D(message, ...) LOGF(LOG_DEBUG, (message), ##__VA_ARGS__)
#define LOGF_I(message, ...) LOGF(LOG_INFO, (message), ##__VA_ARGS__)
#define LOGF_W(message, ...) LOGF(LOG_WARN, (message), ##__VA_ARGS__)
#define LOGF_E(message, ...) LOGF(LOG_ERR, (message), ##__VA_ARGS__)
#define LOGF_P(message, ...) LOGF(LOG_PANIC, (message), ##__VA_ARGS__)

enum LOG_LEVEL {
	LOG_DEBUG = 0x0,
	LOG_INFO = 0x1,
	LOG_WARN = 0x2,
	LOG_ERR = 0x3,
	LOG_PANIC = 0x4,
};

void klog_logf_at(enum LOG_LEVEL lvl, const char *path, const char *func,
		  const char *line, const char *ormat, ...) __attribute__((format(printf, 5, 6)));

#endif // _KERNEL_KLOG_H
