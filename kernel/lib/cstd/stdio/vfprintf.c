// UNITY_TEST DEPENDS ON: kernel/lib/string/strchr.c
// UNITY_TEST DEPENDS ON: kernel/lib/string/strlen.c

#include "lib/console.h"
#include "lib/cppdefs.h"
#include "lib/cstd/stdio.h"
#include "lib/cstd/string.h"

#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CF_MINUS (0x1 << 0)
#define CF_PLUS  (0x1 << 1)
#define CF_SPACE (0x1 << 2)
#define CF_HASH  (0x1 << 3)
#define CF_ZERO  (0x1 << 4)

enum conv_specifiers {
        CS_INT,
        CS_UOCTAL,
        CS_UDEC,
        CS_UHEX,
        CS_UHEX_BIG,
        CS_UCHAR,
        CS_STR,
        CS_PTR,
        CS_WRITTEN_OUT,
        CS_PERCENTAGE,
        CS_INVALID
};

enum conv_length {
        CL_CHAR,
        CL_SHORT,
        CL_LONG,
        CL_LLONG,
        CL_INTMAX,
        CL_SIZET,
        CL_PTRDIFF,
        CL_LDOUBLE,
        CL_NONE
};
#define WIDTH_EMPTY (-2)
#define WIDTH_VAR   (-1)
#define PREC_EMPTY  (-2)
#define PREC_VAR    (-1)
struct conv_spec {
        int flags;
        int width;
        int precision;
        enum conv_length length;
        enum conv_specifiers spec;
};

#ifdef __i686__
static unsigned conv_positive_atoi(const char *s, unsigned len)
{
        // The number cannot be negative.
        // The segment must contain only digits.
        unsigned res = 0;
        switch (len) {
        case 10: res += (s[len - 10] - '0') * 1000000000U;
        case 9: res += (s[len - 9] - '0') * 100000000U;
        case 8: res += (s[len - 8] - '0') * 10000000U;
        case 7: res += (s[len - 7] - '0') * 1000000U;
        case 6: res += (s[len - 6] - '0') * 100000U;
        case 5: res += (s[len - 5] - '0') * 10000U;
        case 4: res += (s[len - 4] - '0') * 1000U;
        case 3: res += (s[len - 3] - '0') * 100U;
        case 2: res += (s[len - 2] - '0') * 10U;
        case 1: res += (s[len - 1] - '0');
        }
        return (res);
}
#endif // __i686__

static struct conv_spec parse_conv_spec(const char *_format, int *spec_len)
{
        const char *format = _format;
        struct conv_spec result = { 0 };
        // The first symbol must be '%'.
        format++;

        // Flags
        while (*format != '\0') {
                bool done = false;
                switch (*format) {
                case '-': {
                        result.flags |= CF_MINUS;
                        format++;
                } break;
                case '+': {
                        result.flags |= CF_PLUS;
                        format++;
                } break;
                case ' ': {
                        result.flags |= CF_SPACE;
                        format++;
                } break;
                case '#': {
                        result.flags |= CF_HASH;
                        format++;
                } break;
                case '0': {
                        result.flags |= CF_ZERO;
                        format++;
                } break;
                default: done = true;
                }
                if (done) {
                        break;
                }
        }

        // Field width

        {
                if (*format == '*') {
                        result.width = WIDTH_VAR;
                        format++;
                        goto skip_width;
                }
                const char *begin = format;
                unsigned len = 0;
                while (begin[len] >= '0' && begin[len] <= '9') {
                        len++;
                }
                if (len == 0) {
                        // Width was not specified.
                        result.width = WIDTH_EMPTY;
                        goto skip_width;
                }

                result.width = (int)conv_positive_atoi(begin, len);
                format += len;

        skip_width:;
        }

        // Precision
        {
                if (*format != '.') {
                        result.precision = PREC_EMPTY;
                        goto skip_precision;
                } else {
                        format++;
                }

                if (*format == '*') {
                        result.precision = PREC_VAR;
                        format++;
                        goto skip_precision;
                }

                const char *begin = format;
                unsigned len = 0;
                while (begin[len] >= '0' && begin[len] <= '9') {
                        len++;
                }
                if (len == 0) {
                        // "If only the period specified, the precision is taken as zero."
                        result.precision = 0;
                        goto skip_precision;
                }
                result.precision = (int)conv_positive_atoi(begin, len);
                format += len;

        skip_precision:;
        }

