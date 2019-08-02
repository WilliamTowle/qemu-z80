#ifndef QEMU_TYPES_H
#define QEMU_TYPES_H

#include <inttypes.h>

//#include "cpu.h"

#include <stdint.h>

typedef uint32_t abi_ulong;
typedef int32_t abi_long;
#define TARGET_ABI_FMT_lx "%08x"
#define TARGET_ABI_FMT_ld "%d"
#define TARGET_ABI_FMT_lu "%u"
#define TARGET_ABI_BITS 32

#endif
