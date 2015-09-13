#ifndef __OFFSETS_H
#define __OFFSETS_H

#include <sys/types.h>
#include <stddef.h>

#define DEFINE(sym, val) \
        asm volatile("\n->" #sym " %0 " #val : : "i" (val))

#define BLANK() asm volatile("\n->" : : )

#define OFFSET(sym, str, mem) \
	DEFINE(sym, offsetof(struct str, mem));

#define DO_OFFSETS(num, offs) \
    void foo_##num(void) \
    { \
	offs \
    }

#endif