        // Length modifier
        {
                switch (*format) {
                case 'h': {
                        if (format[1] == 'h') {
                                result.length = CL_CHAR;
                                format++;
                        } else {
                                result.length = CL_SHORT;
                        }
                } break;
                case 'l': {
                        if (format[1] == 'l') {
                                result.length = CL_LLONG;
                                format++;
                        } else {
                                result.length = CL_LONG;
                        }
                } break;
                case 'j': result.length = CL_INTMAX; break;
                case 'z': result.length = CL_SIZET; break;
                case 't': result.length = CL_PTRDIFF; break;
                case 'L': result.length = CL_LDOUBLE; break;
                default: result.length = CL_NONE;
                }

                if (result.length != CL_NONE) {
                        format++;
                }
        }

        // Conversion specifier
        {
                switch (*format) {
                case 'd':
                case 'i': result.spec = CS_INT; break;
                case 'o': result.spec = CS_UOCTAL; break;
                case 'u': result.spec = CS_UDEC; break;
                case 'x': result.spec = CS_UHEX; break;
                case 'X': result.spec = CS_UHEX_BIG; break;
                case 'c': result.spec = CS_UCHAR; break;
                case 's': result.spec = CS_STR; break;
                case 'p': result.spec = CS_PTR; break;
                case 'n': result.spec = CS_WRITTEN_OUT; break;
                case '%': result.spec = CS_PERCENTAGE; break;
                default: result.spec = CS_INVALID;
                }
                format++;
        }

        *spec_len = format - _format;
        return (result);
}

struct argument {
        union {
                uintmax_t d;
                char *str;
        } val;
        bool negative;
};

static struct argument fetch_arg(struct conv_spec s, va_list *args)
{
        struct argument arg = { 0 };
        switch (s.spec) {
        case CS_INT: {
                intmax_t val = va_arg(*args, intmax_t);
                switch (s.length) {
#define CASE_BODY(type)                      \
        do {                                 \
                type v = (type)val;          \
                arg.val.d = v;               \
                if (v < 0) {                 \
                        arg.negative = true; \
                        arg.val.d *= -1;     \
                }                            \
        } while (0)
                case CL_CHAR: CASE_BODY(char); break;
                case CL_SHORT: CASE_BODY(short); break;
                case CL_LONG: CASE_BODY(long); break;
                case CL_LLONG: CASE_BODY(long long); break;
                case CL_INTMAX: CASE_BODY(intmax_t); break;
                case CL_SIZET: CASE_BODY(size_t); break;
                case CL_NONE: CASE_BODY(int); break;
                case CL_PTRDIFF: CASE_BODY(ptrdiff_t); break;
#undef CASE_BODY
                }
        } break;
        case CS_UHEX:
        case CS_UHEX_BIG:
        case CS_UOCTAL:
        case CS_UDEC: {
                uintmax_t val = va_arg(*args, uintmax_t);
                switch (s.length) {
                case CL_CHAR: arg.val.d = (unsigned char)val; break;
                case CL_SHORT: arg.val.d = (unsigned short)val; break;
                case CL_LONG: arg.val.d = (unsigned long)val; break;
                case CL_LLONG: arg.val.d = (unsigned long long)val; break;
                case CL_INTMAX: arg.val.d = (uintmax_t)val; break;
                case CL_SIZET: arg.val.d = (size_t)val; break;
                case CL_NONE: arg.val.d = (unsigned)val; break;
                case CL_PTRDIFF: arg.val.d = (ptrdiff_t)val; break;
                }
        } break;
        case CS_PTR: arg.val.d = va_arg(*args, int); break;
        case CS_STR: arg.val.str = va_arg(*args, char *); break;
        case CS_UCHAR: arg.val.d = va_arg(*args, int); break;
        }
        return (arg);
}

struct conv_spec_funcs {
        // Print the format specifier.
        void (*print)(fprintf_fn f, struct conv_spec *s, struct argument *a);
        // Calculate the length of the result of print function.
        int (*length)(struct conv_spec *s, struct argument *a);
        // Function that returns string to prepend to the value ("0x" for ptr/hex values, for example)
        // and it's length. Could be NULL.
        const char *(*prefix)(struct conv_spec *s, struct argument *a, unsigned *length);
};

static int length_for_intnumbase(uintmax_t num, unsigned base, unsigned max_num_digits)
{
        for (int i = max_num_digits - 1; i >= 0; i--) {
                uintmax_t power = 1;
                for (int j = 0; j < i; j++) {
                        power *= base;
                }
                if (num >= power) {
                        return (i + 1);
                }
        }
        return (1);
}

static int oct_length(struct conv_spec *s, struct argument *a)
{
        // Special case:
        // The result of converting a zero value with a precision of zero is no characters.
        if (s->precision == 0 && a->val.d == 0) {
                return (0);
        }

        int length = length_for_intnumbase(a->val.d, 8, 22);

        if (s->precision != PREC_EMPTY) {
                if (s->precision >= length) {
                        return (s->precision);
                }
        }

        return (length);
}

