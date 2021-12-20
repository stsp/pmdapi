#ifndef WRAPPER_H
#define WRAPPER_H

#include <dpmi.h>
#include <stdint.h>
#include <stddef.h>
#include "sigcontext.h"
#include "cpu.h"
#include "ldt.h"

typedef unsigned int u_int;
typedef unsigned short us;
typedef unsigned char u_char;
typedef uint32_t dosaddr_t;

#define pushw(base, ptr, val) \
	do { \
		ptr = (Bit16u)(ptr - 1); \
		WRITE_BYTE((base) + ptr, (val) >> 8); \
		ptr = (Bit16u)(ptr - 1); \
		WRITE_BYTE((base) + ptr, val); \
	} while(0)

#define popb(base, ptr) \
	({ \
		Bit8u __res = READ_BYTE((base) + ptr); \
		ptr = (Bit16u)(ptr + 1); \
		__res; \
	})

#define popw(base, ptr) \
	({ \
		Bit8u __res0, __res1; \
		__res0 = READ_BYTE((base) + ptr); \
		ptr = (Bit16u)(ptr + 1); \
		__res1 = READ_BYTE((base) + ptr); \
		ptr = (Bit16u)(ptr + 1); \
		(__res1 << 8) | __res0; \
	})

#define D_printf(...)
#define g_printf(...)
#define x_printf(...)
#define error(...)
#define dosemu_error(...)
#define debug_level(...) 0
#define snprintf(a,b,c,d) sprintf(a,c,d)
#define MEMCPY_2DOS(dos_addr, unix_addr, n) \
	memcpy(LINEAR2UNIX(dos_addr), (unix_addr), (n))
#define MEMSET_DOS(dos_addr, val, n) \
        memset(LINEAR2UNIX(dos_addr), (val), (n))
#define MEMCPY_2UNIX(unix_addr, dos_addr, n) \
	memcpy((unix_addr), LINEAR2UNIX(dos_addr), (n))
#define memcpy_dos2dos(dest, src, n) MEMCPY_DOS2DOS(dest, src, n)
static inline void *LINEAR2UNIX(unsigned int addr)
{
	return (void*)addr;
}

#define min(x,y) ({ \
	typeof(x) _x = (x);	\
	typeof(y) _y = (y);	\
	_x < _y ? _x : _y; })

extern unsigned char *mem_base;
#define MK_FP32(s,o)		((void *)&mem_base[SEGOFF2LINEAR(s,o)])
#define LINP(a) ((unsigned char *)0 + (a))
static inline unsigned char *MEM_BASE32(dosaddr_t a)
{
    uint32_t off = (uint32_t)(ptrdiff_t)(mem_base + a);
    return LINP(off);
}
static inline dosaddr_t DOSADDR_REL(const unsigned char *a)
{
    return (a - mem_base);
}

u_short dos_get_psp(void);

#define TF_MASK		0x00000100
#define IF_MASK		0x00000200
#define IOPL_MASK	0x00003000
#define NT_MASK		0x00004000
#define VM_MASK		0x00020000
#define AC_MASK		0x00040000
#define VIF_MASK	0x00080000	/* virtual interrupt flag */
#define VIP_MASK	0x00100000	/* virtual interrupt pending */
#define ID_MASK		0x00200000

void wrapper_init(void);

#define coopth_leave()
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
void *dosaddr_to_unixaddr(dosaddr_t addr);

#endif
