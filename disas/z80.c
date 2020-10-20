/*
 * QEMU Z80
 * Instruction printing code for the Z80 instruction set
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...Stuart Brady/William Towle, under GPL...]
 */

#include "qemu/osdep.h"
#include "disas/bfd.h"


int
print_insn_z80(bfd_vma memaddr, struct disassemble_info *info)
{
    fprintf_function fprintf_fn = info->fprintf_func;
    int status;
    unsigned char byte;
    void *stream = info->stream;

    status= info->read_memory_func(memaddr, &byte, sizeof byte, info);
    if (status != 0)
    {
        info->memory_error_func(status, memaddr, info);
        return -1;
    }

    fprintf_fn (stream, ".byte 0x%02x", byte);
    return 1;
}