static void oct_print(fprintf_fn f, struct conv_spec *s, struct argument *a)
{
        char buffer[22];
        int buffer_size = ARRAY_SIZE(buffer);
        int buffer_i = buffer_size;

        // Special case:
        // The result of converting a zero value with a precision of zero is no characters.
        if (s->precision == 0 && a->val.d == 0) {
                return;
        }

        do {
                buffer_i--;
                buffer[buffer_i] = a->val.d % 8 + '0';
        } while ((a->val.d /= 8) != 0 && buffer_i > 0);

        if (s->precision != PREC_EMPTY) {
                int already_printed = buffer_size - buffer_i;
                s->precision -= already_printed;
                for (int i = 0; i < s->precision; i++) {
                        f("0", 1);
                }
        }

        f(&buffer[buffer_i], buffer_size - buffer_i);
}

static int dec_length(struct conv_spec *s, struct argument *a)
{
        // Special case:
        // The result of converting a zero value with a precision of zero is no characters.
        if (s->precision == 0 && a->val.d == 0) {
                return (0);
        }

        int length = length_for_intnumbase(a->val.d, 10, 20);

        if (s->precision != PREC_EMPTY) {
                if (s->precision >= length) {
                        return (s->precision);
                }
        }

        return (length);
}

static void dec_print(fprintf_fn f, struct conv_spec *s, struct argument *a)
{
        char buffer[20];
        int buffer_size = ARRAY_SIZE(buffer);
        int buffer_i = buffer_size;

        // Special case:
        // The result of converting a zero value with a precision of zero is no characters.
        if (s->precision == 0 && a->val.d == 0) {
                return;
        }

        do {
                buffer_i--;
                buffer[buffer_i] = a->val.d % 10 + '0';
        } while ((a->val.d /= 10) != 0 && buffer_i > 0);

        if (s->precision != PREC_EMPTY) {
                int already_printed = buffer_size - buffer_i;
                s->precision -= already_printed;
                for (int i = 0; i < s->precision; i++) {
                        f("0", 1);
                }
        }

        f(&buffer[buffer_i], buffer_size - buffer_i);
}

static int hex_length(struct conv_spec *s, struct argument *a)
{
        if (s->spec == CS_UHEX || s->spec == CS_UHEX_BIG) {
                if (s->precision == 0 && a->val.d == 0) {
                        return (0);
                }
        }
        return (length_for_intnumbase(a->val.d, 16, 16));
}

static void hex_print(fprintf_fn f, struct conv_spec *s, struct argument *a)
{
        char buffer[16];
        int buffer_size = ARRAY_SIZE(buffer);
        int buffer_i = buffer_size;

        // A little bit hacky, but it makes no sense to write the same code for the second and third time.
        if (s->spec == CS_UHEX || s->spec == CS_UHEX_BIG) {
                if (s->precision == 0 && a->val.d == 0) {
                        return;
                }
        }

        char hex_offset;
        if (s->spec == CS_UHEX) {
                hex_offset = 'a';
        } else {
                // CS_UHEX_BIG or CS_PTR
                hex_offset = 'A';
        }

        do {
                buffer_i--;
                unsigned char n = a->val.d % 16;
                if (n < 10) {
                        buffer[buffer_i] = n + '0';
                } else {
                        buffer[buffer_i] = (n - 10) + hex_offset;
                }
        } while ((a->val.d /= 16) != 0 && buffer_i > 0);

        f(&buffer[buffer_i], buffer_size - buffer_i);
}

static const char *hex_prefix(struct conv_spec *s, struct argument *a, unsigned *len)
{
        static const char zx[] = "0x";
        // Do not count null-terminator.
        static const unsigned zx_len = sizeof(zx) - 1;

        if (s->spec == CS_PTR) {
                *len = zx_len;
                return (zx);
        }

        if (s->flags & (CF_HASH)) {
                *len = zx_len;
                return (zx);
        }

        *len = 0;
        return ("");
}

static int str_length(struct conv_spec *s, struct argument *a)
{
        int l = kstrlen(a->val.str);
        if (s->precision != PREC_EMPTY && s->precision < l) {
                return (s->precision);
        }
        return (l);
}

static void str_print(fprintf_fn f, struct conv_spec *s, struct argument *a)
{
        const char *str = a->val.str;
        // TODO: Find a way to reuse the result.
        int len = str_length(s, a);

        f(str, len);
}

static int char_length(struct conv_spec *s, struct argument *a)
{
        return (1);
}

static void char_print(fprintf_fn f, struct conv_spec *s, struct argument *a)
{
        f((char *)&a->val.d, 1);
}

