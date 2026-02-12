#ifndef _MSDCNVT_UITRON_H
#define _MSDCNVT_UITRON_H

#ifdef __cplusplus
extern "C"
{
#endif

int msdcnvt_uitron_init(MSDCNVT_IPC_CFG *p_ipc_cfg);
MSDCNVT_IPC_CFG *msdcnvt_uitron_init2(void);
int msdcnvt_uitron_exit(void);
int msdcnvt_uitron_cmd(MSDCNVT_ICMD *p_cmd);
unsigned char *mem_phy_to_noncache(unsigned int addr);
unsigned int mem_to_phy(void *addr);

#ifdef __cplusplus
} //extern "C"
#endif

#endif