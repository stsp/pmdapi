/*
 * pmdapi as a TSR: set up the msdos plugin, have the DPMI host call it
 * for every new client (the resident service provider of DPMI 1.0), and
 * stay resident.
 */
#include <stdio.h>
#include <stdlib.h>
#include <dpmi.h>
#include <sys/segments.h>
#include "emudpmi.h"
#include "entry.h"
#include "msdoshlp.h"
#include "pmdapi.h"

int main(void)
{
    ULONG ds_base;
    unsigned short ds = _my_ds(), cs = _my_cs();

    /* dosemu2's plugin reaches all of DOS memory through near pointers */
    if (__dpmi_get_segment_base_address(ds, &ds_base) == -1 ||
	    __dpmi_set_segment_limit(ds, 0xffffffff) == -1 ||
	    __dpmi_set_segment_limit(cs, 0xffffffff) == -1) {
	printf("pmdapi: cannot map the address space\n");
	return 1;
    }
    mem_base = (unsigned char *)-ds_base;
    dseg32 = ds;
    cur_sp = (uintptr_t)pmdapi_stack + STK_CHUNK * STK_DEPTH;

    wrapper_init();
    msdos_plugin_init();
    /* installs the resident service provider, see rsp_init() */
    msdos_reset();

    printf("pmdapi: installed\n");
    fflush(stdout);
    if (__dpmi_terminate_and_stay_resident(0, 0) == -1) {
	printf("pmdapi: the DPMI host cannot keep us resident\n");
	return 1;
    }
    return 0;
}