static struct conv_spec_funcs cs_funcs_table[] = {
        [CS_INT] = (struct conv_spec_funcs){ .print = dec_print,
                                             .length = dec_length,
                                             .prefix = NULL },
        [CS_UDEC] = (struct conv_spec_funcs){ .print = dec_print,
                                              .length = dec_length,
                                              .prefix = NULL },
        [CS_PTR] = (struct conv_spec_funcs){ .print = hex_print,
                                             .length = hex_length,
                                             .prefix = hex_prefix },
        [CS_UHEX] = (struct conv_spec_funcs){ .print = hex_print,
                                              .length = hex_length,
                                              .prefix = hex_prefix },
        [CS_UHEX_BIG] = (struct conv_spec_funcs){ .print = hex_print,
                                                  .length = hex_length,
                                                  .prefix = hex_prefix },
        [CS_STR] = (struct conv_spec_funcs){ .print = str_print,
                                             .length = str_length,
                                             .prefix = NULL },
        [CS_UCHAR] = (struct conv_spec_funcs){ .print = char_print,
                                               .length = char_length,
                                               .prefix = NULL },
        [CS_UOCTAL] =
                (struct conv_spec_funcs){ .print = oct_print, .length = oct_length, .prefix = NULL }
};

/**
 * @brief Print visible characters, specified by flags.
 * 
 * @return int Number of written characters.
 */
static int put_flags(fprintf_fn f, struct conv_spec s, struct argument arg)
{
        int printed = 0;
        if (arg.negative) {
                f("-", 1);
                printed++;
        } else if (s.flags & CF_PLUS) {
                f("+", 1);
                printed++;
        } else if (s.flags & CF_SPACE) {
                f(" ", 1);
                printed++;
        }
        return (printed);
}

/**
 * @brief Print given conversion specifier field.
 *
 * @return int Number of written characters.
 */
static int print_conv_spec(fprintf_fn f, struct conv_spec s, va_list *args)
{
        if (s.spec == CS_PERCENTAGE) {
                return (f("%", 1));
        }

        struct argument arg = fetch_arg(s, args);
        int num_len = cs_funcs_table[s.spec].length(&s, &arg);
        int printed = 0;

        const char *prefix = NULL;
        unsigned prefix_len = 0;
        if (cs_funcs_table[s.spec].prefix) {
                prefix = cs_funcs_table[s.spec].prefix(&s, &arg, &prefix_len);
        }

        bool flag_present = arg.negative || (s.flags & (CF_PLUS | CF_SPACE));
        int width_to_fill = s.width - num_len - prefix_len - (flag_present ? 1 : 0);

        // If Width != 0 and the flag '-' was not specified, result must be right-justified
        if (!(s.flags & CF_MINUS) && s.width != WIDTH_EMPTY) {
                if (s.flags & CF_ZERO) {
                        if (prefix != NULL) {
                                f(prefix, prefix_len);
                                printed += prefix_len;
                        }
                        printed += put_flags(f, s, arg);
                }
                while (width_to_fill > 0) {
                        // remained_width could be < 0, so decrement in the loop
                        char c = s.flags & CF_ZERO ? '0' : ' ';
                        f(&c, 1);
                        printed++;
                        width_to_fill--;
                }
        }
        if (!(s.flags & CF_ZERO)) {
                if (prefix != NULL) {
                        f(prefix, prefix_len);
                        printed += prefix_len;
                }
                printed += put_flags(f, s, arg);
        }
        cs_funcs_table[s.spec].print(f, &s, &arg);
        printed += num_len;

        // If Width != 0 and the flag '-' was specified, result must be left-justified
        if (s.flags & CF_MINUS && s.width != WIDTH_EMPTY) {
                while (width_to_fill > 0) {
                        // remained_width could be < 0, so decrement in the loop
                        f(" ", 1);
                        printed++;
                        width_to_fill--;
                }
        }
        return (printed);
}

int kvfprintf(fprintf_fn f, const char *restrict format, va_list args)
{
        va_list ap;
        va_copy(ap, args);
        int printed = 0;

        while (*format != '\0') {
                if (*format == '%') {
                        int spec_len = 0;
                        struct conv_spec s = parse_conv_spec(format, &spec_len);
                        printed += print_conv_spec(f, s, &ap);
                        format += spec_len;
                        continue;
                }

                const char *cspec = kstrchr(format, '%');
                size_t toprint;
                if (cspec) {
                        toprint = cspec - format;
                } else {
                        toprint = kstrlen(format);
                }

                f(format, toprint);
                printed += toprint;
                format += toprint;
        }
        va_end(ap);
        return (printed);
}

int kfprintf(fprintf_fn f, const char *restrict format, ...)
{
        va_list args;
        va_start(args, format);
        int i = kvfprintf(f, format, args);
        va_end(args);
        return (i);
}
