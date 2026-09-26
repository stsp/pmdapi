/*
 * dosemu2's view of the DOS address space, for a DPMI client: a linear
 * address is reached through the near pointer our DS covers (djgpp's
 * __djgpp_nearptr_enable), so it is the address minus our own DS base.
 */
#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <string.h>

typedef uint32_t dosaddr_t;

#ifndef PAGE_SIZE
#define PAGE_SIZE	4096
#endif
#define _PAGE_MASK	(~(PAGE_SIZE-1))
#ifndef PAGE_MASK
#define PAGE_MASK	_PAGE_MASK
#endif
#define PAGE_ALIGN(addr)	(((addr)+PAGE_SIZE-1)&_PAGE_MASK)
#define HOST_PAGE_SIZE		PAGE_SIZE
#define HOST_PAGE_MASK		_PAGE_MASK
#define HOST_PAGE_ALIGN(addr)	PAGE_ALIGN(addr)

#define LOWMEM_SIZE 0x100000
#define HMASIZE (64*1024)

extern unsigned char *mem_base;

static inline void *dosaddr_to_unixaddr(dosaddr_t addr)
{
    return mem_base + addr;
}
#define LINEAR2UNIX(addr) dosaddr_to_unixaddr(addr)
static inline unsigned char *MEM_BASE32(dosaddr_t a)
{
    return mem_base + a;
}
static inline dosaddr_t DOSADDR_REL(const unsigned char *a)
{
    return (a - mem_base);
}

#define UNIX_READ_BYTE(addr)		(*(Bit8u *) (addr))
#define UNIX_WRITE_BYTE(addr, val)	(*(Bit8u *) (addr) = (val) )
#define UNIX_READ_WORD(addr)		(*(Bit16u *) (addr))
#define UNIX_WRITE_WORD(addr, val)	(*(Bit16u *) (addr) = (val) )
#define UNIX_READ_DWORD(addr)		(*(Bit32u *) (addr))
#define UNIX_WRITE_DWORD(addr, val)	(*(Bit32u *) (addr) = (val) )
#define READ_BYTE(addr)		UNIX_READ_BYTE(LINEAR2UNIX(addr))
#define WRITE_BYTE(addr, val)	UNIX_WRITE_BYTE(LINEAR2UNIX(addr), val)
#define READ_WORD(addr)		UNIX_READ_WORD(LINEAR2UNIX(addr))
#define WRITE_WORD(addr, val)	UNIX_WRITE_WORD(LINEAR2UNIX(addr), val)
#define READ_DWORD(addr)	UNIX_READ_DWORD(LINEAR2UNIX(addr))
#define WRITE_DWORD(addr, val)	UNIX_WRITE_DWORD(LINEAR2UNIX(addr), val)
#define READ_BYTEP(addr)	READ_BYTE(DOSADDR_REL(addr))
#define WRITE_BYTEP(addr, val)	WRITE_BYTE(DOSADDR_REL(addr), val)
#define READ_WORDP(addr)	READ_WORD(DOSADDR_REL(addr))
#define WRITE_WORDP(addr, val)	WRITE_WORD(DOSADDR_REL(addr), val)
#define READ_DWORDP(addr)	READ_DWORD(DOSADDR_REL(addr))
#define WRITE_DWORDP(addr, val)	WRITE_DWORD(DOSADDR_REL(addr), val)
#define read_byte(addr)		READ_BYTE(addr)
#define read_word(addr)		READ_WORD(addr)
#define read_dword(addr)	READ_DWORD(addr)

#define MEMCPY_2DOS(dos_addr, unix_addr, n) \
	memcpy(LINEAR2UNIX(dos_addr), (unix_addr), (n))
#define MEMCPY_2UNIX(unix_addr, dos_addr, n) \
	memcpy((unix_addr), LINEAR2UNIX(dos_addr), (n))
#define MEMCPY_DOS2DOS(dos_addr1, dos_addr2, n) \
	memcpy(LINEAR2UNIX(dos_addr1), LINEAR2UNIX(dos_addr2), (n))
#define MEMMOVE_DOS2DOS(dos_addr1, dos_addr2, n) \
	memmove(LINEAR2UNIX(dos_addr1), LINEAR2UNIX(dos_addr2), (n))
#define MEMSET_DOS(dos_addr, val, n) \
	memset(LINEAR2UNIX(dos_addr), (val), (n))
#define memcpy_dos2dos(dest, src, n) MEMCPY_DOS2DOS(dest, src, n)

#endif
