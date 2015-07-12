#include "cpu.h"
#include "emm.h"

int emm_get_partial_map_registers(void *ptr, const u_short *segs)
{
  return 0;
}

void emm_set_partial_map_registers(const void *ptr)
{
}

int emm_get_size_for_partial_page_map(int pages)
{
  return 0;
}

int emm_map_unmap_multi(const u_short *array, int handle, int map_len)
{
  return 0;
}
