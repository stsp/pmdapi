#ifndef __ARCH_DESC_H
#define __ARCH_DESC_H

#define LDT_entry_a(info) \
	((((info)->base_addr & 0x0000ffff) << 16) | ((info)->limit & 0x0ffff))

#define LDT_entry_b(info) \
	(((info)->base_addr & 0xff000000) | \
	(((info)->base_addr & 0x00ff0000) >> 16) | \
	((info)->limit & 0xf0000) | \
	(((info)->read_exec_only ^ 1) << 9) | \
	((info)->contents << 10) | \
	(((info)->seg_not_present ^ 1) << 15) | \
	((info)->seg_32bit << 22) | \
	((info)->limit_in_pages << 23) | \
	((info)->useable << 20) | \
	0x7000)

static __inline__ int
_my_fs(void)
{
  unsigned short result;
  __asm__("movw %%fs,%0" : "=r" (result));
  return result;
}
static __inline__ int
_my_gs(void)
{
  unsigned short result;
  __asm__("movw %%gs,%0" : "=r" (result));
  return result;
}

#endif
