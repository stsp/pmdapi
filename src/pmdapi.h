#ifndef PMDAPI_H
#define PMDAPI_H

#include <stdint.h>

/* what the resident service provider calls tell us about clients */
void pmdapi_rsp_pre(int op, int clnt, uint16_t ds);
void pmdapi_rsp_post(int op, int clnt, int prev);

int ldtmon_call32(unsigned eax, unsigned ebx, unsigned ecx, unsigned edx,
	const void *api);
int ldtmon_call16(unsigned eax, unsigned ebx, unsigned ecx, unsigned edx,
	const void *api);

void wrapper_init(void);
void msdos_plugin_init(void);

#endif
