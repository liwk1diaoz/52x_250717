#ifndef _VPE_UTI_H_
#define _VPE_UTI_H_

extern void *vpe_uti_mem_alloc(UINT32 mem_size);
extern void vpe_uti_mem_free(void *mem_addr);
extern UINT32 vpe_uti_calc_2dlut_tbl_size(UINT32 _2dlut_idx);
extern UINT32 vpe_uti_calc_2dlut_ioctl_size(UINT32 _2dlut_idx);
extern void vpe_uti_calc_2dlut_output_size(USIZE *size, UINT32 lut_sz, UINT32 *lut_tbl);

#endif

