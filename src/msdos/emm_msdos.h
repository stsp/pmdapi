/* emm.h name already taken */
#ifndef EMM_MSDOS_H
#define EMM_MSDOS_H

int emm_allocate_handle(sigcontext_t *scp, int is_32, int pages_needed);
int emm_deallocate_handle(sigcontext_t *scp, int is_32, int handle);
int emm_save_handle_state(sigcontext_t *scp, int is_32, int handle);
int emm_restore_handle_state(sigcontext_t *scp, int is_32, int handle);
int emm_map_unmap_multi(sigcontext_t *scp, int is_32, const u_short *array,
    int handle, int map_len);
int emm_get_mpa_len(sigcontext_t *scp, int is_32);

struct emm_phys_page_desc {
    uint16_t seg;
    uint16_t num;
};
int emm_get_mpa_array(sigcontext_t *scp, int is_32,
	struct emm_phys_page_desc *array, int max_len);

#endif
