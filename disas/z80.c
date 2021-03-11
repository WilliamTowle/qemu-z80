/*
 * QEMU Z80
 * Instruction printing code for the Z80 instruction set
 * vim: ft=c sw=4 ts=4 et :
 *
 * [...Stuart Brady/William Towle, under GPL...]
 */

#include "qemu/osdep.h"
#include "disas/bfd.h"


struct buffer
{
    bfd_vma base;
    int n_fetch;
    int n_used;
    signed char data[4];
};


typedef int (*prt_func)(struct buffer *, disassemble_info *, const char *);

struct tab_elt
{
    unsigned char val;
    unsigned char mask;
    prt_func          fp;
    const char *  text;
};


static
int fetch_data(struct buffer *buf, disassemble_info * info, int n)
{
    int r;

    if (buf->n_fetch + n > 4)
          abort();

    r= info->read_memory_func(buf->base + buf->n_fetch,
                            (unsigned char*) buf->data + buf->n_fetch,
                            n, info);

    if (r == 0)
        buf->n_fetch+= n;
    return !r;
}

#if 0   /* FIXME: unreferenced due to dump() use */
static
int prt(struct buffer *buf, disassemble_info *info, const char *txt)
{
    info->fprintf_func(info->stream, "%s", txt);
    buf->n_used = buf->n_fetch;
    return 1;
}
#else
static
int dump (struct buffer *buf, disassemble_info * info, const char *txt)
{
    int i;

    info->fprintf_func (info->stream, "defb ");
    for (i = 0; txt[i]; ++i)
        info->fprintf_func (info->stream, i ? ", 0x%02x" : "0x%02x",
                          (unsigned char) buf->data[i]);
    buf->n_used = i;
    return buf->n_used;
}
#endif

/* Table to disassemble machine codes without prefix.  */
static struct tab_elt opc_main[] =
{
#if 0
//    { 0x00, 0xFF, prt, "nop" },
//    { 0x01, 0xCF, prt_rr_nn, "ld %s,0x%%04x" },
//    { 0x02, 0xFF, prt, "ld (bc),a" },
//    { 0x03, 0xCF, prt_rr, "inc " },
//    { 0x04, 0xC7, prt_r, "inc %s" },
//    { 0x05, 0xC7, prt_r, "dec %s" },
//    { 0x06, 0xC7, ld_r_n, "ld %s,0x%%02x" },
//    { 0x07, 0xFF, prt, "rlca" },
//    { 0x08, 0xFF, prt, "ex af,af'" },
//    { 0x09, 0xCF, prt_rr, "add hl," },
//    { 0x0A, 0xFF, prt, "ld a,(bc)" },
//    { 0x0B, 0xCF, prt_rr, "dec " },
//    { 0x0F, 0xFF, prt, "rrca" },
//    { 0x10, 0xFF, prt_e, "djnz " },
//    { 0x12, 0xFF, prt, "ld (de),a" },
//    { 0x17, 0xFF, prt, "rla" },
//    { 0x18, 0xFF, prt_e, "jr "},
//    { 0x1A, 0xFF, prt, "ld a,(de)" },
//    { 0x1F, 0xFF, prt, "rra" },
//    { 0x20, 0xE7, jr_cc, "jr %s,"},
//    { 0x22, 0xFF, prt_nn, "ld (0x%04x),hl" },
//    { 0x27, 0xFF, prt, "daa"},
//    { 0x2A, 0xFF, prt_nn, "ld hl,(0x%04x)" },
//    { 0x2F, 0xFF, prt, "cpl" },
//    { 0x32, 0xFF, prt_nn, "ld (0x%04x),a" },
//    { 0x37, 0xFF, prt, "scf" },
//    { 0x3A, 0xFF, prt_nn, "ld a,(0x%04x)" },
//    { 0x3F, 0xFF, prt, "ccf" },
//
//    { 0x76, 0xFF, prt, "halt" },
//    { 0x40, 0xC0, ld_r_r, "ld %s,%s"},
//
//    { 0x80, 0xC0, arit_r, "%s%s" },
//
//    { 0xC0, 0xC7, prt_cc, "ret " },
//    { 0xC1, 0xCF, pop_rr, "pop" },
//    { 0xC2, 0xC7, jp_cc_nn, "jp " },
//    { 0xC3, 0xFF, prt_nn, "jp 0x%04x" },
//    { 0xC4, 0xC7, jp_cc_nn, "call " },
//    { 0xC5, 0xCF, pop_rr, "push" },
//    { 0xC6, 0xC7, arit_n, "%s0x%%02x" },
//    { 0xC7, 0xC7, rst, "rst 0x%02x" },
//    { 0xC9, 0xFF, prt, "ret" },
//    { 0xCB, 0xFF, pref_cb, "" },
//    { 0xCD, 0xFF, prt_nn, "call 0x%04x" },
//    { 0xD3, 0xFF, prt_n, "out (0x%02x),a" },
//    { 0xD9, 0xFF, prt, "exx" },
//    { 0xDB, 0xFF, prt_n, "in a,(0x%02x)" },
//    { 0xDD, 0xFF, pref_ind, "ix" },
//    { 0xE3, 0xFF, prt, "ex (sp),hl" },
//    { 0xE9, 0xFF, prt, "jp (hl)" },
//    { 0xEB, 0xFF, prt, "ex de,hl" },
//    { 0xED, 0xFF, pref_ed, ""},
//    { 0xF3, 0xFF, prt, "di" },
//    { 0xF9, 0xFF, prt, "ld sp,hl" },
//    { 0xFB, 0xFF, prt, "ei" },
//    { 0xFD, 0xFF, pref_ind, "iy" },
    { 0x00, 0x00, prt, "????" },    /* normally ends list */
#else   /* use dump() until table is implemented */
    { 0x00, 0x00, dump, "x" },
#endif
};

int
print_insn_z80(bfd_vma memaddr, disassemble_info *info)
{
    struct buffer buf;
    struct tab_elt *p;

    buf.base= memaddr;
    buf.n_fetch= 0;
    buf.n_used= 0;

    if (!fetch_data(&buf, info, 1))
        return -1;

    for (p= opc_main; p->val != (buf.data[0] & p->mask); ++p)
        ;
    p->fp(&buf, info, p->text);

    return buf.n_used;
}
