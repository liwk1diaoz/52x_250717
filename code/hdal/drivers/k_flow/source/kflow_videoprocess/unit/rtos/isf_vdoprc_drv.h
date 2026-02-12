#ifndef __ISF_VDOPRC_DRV_H
#define __ISF_VDOPRC_DRV_H
int kflow_videoproc_init(void);
void *isf_vdoprc_drv_malloc(unsigned int want_size);
void isf_vdoprc_drv_free(void *p_buf);
#endif

