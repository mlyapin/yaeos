#include "arch_i686/exceptions.h"

#include "arch_i686/intr.h"

#include "kernel/kernel.h"
#include "kernel/panic.h"

#include "lib/cstd/stdio.h"

static char DESCRIPTION_BUFFER[64];
static int DESCBUFFER_POS = 0;

static size_t min(size_t x, size_t y)
{
        return (x < y ? x : y);
}

static int descbuffer_write(const char *msg, size_t len)
{
        char *restrict buffer = DESCRIPTION_BUFFER;
        size_t towrite = min(len, ARRAY_SIZE(DESCRIPTION_BUFFER) - DESCBUFFER_POS);
        for (int i = 0; i < towrite; i++) {
                buffer[DESCBUFFER_POS++] = msg[i];
        }
        return (towrite);
}

static struct kernel_panic_info prepare_panic_info(struct intr_ctx *ctx, const char *desc)
{
        kfprintf(descbuffer_write, "%s (INTNO: %d; ERR: %d)", desc, ctx->int_n, ctx->err_code);
        struct kernel_panic_info info = (struct kernel_panic_info){
                .description = DESCRIPTION_BUFFER,
                .location = NULL,
        };
        void *mem = __builtin_alloca(kvstore_predict_reqmem(PLATFORM_REGISTERS_COUNT));
        info.regs = kvstore_create(mem, PLATFORM_REGISTERS_COUNT,
                                   (int (*)(void const *, void const *))kstrcmp);

        kvstore_append(info.regs, "EDI", uint2ptr(ctx->edi));
        kvstore_append(info.regs, "ESI", uint2ptr(ctx->esi));
        kvstore_append(info.regs, "EBP", uint2ptr(ctx->ebp));
        kvstore_append(info.regs, "ESP", uint2ptr(ctx->esp));
        kvstore_append(info.regs, "EBX", uint2ptr(ctx->ebx));
        kvstore_append(info.regs, "EDX", uint2ptr(ctx->edx));
        kvstore_append(info.regs, "ECX", uint2ptr(ctx->ecx));
        kvstore_append(info.regs, "EAX", uint2ptr(ctx->eax));
        kvstore_append(info.regs, "EIP", uint2ptr(ctx->eip));
        kvstore_append(info.regs, "ESP", uint2ptr(ctx->preint_esp));

        return (info);
}

static const char *descriptions[22] = { [0x0] = "Divide error",
                                        [0x1] = "Debug exception",
                                        [0x2] = "NMI interrupt",
                                        [0x3] = "Breakpoint",
                                        [0x4] = "Overflow",
                                        [0x5] = "BOUND Range Exceeded",
                                        [0x6] = "Invalid Opcode",
                                        [0x7] = "No Math Coprocessor",
                                        [0x8] = "Double Fault",
                                        [0x9] = NULL,
                                        [0xA] = "Invalid TSS",
                                        [0xB] = "Segment Not Present",
                                        [0xC] = "Stack-Segment Fault",
                                        [0xD] = "General Protection",
                                        [0xE] = "Page Fault",
                                        [0xF] = NULL,
                                        [0x10] = "x87 FPU Floating-Point Error",
                                        [0x11] = "Alignment Check",
                                        [0x12] = "Machine Check",
                                        [0x13] = "SIMD Floating-Point Exception",
                                        [0x14] = "Virtualization Exception",
                                        [0x15] = "Control Protection Exception" };

static void default_handler(struct intr_ctx *ctx)
{
        const char *msg = "Unknown exception";
        if (ctx->int_n < 22) {
                msg = descriptions[ctx->int_n];
        }
        struct kernel_panic_info i = prepare_panic_info(ctx, msg);
        kernel_panic(&i);
}

void i686_setup_exception_handlers(void)
{
        intr_handler_cpu_default(default_handler);
}
