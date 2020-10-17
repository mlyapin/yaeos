CFLAGS_COMMON += -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls -fsanitize-memory-track-origins
LDFLAGS_COMMON += -fsanitize=address
CPPFLAGS_COMMON += -D__i686__ -DUNITY_INCLUDE_PRINT_FORMATTED -Wno-macro-redefined
# Silence unity output
CPPFLAGS_COMMON += -DUNITY_OUTPUT_CHAR=""

CC := clang
HOST_CC := $(CC)
LD := clang
AS := gcc -xassembler-with-cpp -c
AR := ar

ARCH := i686
