#ifndef HLT_H
#define HLT_H
/* dosemu2's hlt handlers have no counterpart in a DPMI client;
 * the type is here only for the prototype in msdoshlp.h */
typedef struct {
  const char *name;
  void *func;
} emu_hlt_t;
#endif
