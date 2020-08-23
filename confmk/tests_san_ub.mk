CFLAGS += -fsanitize=undefined -fno-omit-frame-pointer
LDFLAGS += -fsanitize=undefined
CPPFLAGS += -D__i686__ -DUNITY_INCLUDE_PRINT_FORMATTED
# Silence unity output
CPPFLAGS += -DUNITY_OUTPUT_CHAR=""

CC := clang
LD := clang
AS := gcc -xassembler-with-cpp -c
AR := ar

ARCH := i686
