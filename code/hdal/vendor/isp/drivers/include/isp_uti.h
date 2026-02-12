#ifndef _ISP_UTI_H_
#define _ISP_UTI_H_

extern void *isp_uti_kmem_alloc(UINT32 mem_size);
extern void isp_uti_kmem_free(void *mem_addr);
extern void *isp_uti_vmem_alloc(UINT32 mem_size);
extern void isp_uti_vmem_free(void *mem_addr);
extern ER isp_uti_enable_pwm(UINT32 pwm_id, UINT32 period, UINT32 duty_cycle, UINT32 polarity);
extern ER isp_uti_set_pwm(UINT32 pwm_id, UINT32 period, UINT32 duty_cycle, UINT32 polarity);
extern ER isp_uti_disable_pwm(UINT32 pwm_id);

#endif

