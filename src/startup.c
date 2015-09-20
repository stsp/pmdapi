#include <stdio.h>
#include <sys/segments.h>
#include <dpmi.h>
#include "desc.h"
#include "ldt.h"
#include "entry.h"
#include "emmwrp.h"
#include "wrapper.h"
#include "startup.h"

int have_fs = 0, have_gs = 0;
unsigned char fs_desc[8], gs_desc[8];

int main()
{
  __dpmi_callback_info info;
  unsigned long desc[2];
  short cs16, cs32, ds16, ds32;
  unsigned long cs_base, ds_base;
  modify_ldt_t ldt;

  emm_init();
  wrapper_init();

  cs32 = _my_cs();
  ds32 = _my_ds();
  if (__dpmi_get_segment_base_address(cs32, &cs_base) == -1) {
    printf("base_addr failed\n");
    return 1;
  }
  if (__dpmi_get_segment_base_address(ds32, &ds_base) == -1) {
    printf("base_addr failed\n");
    return 1;
  }
  printf("cs=%#hx ds=%#hx cs_base=%#lx ds_base=%#lx\n", cs32, ds32, cs_base, ds_base);

  ldt.seg_32bit = 0;
  ldt.read_exec_only = 0;
  ldt.limit_in_pages = 0;
  ldt.seg_not_present = 0;

  if ((cs16 = __dpmi_allocate_ldt_descriptors(1)) == -1) {
    printf("alloc_desc failed\n");
    return 1;
  }
  ldt.entry_number = cs16 >> 3;
  ldt.base_addr = cs_base + (unsigned long)code16;
  ldt.limit = (unsigned long)code16_end - (unsigned long)code16;
  ldt.contents = MODIFY_LDT_CONTENTS_CODE;
  desc[0] = LDT_entry_a(&ldt);
  desc[1] = LDT_entry_b(&ldt);
  if (__dpmi_set_descriptor(cs16, desc) == -1) {
    printf("set_desc failed\n");
    return 1;
  }

  if ((ds16 = __dpmi_allocate_ldt_descriptors(1)) == -1) {
    printf("alloc_desc failed\n");
    return 1;
  }
  ldt.entry_number = ds16 >> 3;
  ldt.base_addr = ds_base + (unsigned long)data16;
  ldt.limit = (unsigned long)data16_end - (unsigned long)data16;
  ldt.contents = MODIFY_LDT_CONTENTS_DATA;
  desc[0] = LDT_entry_a(&ldt);
  desc[1] = LDT_entry_b(&ldt);
  if (__dpmi_set_descriptor(ds16, desc) == -1) {
    printf("set_desc failed\n");
    return 1;
  }

  if (__dpmi_get_descriptor(cs32, cs32_desc) == -1) {
    printf("get_desc failed\n");
    return 1;
  }
  if (__dpmi_get_descriptor(ds32, ds32_desc) == -1) {
    printf("get_desc failed\n");
    return 1;
  }

  if ((have_fs = _my_fs())) {
    if (__dpmi_get_descriptor(have_fs, fs_desc) == -1) {
      printf("get_desc failed\n");
      return 1;
    }
  }
  if ((have_gs = _my_gs())) {
    if (__dpmi_get_descriptor(have_gs, gs_desc) == -1) {
      printf("get_desc failed\n");
      return 1;
    }
  }

  if (__dpmi_get_descriptor(cs16, info.code16) == -1) {
    printf("get_desc failed\n");
    return 1;
  }
  if (__dpmi_get_descriptor(cs32, info.code32) == -1) {
    printf("get_desc failed\n");
    return 1;
  }
  if (__dpmi_get_descriptor(ds16, info.data16) == -1) {
    printf("get_desc failed\n");
    return 1;
  }
  if (__dpmi_get_descriptor(ds32, info.data32) == -1) {
    printf("get_desc failed\n");
    return 1;
  }
  info.ip = (unsigned long)entry16 - (unsigned long)code16;
  info.eip = (unsigned long)entry32;

  if (__dpmi_install_resident_service_provider_callback(&info) == -1) {
    printf("inst_res failed\n");
    return 1;
  }
  if (__dpmi_terminate_and_stay_resident(0, 0) == -1) {
    printf("tsr failed\n");
    return 1;
  }
  return 0;
}
