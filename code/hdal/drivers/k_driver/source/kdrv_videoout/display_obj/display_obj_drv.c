#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include "display_obj_drv.h"
#include "display_obj_ioctl.h"
#include "display_obj_dbg.h"
#include "display.h"
#include <mach/fmem.h>

#ifdef CONFIG_NVT_SMALL_HDAL
#define VERF_DRV_EN 0
#else
#define VERF_DRV_EN 1
#endif
#include "kwrap/file.h"

/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
int nvt_dispobj_drv_wait_cmd_complete(PMODULE_INFO pmodule_info);
int nvt_dispobj_drv_ioctl(unsigned char uc_if, MODULE_INFO *pmodule_info, unsigned int cmd, unsigned long arg);
/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
typedef irqreturn_t (*irq_handler_t)(int, void *);
typedef struct _cfg_line_t_{
	int size;
	char buf[256];
} cfg_line_t;

typedef struct _attr_t_{
	char name[24];
	char num;
	void *target;
	unsigned char size;
} attr_t;

typedef struct _disp_func_t_{
	gamma_t gamma;
	cc_t	color_control;
} disp_func_t;

typedef struct _display_info_t_{
	int gamma_en;
	int cc_en;

	disp_func_t func;
} display_info_t;



#define BUF_Y           0
#define BUF_CB          1
#define BUF_CR          2
#define BUF_RGB         0
#define BUF_ALPHA       1
#define BUF_OSD         0
#define base_color_num  (5*4)

#define B_COMP              0
#define G_COMP              1
#define R_COMP              2
#define Y_COMP              0
#define U_COMP              1
#define V_COMP              2
#define A_COMP              3

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/
int i_even_flag = 0;
#if VERF_DRV_EN
static PDISP_OBJ       p_emu_disp_obj;//
static DISPCTRL_PARAM  emu_disp_ctrl;//
static DISPDEV_PARAM   emu_disp_dev; //
static DISPLAYER_PARAM emu_disp_lyr;


unsigned char base_color20[base_color_num][3] = {
	//y   cb    cr
	{0x96, 0x29, 0x15},    //green
	{0x4C, 0x55, 0xFF},    //red
	{0x00, 0x80, 0x80},    //black
	{0xFF, 0x80, 0x80},    //white
	{0x1D, 0xFF, 0x6B},    //blue

	//
	{0xE2, 0x00, 0x95},    //Y
	{0xB3, 0xAB, 0x00},    //C
	{0x69, 0xD4, 0xEB},    //M
	{0x96, 0x29, 0x15},    //green
	{0x4C, 0x55, 0xFF},    //red

	//
	{0x97, 0x2B, 0xCA},    //orange
	{0xBC, 0x16, 0x55},    //YG
	{0xA4, 0x6C, 0x0B},    //GB
	{0xFF, 0x80, 0x80},    //white
	{0x1D, 0xFF, 0x6B},    //blue

	//
	{0x68, 0xD5, 0x36},    //BG
	{0x43, 0xF3, 0xAB},    //P
	{0x5B, 0x95, 0xF5},    //Rose
	{0x1D, 0xFF, 0x6B},    //blue
	{0x4C, 0x55, 0xFF}    //red
};

//define base color RGB
unsigned char base_color20rgb[base_color_num][3] = {
	//R   G    B
	{0x00, 0xFF, 0x00},    //green
	{0xFF, 0x00, 0x00},    //red
	{0x00, 0x00, 0x00},    //black
	{0xFF, 0xFF, 0xFF},    //white
	{0x00, 0x00, 0xFF},    //blue

	//
	{0xFF, 0xFF, 0x00},    //Y
	{0x00, 0xFF, 0xFF},    //C
	{0xFF, 0x00, 0xFF},    //M
	{0x00, 0xFF, 0x00},    //green
	{0xFF, 0x00, 0x00},    //red

	//
	{0x97, 0x2B, 0xCA},    //?orange
	{0xBC, 0x16, 0x55},    //?YG
	{0xA4, 0x6C, 0x0B},    //?GB
	{0xaa, 0xaa, 0xaa},    //?white
	{0x00, 0x00, 0xFF},    //blue

	//
	{0x68, 0xD5, 0x36},    //?BG
	{0x43, 0xF3, 0xAB},    //?P
	{0x5B, 0x95, 0xF5},    //?Rose
	{0x00, 0x00, 0xFF},    //blue
	{0xFF, 0x00, 0x00}    //red
};
#endif
/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int nvt_dispobj_drv_open(PMODULE_INFO pmodule_info, unsigned char uc_if)
{
	nvt_dbg(IND, "%d\n", uc_if);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}


int nvt_dispobj_drv_release(PMODULE_INFO pmodule_info, unsigned char uc_if)
{
	nvt_dbg(IND, "%d\n", uc_if);

	/* Add HW Moduel release operation here when device file closed */

	return 0;
}

int nvt_dispobj_drv_init(MODULE_INFO *pmodule_info)
{
	int i_ret = 0;

	init_waitqueue_head(&pmodule_info->dispobj_wait_queue);
	// coverity[side_effect_free]
	spin_lock_init(&pmodule_info->dispobj_spinlock);
	sema_init(&pmodule_info->dispobj_sem, 1);
	init_completion(&pmodule_info->dispobj_completion);

	/* initial clock here */


	/* Add HW Module initialization here when driver loaded */

	return i_ret;
}

int nvt_dispobj_drv_remove(MODULE_INFO *pmodule_info)
{
	/* Add HW Moduel release operation here*/
#if VERF_DRV_EN
	p_emu_disp_obj = NULL;
	memset((void *)(&emu_disp_ctrl), 0, sizeof(emu_disp_ctrl));
	memset((void *)(&emu_disp_dev), 0, sizeof(emu_disp_dev));
	memset((void *)(&emu_disp_dev), 0, sizeof(emu_disp_dev));
#endif
	return 0;
}

int nvt_dispobj_drv_suspend(MODULE_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");

	/* Add suspend operation here*/

	return 0;
}

int nvt_dispobj_drv_resume(MODULE_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");
	/* Add resume operation here*/

	return 0;
}

void read_line(char *start, int size, cfg_line_t *line)
{
	for (line->size = 0; line->size < size; line->size++) {
		if (start[line->size] == '#')
			break;
	}

	if (size > 256) {
		DBG_ERR("valid data (%d) > 256\r\n", size);
	} else {
		memcpy(line->buf, start, line->size);
	}

	DBG_IND("(%d) %s, %d\r\n", line->size, line->buf, size);
}

int parse_gamma_cfg(cfg_line_t *line, void *pstCfg)
{
	int i, tmp;
	gamma_t *gamma_info = (gamma_t *)pstCfg;
	int j = 0;
	char *string;
	char *token;

	attr_t attr[] = {
		{"enable=",	 1, &gamma_info->enable, 4},
		{"r_table=", 	 33, &gamma_info->gamma_r, 1},
		{"g_table=", 	 33, &gamma_info->gamma_g, 1},
		{"b_table=", 	 33, &gamma_info->gamma_b, 1},
	};

	for (i = 0; i < (int)(sizeof(attr)/sizeof(attr_t)); i++) {
		if (!strncmp(line->buf, attr[i].name, strlen(attr[i].name))) {
			if (!strcmp(attr[i].name, "enable=")) {
				sscanf(line->buf + strlen(attr[i].name), "%d", ((int *)attr[i].target));
			} else if (!strcmp(attr[i].name, "r_table=")) {
				string = (char *)(line->buf + strlen(attr[i].name));
				for (token = strsep(&string, ","); token != NULL; token = strsep(&string, ",")) {
					sscanf(token, "%d", &tmp);
					gamma_info->gamma_r[j++] = tmp;
				}
			} else if (!strcmp(attr[i].name, "g_table=")) {
				string = (char *)(line->buf + strlen(attr[i].name));
				for (token = strsep(&string, ","); token != NULL; token = strsep(&string, ",")) {
					sscanf(token, "%d", &tmp);
					gamma_info->gamma_g[j++] = tmp;
				}
			} else if (!strcmp(attr[i].name, "b_table=")) {
				string = (char *)(line->buf + strlen(attr[i].name));
				for (token = strsep(&string, ","); token != NULL; token = strsep(&string, ",")) {
					sscanf(token, "%d", &tmp);
					gamma_info->gamma_b[j++] = tmp;
				}
			}
			break;
		}
	}

	return 1;
}

int parse_cc_cfg(cfg_line_t *line, void *pstCfg)
{
	int i;
	cc_t *cc_info = (cc_t *)pstCfg;

	attr_t attr[] = {
		{"enable=",	 1, &cc_info->enable, 4},
		{"saturation_offset=", 	 1, &cc_info->sat_ofs, 1},
		{"brightness_offset=", 	 1, &cc_info->int_ofs, 1},
		{"ycon=", 	 1, &cc_info->ycon, 1},
		{"ccon=", 	 1, &cc_info->ccon, 1},
	};

	for (i = 0; i < (int)(sizeof(attr)/sizeof(attr_t)); i++) {
		if (!strncmp(line->buf, attr[i].name, strlen(attr[i].name))) {
			sscanf(line->buf + strlen(attr[i].name), "%d", ((int *)attr[i].target));
			break;
		}
	}


	return 1;
}

int display_parse_cfg(char *buf, size_t buf_size, unsigned int id)
{
	typedef struct _entry_t_{
		char name[32];
		int  *enable;
		void *pstCfg;
		int  (*pfunc)(cfg_line_t *line, void *pstCfg);
	} entry_t;

	PDISP_OBJ       disp_obj;
	DISPCTRL_PARAM stfunc;

	display_info_t disp_info;
	char *start = buf;
	
	entry_t entry[] = {
		{"[DISP_ID]",	NULL,				NULL,							NULL},
		{"[GAMMA]",		&disp_info.gamma_en,&disp_info.func.gamma,			parse_gamma_cfg},
		{"[COLOR_CONTROL]",&disp_info.cc_en,	&disp_info.func.color_control,	parse_cc_cfg},
	};

	char *end;
	int entry_l0_size = sizeof(entry)/sizeof(entry_t);
	int i;
	int (*pfunc)(cfg_line_t *line, void *pstCfg) = NULL;

	memset(&disp_info, 0, sizeof(display_info_t));

	do{
		cfg_line_t line = {0};
		end = strchr(start, '\n');
		if (end != start) {
			read_line(start, (end - start), &line);
			if (*start == '[') {
				pfunc = NULL;
				for (i = 0; i < entry_l0_size; i++) {
					if (!strncmp(line.buf, entry[i].name, strlen(entry[i].name))) {
						pfunc = entry[i].pfunc;
						break;
					}
				}
			} else {
				if (pfunc != NULL) {
					*entry[i].enable = pfunc(&line, entry[i].pstCfg);
				}
			}
		}
		start = end + 1;
	}while((size_t)(start - buf) < buf_size);

	DBG_IND("Disp id\r\n");
	DBG_IND("id %d\r\n", id);
	DBG_IND("Gamma\r\n");
	DBG_IND("enable %d\r\n", disp_info.func.gamma.enable);
	for (i = 0; i < 33; i++) {
		DBG_IND("r_table %d\r\n", disp_info.func.gamma.gamma_r[i]);
		DBG_IND("g_table %d\r\n", disp_info.func.gamma.gamma_g[i]);
		DBG_IND("b_table %d\r\n", disp_info.func.gamma.gamma_b[i]);
	}

	DBG_IND("Color Control\r\n");
	DBG_IND("enable %d\r\n", disp_info.func.color_control.enable);
	DBG_IND("saturation offset %d\r\n", disp_info.func.color_control.sat_ofs);
	DBG_IND("brightness offset %d\r\n", disp_info.func.color_control.int_ofs);
	DBG_IND("ycon %d\r\n", disp_info.func.color_control.ycon);
	DBG_IND("ccon %d\r\n", disp_info.func.color_control.ccon);

	disp_obj = disp_get_display_object(id);
	if (disp_obj == NULL) {
		DBG_ERR("disp obj is NULL\r\n");
		return -1;	
	}	
	if (disp_info.func.gamma.enable) {
		stfunc.SEL.SET_GAMMA_EN.b_en = TRUE;
		disp_obj->disp_ctrl(DISPCTRL_SET_GAMMA_EN, &stfunc);

		stfunc.SEL.SET_ICST0_EN.b_en = TRUE;
		disp_obj->disp_ctrl(DISPCTRL_SET_ICST0_EN, &stfunc);
		stfunc.SEL.SET_CST_EN.b_en = TRUE;
		disp_obj->disp_ctrl(DISPCTRL_SET_CST_EN, &stfunc);

		stfunc.SEL.SET_GAMMA_RGB.p_gamma_tab_r = disp_info.func.gamma.gamma_r;
		stfunc.SEL.SET_GAMMA_RGB.p_gamma_tab_g = disp_info.func.gamma.gamma_g;
		stfunc.SEL.SET_GAMMA_RGB.p_gamma_tab_b = disp_info.func.gamma.gamma_b;
		disp_obj->disp_ctrl(DISPCTRL_SET_GAMMA_RGB, &stfunc);
		
	} else {
		stfunc.SEL.SET_GAMMA_EN.b_en = FALSE;
		disp_obj->disp_ctrl(DISPCTRL_SET_GAMMA_EN, &stfunc);

		stfunc.SEL.SET_ICST0_EN.b_en = FALSE;
		disp_obj->disp_ctrl(DISPCTRL_SET_ICST0_EN, &stfunc);

		stfunc.SEL.SET_CST_EN.b_en = FALSE;
		disp_obj->disp_ctrl(DISPCTRL_SET_CST_EN, &stfunc);


	}

	if (disp_info.func.color_control.enable) {
		stfunc.SEL.SET_CCA_EN.b_en = TRUE;
		disp_obj->disp_ctrl(DISPCTRL_SET_CCA_EN, &stfunc);

		stfunc.SEL.SET_CCA_YCON_EN.b_en = TRUE;
		disp_obj->disp_ctrl(DISPCTRL_SET_CCA_YCON_EN, &stfunc);

		stfunc.SEL.SET_CCA_CCON_EN.b_en = TRUE;
		disp_obj->disp_ctrl(DISPCTRL_SET_CCA_CCON_EN, &stfunc);

		stfunc.SEL.SET_CC_EN.b_en = TRUE;
		disp_obj->disp_ctrl(DISPCTRL_SET_CC_EN, &stfunc);

		stfunc.SEL.SET_CC_SAT_OFS.isatofs = disp_info.func.color_control.sat_ofs;
		disp_obj->disp_ctrl(DISPCTRL_SET_CC_SAT_OFS, &stfunc);

		stfunc.SEL.SET_CC_INT_OFS.iintofs = disp_info.func.color_control.int_ofs;
		disp_obj->disp_ctrl(DISPCTRL_SET_CC_INT_OFS, &stfunc);

		stfunc.SEL.SET_CCA_YCON.uiycon = disp_info.func.color_control.ycon;
		disp_obj->disp_ctrl(DISPCTRL_SET_CCA_YCON, &stfunc);

		stfunc.SEL.SET_CCA_CCON.uiccon = disp_info.func.color_control.ccon;
		disp_obj->disp_ctrl(DISPCTRL_SET_CCA_CCON, &stfunc);
	} else {
		stfunc.SEL.SET_CCA_EN.b_en = FALSE;
		disp_obj->disp_ctrl(DISPCTRL_SET_CCA_EN, &stfunc);

		stfunc.SEL.SET_CCA_YCON_EN.b_en = FALSE;
		disp_obj->disp_ctrl(DISPCTRL_SET_CCA_YCON_EN, &stfunc);

		stfunc.SEL.SET_CCA_CCON_EN.b_en = FALSE;
		disp_obj->disp_ctrl(DISPCTRL_SET_CCA_CCON_EN, &stfunc);

		stfunc.SEL.SET_CC_EN.b_en = FALSE;
		disp_obj->disp_ctrl(DISPCTRL_SET_CC_EN, &stfunc);
	}
	disp_obj->load(TRUE);
	//g_vemd_info.enable = TRUE;

	return 0;
}

int display_gen_cfg(char *buf, size_t buf_size, unsigned int id)
{
	PDISP_OBJ       disp_obj;
	DISPCTRL_PARAM stfunc;
	UINT8	gamma_table[3][33];
	int i;

	disp_obj = disp_get_display_object(id);

	if (buf == NULL) {
		DBG_ERR("write buffer not ready\r\n");
		return -1;
	}
	if (disp_obj == NULL) {
		DBG_ERR("disp obj is NULL\r\n");
		return -1;	
	}

	memset(buf, 0, buf_size);

	sprintf(buf + strlen(buf), "[DISP_ID]\n");
	sprintf(buf + strlen(buf), "id=%d\n", disp_obj->is_opened() ? id : -1);
	DBG_IND("Disp id\r\n");
	DBG_IND("id %d\r\n", disp_obj->is_opened() ? id : -1);

	disp_obj->disp_ctrl(DISPCTRL_GET_GAMMA_EN, &stfunc);
	sprintf(buf + strlen(buf), "[GAMMA]\n");
	sprintf(buf + strlen(buf), "enable=%d\n", stfunc.SEL.GET_GAMMA_EN.b_en);
	DBG_IND("Gamma\r\n");
	DBG_IND("enable %d\r\n", stfunc.SEL.GET_GAMMA_EN.b_en);

	stfunc.SEL.GET_GAMMA_RGB.p_gamma_tab_r = gamma_table[0];
	stfunc.SEL.GET_GAMMA_RGB.p_gamma_tab_g = gamma_table[1];
	stfunc.SEL.GET_GAMMA_RGB.p_gamma_tab_b = gamma_table[2];
	disp_obj->disp_ctrl(DISPCTRL_GET_GAMMA_RGB, &stfunc);
	sprintf(buf + strlen(buf), "r_table=");
	for (i = 0; i < 32; i++) {
		sprintf(buf + strlen(buf), "%d,", (int)gamma_table[0][i]);
	}
	sprintf(buf + strlen(buf), "%d\n", (int)gamma_table[0][i]);
	sprintf(buf + strlen(buf), "g_table=");
	for (i = 0; i < 32; i++) {
		sprintf(buf + strlen(buf), "%d,", (int)gamma_table[1][i]);
	}
	sprintf(buf + strlen(buf), "%d\n", (int)gamma_table[1][i]);
	sprintf(buf + strlen(buf), "b_table=");
	for (i = 0; i < 32; i++) {
		sprintf(buf + strlen(buf), "%d,", (int)gamma_table[2][i]);
	}
	sprintf(buf + strlen(buf), "%d\n", (int)gamma_table[2][i]);
	for (i = 0; i < 33; i++) {
		DBG_IND("r_table %d\r\n", stfunc.SEL.GET_GAMMA_RGB.p_gamma_tab_r[i]);
		DBG_IND("g_table %d\r\n", stfunc.SEL.GET_GAMMA_RGB.p_gamma_tab_g[i]);
		DBG_IND("b_table %d\r\n", stfunc.SEL.GET_GAMMA_RGB.p_gamma_tab_b[i]);
	}

	disp_obj->disp_ctrl(DISPCTRL_GET_CC_EN, &stfunc);
	sprintf(buf + strlen(buf), "[COLOR_CONTROL]\n");
	sprintf(buf + strlen(buf), "enable=%d\n", stfunc.SEL.GET_CC_EN.b_en);
	DBG_IND("Color Control\r\n");
	DBG_IND("enable %d\r\n", stfunc.SEL.GET_CC_EN.b_en);
	disp_obj->disp_ctrl(DISPCTRL_GET_CC_SAT_OFS, &stfunc);
	sprintf(buf + strlen(buf), "saturation_offset=%d\n", stfunc.SEL.GET_CC_SAT_OFS.isatofs);
	DBG_IND("saturation offset %d\r\n", stfunc.SEL.GET_CC_SAT_OFS.isatofs);

	disp_obj->disp_ctrl(DISPCTRL_GET_CC_INT_OFS, &stfunc);
	sprintf(buf + strlen(buf), "brightness_offset=%d\n", stfunc.SEL.GET_CC_INT_OFS.iintofs);
	DBG_IND("brightness offset %d\r\n", stfunc.SEL.GET_CC_INT_OFS.iintofs);

	disp_obj->disp_ctrl(DISPCTRL_GET_CCA_YCON, &stfunc);
	sprintf(buf + strlen(buf), "ycon=%d\n", stfunc.SEL.GET_CCA_YCON.uiycon);
	DBG_IND("ycon %d\r\n", stfunc.SEL.GET_CCA_YCON.uiycon);
	disp_obj->disp_ctrl(DISPCTRL_GET_CCA_CCON, &stfunc);
	sprintf(buf + strlen(buf), "ccon=%d\n", stfunc.SEL.GET_CCA_CCON.uiccon);
	DBG_IND("ccon %d\r\n", stfunc.SEL.GET_CCA_CCON.uiccon);

	return 0;
}

int display_read_cfg(char *file_name, unsigned int id)
{
	VOS_FILE c_file = 0;
	struct vos_stat f_stat;
	//CHAR file_path[64] = { '\0' };

	char *buf = NULL;
	int ret = 0;
	DBG_ERR("1\r\n");
	//strncpy(file_path, file_name, 64);
	if ((c_file = vos_file_open(file_name, O_RDONLY, 0)) == -1) {
		DBG_ERR("open file (%s) error\r\n", file_name);
		ret = -1;
		goto exit;
	}
	if (vos_file_fstat(c_file, &f_stat) != 0) {
		DBG_ERR("fstat error\r\n");
		ret = -1;
		goto free_file;
	}
#if defined(__LINUX)
	buf = (char *)vmalloc(sizeof(char)*f_stat.st_size);
#elif defined(__FREERTOS)
	buf = (char *)malloc(sizeof(char)*f_stat.st_size);
#endif

	if (buf == NULL) {
		DBG_ERR("malloc buffer error\r\n");
		ret = -1;
		goto free_file;
	}
	if (vos_file_read(c_file, buf, f_stat.st_size) != (int)f_stat.st_size) {
		DBG_ERR("read file error\r\n");
		ret = -1;
		goto free_buf;
	}

	buf[f_stat.st_size - 1] = '\n';
	
	if (display_parse_cfg(buf, f_stat.st_size, id) != 0) {
		DBG_ERR("parsing cfg error\r\n");
		ret = -1;
	}
	
free_buf:
#if defined(__LINUX)
		vfree(buf);
#elif defined(__FREERTOS)
		free(buf);
#endif
free_file:
	vos_file_close(c_file);
exit:
	return ret;
}
EXPORT_SYMBOL(display_read_cfg);

int display_write_cfg(char file_name[64], unsigned int id)
{
	VOS_FILE c_file = 0;

	char *buf = NULL;
	int ret = 0;
	size_t buf_size = 1024*5;

	if ((c_file = vos_file_open(file_name, O_CREAT | O_WRONLY | O_SYNC, 0)) == -1) {
		DBG_ERR("open file (%s) error\r\n", file_name);
		ret = -1;
		goto exit;
	}


#if defined(__LINUX)
	buf = (char *)vmalloc(sizeof(char)*buf_size);
#elif defined(__FREERTOS)
	buf = (char *)malloc(sizeof(char)*buf_size);
#endif

	if (display_gen_cfg(buf, buf_size, id) != 0) {
		DBG_ERR("generate cfg error\r\n");
		ret = -1;
		goto exit;
	}

	vos_file_write(c_file, buf, strlen(buf));

exit:
	if (buf != NULL) {
#if defined(__LINUX)
		vfree(buf);
#elif defined(__FREERTOS)
		free(buf);
#endif
	}

	if (c_file != -1)
		vos_file_close(c_file);

	return ret;
}
static int read_cfg(unsigned long arg, unsigned int id)
{
	char file_name[65] = { '\0' };

	if (id > 0){
		DBG_ERR("read_cfg no IDE2!\r\n");
		return -1;
	}
	if (copy_from_user((void *)file_name, (void *)arg, sizeof(char)*64) != 0) {
		DBG_ERR("copy_from_user error\n");
		return -1;
	}

	DBG_IND("read file_name = %s\r\n", file_name);

	return display_read_cfg(file_name, id);
}

static int write_cfg(unsigned long arg, unsigned int id)
{
	char file_name[65] = { '\0' };
	
	if (id > 0){
		printk("write_cfg no IDE2!\r\n");
		return -1;
	}
	
	if (copy_from_user((void *)file_name, (void *)arg, sizeof(char)*64) != 0) {
		DBG_ERR("copy_from_user error\n");
		return -1;
	}

	DBG_IND("write file_name = %s\r\n", file_name);

	return display_write_cfg(file_name, id);
}

int nvt_dispobj_drv_ioctl(unsigned char uc_if, MODULE_INFO *pmodule_info, unsigned int ui_cmd, unsigned long ui_arg)
{
	//REG_INFO reg_info;
	//REG_INFO_LIST reg_info_list;
	//int i_loop;
	int i_ret = 0;

	nvt_dbg(IND, "IF-%d cmd:%x\n", uc_if, ui_cmd);



	switch (ui_cmd) {
	case DISPOBJ_IOC_START:
		/*call someone to start operation*/
		break;

	case DISPOBJ_IOC_STOP:
		/*call someone to stop operation*/
		break;
#if 0
	case DISPOBJ_IOC_READ_REG:
		i_ret = copy_from_user(&reg_info, (void __user *)ui_arg, sizeof(REG_INFO));
		if (!i_ret) {
			//reg_info.ui_value = READ_REG(pmodule_info->io_addr[uc_if] + reg_info.ui_addr);
			i_ret = copy_to_user((void __user *)ui_arg, &reg_info, sizeof(REG_INFO));
		}
		break;

	case DISPOBJ_IOC_WRITE_REG:
		i_ret = copy_from_user(&reg_info, (void __user *)ui_arg, sizeof(REG_INFO));
		if (!i_ret)
			//WRITE_REG(reg_info.ui_value, pmodule_info->io_addr[uc_if] + reg_info.ui_addr);
		{
			break;
		}

	case DISPOBJ_IOC_READ_REG_LIST:
		i_ret = copy_from_user(&reg_info_list, (void __user *)ui_arg, sizeof(REG_INFO_LIST));
		if (!i_ret) {
			for (i_loop = 0 ; i_loop < reg_info_list.ui_count; i_loop++)
				//reg_info_list.reg_list[i_loop].ui_value = READ_REG(pmodule_info->io_addr[uc_if] + reg_info_list.reg_list[i_loop].ui_addr);

			{
				i_ret = copy_to_user((void __user *)ui_arg, &reg_info_list, sizeof(REG_INFO_LIST));
			}
		}
		break;
	case DISPOBJ_IOC_WRITE_REG_LIST:
		i_ret = copy_from_user(&reg_info_list, (void __user *)ui_arg, sizeof(REG_INFO_LIST));
		if (!i_ret) {
			for (i_loop = 0 ; i_loop < reg_info_list.ui_count ; i_loop++)
				//WRITE_REG(reg_info_list.reg_list[i_loop].ui_value, pmodule_info->io_addr[uc_if] + reg_info_list.reg_list[i_loop].ui_addr);
			{
				break;
			}
		}
		break;
#endif
	case DISPOBJ_IOC_WRITE_CFG:
		i_ret = write_cfg(ui_arg, 0);
		break;

	case DISPOBJ_1_IOC_WRITE_CFG:
		i_ret = write_cfg(ui_arg, 1);
		break;

	case DISPOBJ_IOC_READ_CFG:
		i_ret = read_cfg(ui_arg, 0);
		break;

	case DISPOBJ_1_IOC_READ_CFG:
		i_ret = read_cfg(ui_arg, 1);
		break;

		/* Add other operations here */
	}

	return i_ret;
}

int nvt_dispobj_drv_wait_cmd_complete(PMODULE_INFO pmodule_info)
{
	wait_for_completion(&pmodule_info->dispobj_completion);
	return 0;
}

int nvt_dispobj_drv_write_reg(PMODULE_INFO pmodule_info, unsigned long addr, unsigned long value)
{
	//WRITE_REG(value, pmodule_info->io_addr[0] + addr);
	return 0;
}

int nvt_dispobj_drv_read_reg(PMODULE_INFO pmodule_info, unsigned long addr)
{
	return 0;//READ_REG(pmodule_info->io_addr[0] + addr);
}

#if VERF_DRV_EN
int nvt_dispobj_drv_lcd_open(PMODULE_INFO pmodule_info, unsigned int open)
{
	void __iomem *top_addr;
	UINT32 temp_reg;

	top_addr = ioremap_nocache(0xf0010000, 0x200);

	temp_reg = READ_REG(top_addr + 0x8);
	WRITE_REG((0x4 | temp_reg), top_addr + 0x8);

	temp_reg = READ_REG(top_addr + 0xb8);
	WRITE_REG((temp_reg & 0xfffff000), top_addr + 0xb8);

	p_emu_disp_obj = disp_get_display_object(DISP_1);
	p_emu_disp_obj->open();

	emu_disp_dev.SEL.HOOK_DEVICE_OBJECT.dev_id         = DISPDEV_ID_PANEL;
	emu_disp_dev.SEL.HOOK_DEVICE_OBJECT.p_disp_dev_obj   = p_emu_disp_obj->dev_callback();
	p_emu_disp_obj->dev_ctrl(DISPDEV_HOOK_DEVICE_OBJECT, &emu_disp_dev);

//	emu_disp_ctrl.SEL.SET_SRCCLK.src_clk = DISPCTRL_SRCCLK_PLL2;
//	emu_disp_ctrl.SEL.SET_SRCCLK.src_clk = DISPCTRL_SRCCLK_PLL1;
//	emu_disp_ctrl.SEL.SET_SRCCLK.src_clk = DISPCTRL_SRCCLK_PLL4;
//	emu_disp_ctrl.SEL.SET_SRCCLK.src_clk = DISPCTRL_SRCCLK_PLL14;
//	p_emu_disp_obj->disp_ctrl(DISPCTRL_SET_SRCCLK, &emu_disp_ctrl);
	emu_disp_dev.SEL.SET_REG_IF.lcd_ctrl     = DISPDEV_LCDCTRL_SIF;
	emu_disp_dev.SEL.SET_REG_IF.ui_sif_ch     = SIF_CH1;
	//emu_disp_dev.SEL.SET_REG_IF.ui_gpio_sen   = GPIO_LCD_SIF_SEN;
	//emu_disp_dev.SEL.SET_REG_IF.ui_gpio_clk   = GPIO_LCD_SIF_SCK;
	//emu_disp_dev.SEL.SET_REG_IF.ui_gpio_data  = GPIO_LCD_SIF_SDA;
	p_emu_disp_obj->dev_ctrl(DISPDEV_SET_REG_IF, &emu_disp_dev);

	p_emu_disp_obj->dev_ctrl(DISPDEV_CLOSE_DEVICE, NULL);

	emu_disp_dev.SEL.GET_PREDISPSIZE.dev_id = DISPDEV_ID_PANEL;
	p_emu_disp_obj->dev_ctrl(DISPDEV_GET_PREDISPSIZE, &emu_disp_dev);
	nvt_dbg(IND, "Pre Get Size =%d, %d\r\n", emu_disp_dev.SEL.GET_PREDISPSIZE.ui_buf_width, emu_disp_dev.SEL.GET_PREDISPSIZE.ui_buf_height);

	p_emu_disp_obj->dev_ctrl(DISPDEV_GET_LCDMODE, &emu_disp_dev);
	nvt_dbg(IND, "LCD mode =%d\r\n", emu_disp_dev.SEL.GET_LCDMODE.mode);

	emu_disp_dev.SEL.OPEN_DEVICE.dev_id = DISPDEV_ID_PANEL;
	emu_disp_dev.SEL.OPEN_DEVICE.user_data = 0;
	p_emu_disp_obj->dev_ctrl(DISPDEV_OPEN_DEVICE, &emu_disp_dev);

	return 0;//READ_REG(pmodule_info->io_addr[0] + addr);
}

int nvt_dispobj_drv_tv_open(PMODULE_INFO pmodule_info, unsigned int open)
{
	/*
	void __iomem *top_addr;
	UINT32 temp_reg;

	top_addr = ioremap_nocache(0xf0010000, 0x200);

	temp_reg = READ_REG(top_addr + 0x8);
	WRITE_REG((0x4 | temp_reg), top_addr + 0x8);

	temp_reg = READ_REG(top_addr + 0xb8);
	WRITE_REG((temp_reg & 0xfffff000), top_addr + 0xb8);

	p_emu_disp_obj = disp_get_display_object(DISP_1);
	p_emu_disp_obj->open();

	emu_disp_dev.SEL.HOOK_DEVICE_OBJECT.dev_id         = DISPDEV_ID_TVNTSC;
	emu_disp_dev.SEL.HOOK_DEVICE_OBJECT.p_disp_dev_obj   = dispdev_get_tv_dev_obj();
	p_emu_disp_obj->dev_ctrl(DISPDEV_HOOK_DEVICE_OBJECT, &emu_disp_dev);

	p_emu_disp_obj->dev_ctrl(DISPDEV_CLOSE_DEVICE, NULL);

	emu_disp_ctrl.SEL.SET_SRCCLK.src_clk = DISPCTRL_SRCCLK_PLL2;
	p_emu_disp_obj->disp_ctrl(DISPCTRL_SET_SRCCLK ,&emu_disp_ctrl);

	emu_disp_dev.SEL.SET_TVADJUST.tv_adjust = DISPDEV_TVADJUST_3_3_K;
	p_emu_disp_obj->dev_ctrl(DISPDEV_SET_TVADJUST,&emu_disp_dev);

	emu_disp_dev.SEL.GET_PREDISPSIZE.dev_id = DISPDEV_ID_TVNTSC;
	p_emu_disp_obj->dev_ctrl(DISPDEV_GET_PREDISPSIZE, &emu_disp_dev);
	nvt_dbg(IND, "Pre Get Size =%d, %d\r\n", emu_disp_dev.SEL.GET_PREDISPSIZE.ui_buf_width, emu_disp_dev.SEL.GET_PREDISPSIZE.ui_buf_height);

	emu_disp_dev.SEL.OPEN_DEVICE.dev_id = DISPDEV_ID_TVNTSC;
	p_emu_disp_obj->dev_ctrl(DISPDEV_OPEN_DEVICE, &emu_disp_dev);

	// Enable IDE
	emu_disp_ctrl.SEL.SET_ENABLE.b_en = TRUE;
	p_emu_disp_obj->disp_ctrl(DISPCTRL_SET_ENABLE, &emu_disp_ctrl);
*/
	return 0;//READ_REG(pmodule_info->io_addr[0] + addr);
}


int nvt_dispobj_drv_fill_background(PMODULE_INFO pmodule_info, unsigned int color)
{
	if (color == 0) {
		emu_disp_ctrl.SEL.SET_BACKGROUND.ui_color_y = 0x00;
		emu_disp_ctrl.SEL.SET_BACKGROUND.ui_color_cb = 0x80;
		emu_disp_ctrl.SEL.SET_BACKGROUND.ui_color_cr = 0x80;
		p_emu_disp_obj->disp_ctrl(DISPCTRL_SET_BACKGROUND, &emu_disp_ctrl);
	} else if (color == 1) {
		emu_disp_ctrl.SEL.SET_BACKGROUND.ui_color_y = 0xff;
		emu_disp_ctrl.SEL.SET_BACKGROUND.ui_color_cb = 0x80;
		emu_disp_ctrl.SEL.SET_BACKGROUND.ui_color_cr = 0x80;
		p_emu_disp_obj->disp_ctrl(DISPCTRL_SET_BACKGROUND, &emu_disp_ctrl);
	} else if (color == 2) {
		emu_disp_ctrl.SEL.SET_BACKGROUND.ui_color_y = 0x4d;
		emu_disp_ctrl.SEL.SET_BACKGROUND.ui_color_cb = 0x55;
		emu_disp_ctrl.SEL.SET_BACKGROUND.ui_color_cr = 0xff;
		p_emu_disp_obj->disp_ctrl(DISPCTRL_SET_BACKGROUND, &emu_disp_ctrl);
	} else if (color == 3) {
		emu_disp_ctrl.SEL.SET_BACKGROUND.ui_color_y = 112;
		emu_disp_ctrl.SEL.SET_BACKGROUND.ui_color_cb = 72;
		emu_disp_ctrl.SEL.SET_BACKGROUND.ui_color_cr = 58;
		p_emu_disp_obj->disp_ctrl(DISPCTRL_SET_BACKGROUND, &emu_disp_ctrl);
	} else if (color == 4) {
		emu_disp_ctrl.SEL.SET_BACKGROUND.ui_color_y = 35;
		emu_disp_ctrl.SEL.SET_BACKGROUND.ui_color_cb = 212;
		emu_disp_ctrl.SEL.SET_BACKGROUND.ui_color_cr = 114;
		p_emu_disp_obj->disp_ctrl(DISPCTRL_SET_BACKGROUND, &emu_disp_ctrl);
	} else {
		nvt_dbg(WRN, "not support\r\n");
	}
	p_emu_disp_obj->load(TRUE);

	return 0;//READ_REG(pmodule_info->io_addr[0] + addr);
}

void fill_video_block422p(unsigned int width, unsigned int height, unsigned int loff, UINT32 y_addr, UINT32 cb_addr)
{
	UINT32  H_num, V_num, i, j, a;
	UINT32  uicbcr;
	UINT8   y;
	UINT32 t_width, t_height, k = 0;

	H_num   = 5;
	V_num   = 4;

	p_emu_disp_obj->dev_ctrl(DISPDEV_GET_LCDMODE, &emu_disp_dev);
	nvt_dbg(IND, "LCD mode =%d, width %d, height %d, loff %d\r\n", emu_disp_dev.SEL.GET_LCDMODE.mode, width, height, loff);
	nvt_dbg(IND, "Fill buffer Block mode = %d\r\n", emu_disp_dev.SEL.GET_LCDMODE.mode);

	if (width % H_num) {
		H_num = 4;
	}
	t_width  = width / H_num;
	t_height = height / V_num;

	for (j = 0; j < height; j++) {

		if ((!(j % t_height)) && j) {
			k += 5;
		}


		for (i = 0; i < H_num - 1; i++) {

			if (emu_disp_dev.SEL.GET_LCDMODE.mode == DISPDEV_LCDMODE_CCIR656) {
				y = base_color20[i + k][BUF_Y];
				uicbcr = (UINT32)(base_color20[i + k][BUF_CB]) + ((UINT32)base_color20[i + k][BUF_CR] << 8);

				if (y == 0x00) {
					y++;
				} else if (y == 0xFF) {
					y--;
				}

				if (uicbcr == 0x00) {
					uicbcr++;
				} else if (uicbcr == 0xFF) {
					uicbcr--;
				}
				memset((void *)(y_addr + loff * j + t_width * i), y,  t_width);
				for (a = 0; a < t_width; a++) {
					memset((void *)(cb_addr + (loff)*j + (t_width)*i) + a * 2, (uicbcr&0xff), 1);
					memset((void *)(cb_addr + (loff)*j + (t_width)*i) + a * 2 + 1, ((uicbcr >> 8)&0xff), 1);
				}
			} else {
				//uicbcr = (UINT32)(base_color20[i + k][BUF_CB]) + ((UINT32)base_color20[i + k][BUF_CR] << 8);
				memset((void *)(y_addr + loff * j + t_width * i), base_color20[i + k][BUF_Y],  t_width);
				for (a = 0; a < t_width; a++) {
					memset((void *)(cb_addr + (loff)*j + (t_width)*i) + a * 2, base_color20[i + k][BUF_CB], 1);
					memset((void *)(cb_addr + (loff)*j + (t_width)*i) + a * 2 + 1, base_color20[i + k][BUF_CR], 1);
				}
			}
		}

		if (emu_disp_dev.SEL.GET_LCDMODE.mode == DISPDEV_LCDMODE_CCIR656) {
			y = base_color20[i + k][BUF_Y];
			uicbcr = (UINT32)(base_color20[i + k][BUF_CB]) + ((UINT32)base_color20[i + k][BUF_CR] << 8);
			if (y == 0x00) {
				y++;
			} else if (y == 0xFF) {
				y--;
			}

			if (uicbcr == 0x00) {
				uicbcr++;
			} else if (uicbcr == 0xFF) {
				uicbcr--;
			}
			memset((void *)(y_addr + loff * j + t_width * i), y,  t_width + (width % H_num));
			for (a = 0; a < t_width + (width % H_num); a++) {
				memset((void *)(cb_addr + (loff)*j + (t_width)*i) + a * 2, (uicbcr&0xff), 1);
				memset((void *)(cb_addr + (loff)*j + (t_width)*i) + a * 2 + 1, ((uicbcr >> 8)&0xff), 1);
			}

		} else {
			//uicbcr = (UINT32)(base_color20[i + k][BUF_CB]) + ((UINT32)base_color20[i + k][BUF_CR] << 8);
			memset((void *)(y_addr + loff * j + t_width * i), base_color20[i + k][BUF_Y],  t_width + (width % H_num));
			for (a = 0; a < t_width + (width % H_num); a++) {
				memset((void *)(cb_addr + (loff)*j + (t_width)*i) + a * 2, (base_color20[i + k][BUF_CB]), 1);
				memset((void *)(cb_addr + (loff)*j + (t_width)*i) + a * 2 + 1, base_color20[i + k][BUF_CR], 1);
			}
		}
	}
}

void fill_osd_block(unsigned int width, unsigned int height, unsigned int loff, UINT32 rgb_addr)
{
	UINT32  H_num, V_num, i, j;
	UINT32 t_width, t_height, k = 0, q, total;
	UINT16 color;

	H_num   = 5;
	V_num   = 4;
	t_width  = width / H_num;
	t_height = height / V_num;

	for (j = 0; j < height; j++) {

		if ((!(j % t_height)) && j) {
			k += H_num;
		}


		for (i = 0; i < H_num - 1; i++) {
			color = ((base_color20rgb[i + k][Y_COMP] & 0xF8) << 8) | ((base_color20rgb[i + k][U_COMP] >> 2) << 5) | (base_color20rgb[i + k][V_COMP] >> 3);
			total = t_width * 2;
			//pBuf = (UINT16 *)(rgb_addr + loff * j + t_width * i * 2);

			for (q = 0; q < total / 2; q++) {
				//pBuf[q] = color;
				memset((void *)(rgb_addr + loff * j + t_width * i * 2 + q * 2), color&0xff, 1);
				memset((void *)(rgb_addr + loff * j + t_width * i * 2 + q * 2 + 1), (color >> 8)&0xff, 1);
			}
		}

		color = ((base_color20rgb[i + k][Y_COMP] & 0xF8) << 8) | ((base_color20rgb[i + k][U_COMP] >> 2) << 5) | (base_color20rgb[i + k][V_COMP] >> 3);
		total = (t_width + (width % H_num)) * 2;
		//pBuf = (UINT16 *)(rgb_addr + loff * j + t_width * i * 2);

		for (q = 0; q < total / 2; q++) {
			//pBuf[q] = color;
			memset((void *)(rgb_addr + loff * j + t_width * i * 2 + q * 2), color&0xff, 1);
			memset((void *)(rgb_addr + loff * j + t_width * i * 2 + q * 2 + 1), (color >> 8)&0xff, 1);
		}

	}

}

int nvt_dispobj_drv_fill_video(PMODULE_INFO pmodule_info, unsigned int format)
{
	/*
	frammap_buf_t      buf_info = {0};
	frammap_buf_t      buf_info_y = {0};
	frammap_buf_t      buf_info_cbcr = {0};
	VIDEO_BUF_ATTR     v_buf_attr;
	unsigned long phys_addr = 0;
	char __iomem *virt_addr[4] = {0};
	unsigned int buffer_len = 0;
	struct fb_var_screeninfo var;
	struct fb_fix_screeninfo fix;


	buf_info_y.size = 960 * 240;
	buf_info_y.align = 64;      ///< address alignment
	buf_info_y.name = "video_y";
	buf_info_y.alloc_type = ALLOC_CACHEABLE;
	//pdrv_info->vaddr = buf_info.va_addr;
	//pdrv_info->paddr = (void*)buf_info.phy_addr;

	buf_info_cbcr.size = 0x40000;
	buf_info_cbcr.align = 64;      ///< address alignment
	buf_info_cbcr.name = "video_uv";
	buf_info_cbcr.alloc_type = ALLOC_CACHEABLE;


	buf_info.size = buf_info_y.size + buf_info_cbcr.size;
	buf_info.align = buf_info_cbcr.align > buf_info_y.align ? buf_info_cbcr.align : buf_info_y.align;
	buf_info.name = "video";
	buf_info.alloc_type = ALLOC_CACHEABLE;

	frm_get_buf_ddr(DDR_ID0, &buf_info);

	buf_info_y.phy_addr = buf_info.phy_addr;
	buf_info_y.va_addr = buf_info.va_addr;

	buf_info_cbcr.phy_addr = buf_info.phy_addr + buf_info_y.size;
	buf_info_cbcr.va_addr = buf_info.va_addr + buf_info_y.size;

	nvt_dbg(IND, "buf_info_y.va_addr = 0x%08x, buf_info_y.phy_addr = 0x%08x\r\n", (UINT32)buf_info_y.va_addr, (UINT32)buf_info_y.phy_addr);
	nvt_dbg(IND, "buf_info_cbcr.va_addr = 0x%08x, buf_info_cbcr.phy_addr = 0x%08x\r\n", (UINT32)buf_info_cbcr.va_addr, (UINT32)buf_info_cbcr.phy_addr);

	//pdrv_info->vaddr = buf_info.va_addr;
	//pdrv_info->paddr = (void*)buf_info.phy_addr;
	nvt_dbg(IND, "buf_info_cbcr.va_addr = 0x%08x, buf_info_cbcr.phy_addr = 0x%08x\r\n", (UINT32)buf_info_cbcr.va_addr, (UINT32)buf_info_cbcr.phy_addr);

	emu_disp_lyr.SEL.SET_VDOBUFADDR.buf_sel    = DISPBUFADR_0;
	emu_disp_lyr.SEL.SET_VDOBUFADDR.ui_addr_y0  = (UINT32)buf_info_y.va_addr;
	emu_disp_lyr.SEL.SET_VDOBUFADDR.ui_addr_cb0 = (UINT32)buf_info_cbcr.va_addr;

	p_emu_disp_obj->disp_lyr_ctrl(DISPLAYER_VDO1, DISPLAYER_OP_SET_VDOBUFADDR, &emu_disp_lyr);

	if (format == 0) {
		emu_disp_lyr.SEL.SET_MODE.buf_format   = DISPBUFFORMAT_YUV422PACK;
		emu_disp_lyr.SEL.SET_MODE.buf_mode     = DISPBUFMODE_BUFFER_REPEAT;
		emu_disp_lyr.SEL.SET_MODE.buf_number   = DISPBUFNUM_3;
		p_emu_disp_obj->disp_lyr_ctrl(DISPLAY_VIDEO1_EN, DISPLAYER_OP_SET_MODE, &emu_disp_lyr);

		emu_disp_lyr.SEL.SET_BUFSIZE.ui_buf_width   =  960;
		emu_disp_lyr.SEL.SET_BUFSIZE.ui_buf_height  =  240;
		emu_disp_lyr.SEL.SET_BUFSIZE.ui_buf_line_ofs = ((960 + 3) & 0xFFFFFFFC);
		p_emu_disp_obj->disp_lyr_ctrl(DISPLAY_VIDEO1_EN, DISPLAYER_OP_SET_BUFSIZE, &emu_disp_lyr);
		v_buf_attr.buf_w = emu_disp_lyr.SEL.SET_BUFSIZE.ui_buf_width;
		v_buf_attr.buf_h = emu_disp_lyr.SEL.SET_BUFSIZE.ui_buf_height;
		v_buf_attr.buf_lineoffset = emu_disp_lyr.SEL.SET_BUFSIZE.ui_buf_line_ofs;
		emu_disp_lyr.SEL.SET_WINSIZE.ui_win_width   = 960;
		emu_disp_lyr.SEL.SET_WINSIZE.ui_win_height  = 240;
		emu_disp_lyr.SEL.SET_WINSIZE.i_win_ofs_x     = 0;
		emu_disp_lyr.SEL.SET_WINSIZE.i_win_ofs_y     = 0;
		p_emu_disp_obj->disp_lyr_ctrl(DISPLAY_VIDEO1_EN, DISPLAYER_OP_SET_WINSIZE, &emu_disp_lyr);

		fill_video_block422p(v_buf_attr.buf_w, v_buf_attr.buf_h, v_buf_attr.buf_lineoffset, (UINT32)buf_info_y.va_addr, (UINT32)buf_info_cbcr.va_addr);

	} else if (format == 1) {
		nvt_dbg(WRN, "not support\r\n");
	} else {
		nvt_dbg(WRN, "not support\r\n");
	}

	emu_disp_ctrl.SEL.SET_ALL_LYR_EN.b_en      = TRUE;
	emu_disp_ctrl.SEL.SET_ALL_LYR_EN.disp_lyr  = DISPLAYER_VDO1;
	p_emu_disp_obj->disp_ctrl(DISPCTRL_SET_ALL_LYR_EN, &emu_disp_ctrl);
	p_emu_disp_obj->load(TRUE);

	msleep(5000);

	display_obj_set_video1_blank(DISP_1, 1);

	msleep(5000);

	display_obj_set_video1_blank(DISP_1, 0);

	display_obj_get_video1_buf_addr(DISP_1, &phys_addr, &virt_addr[0], &buffer_len);

	display_obj_get_video1_param(DISP_1, &var, &fix);
*/
	return 0;//READ_REG(pmodule_info->io_addr[0] + addr);

}

int nvt_dispobj_drv_fill_video2(PMODULE_INFO pmodule_info, unsigned int format)
{
	/*
	frammap_buf_t      buf_info_y = {0};
	frammap_buf_t      buf_info_cbcr = {0};
	VIDEO_BUF_ATTR     v_buf_attr;

	buf_info_y.size = 0x40000;
	buf_info_y.align = 64;      ///< address alignment
	buf_info_y.name = "video_y";
	buf_info_y.alloc_type = ALLOC_CACHEABLE;
	frm_get_buf_ddr(DDR_ID0, &buf_info_y);
	//pdrv_info->vaddr = buf_info.va_addr;
	//pdrv_info->paddr = (void*)buf_info.phy_addr;
	nvt_dbg(IND, "buf_info_y.va_addr = 0x%08x, buf_info_y.phy_addr = 0x%08x\r\n", (UINT32)buf_info_y.va_addr, (UINT32)buf_info_y.phy_addr);

	buf_info_cbcr.size = 0x40000;
	buf_info_cbcr.align = 64;      ///< address alignment
	buf_info_cbcr.name = "video_uv";
	buf_info_cbcr.alloc_type = ALLOC_CACHEABLE;
	frm_get_buf_ddr(DDR_ID0, &buf_info_cbcr);
	//pdrv_info->vaddr = buf_info.va_addr;
	//pdrv_info->paddr = (void*)buf_info.phy_addr;
	nvt_dbg(IND, "buf_info_cbcr.va_addr = 0x%08x, buf_info_cbcr.phy_addr = 0x%08x\r\n", (UINT32)buf_info_cbcr.va_addr, (UINT32)buf_info_cbcr.phy_addr);

	emu_disp_lyr.SEL.SET_VDOBUFADDR.buf_sel    = DISPBUFADR_0;
	emu_disp_lyr.SEL.SET_VDOBUFADDR.ui_addr_y0  = (UINT32)buf_info_y.va_addr;
	emu_disp_lyr.SEL.SET_VDOBUFADDR.ui_addr_cb0 = (UINT32)buf_info_cbcr.va_addr;

	p_emu_disp_obj->disp_lyr_ctrl(DISPLAYER_VDO2, DISPLAYER_OP_SET_VDOBUFADDR, &emu_disp_lyr);

	if (format == 0) {
		emu_disp_lyr.SEL.SET_MODE.buf_format   = DISPBUFFORMAT_YUV422PACK;
		emu_disp_lyr.SEL.SET_MODE.buf_mode     = DISPBUFMODE_BUFFER_REPEAT;
		emu_disp_lyr.SEL.SET_MODE.buf_number   = DISPBUFNUM_3;
		p_emu_disp_obj->disp_lyr_ctrl(DISPLAY_VIDEO2_EN, DISPLAYER_OP_SET_MODE, &emu_disp_lyr);

		emu_disp_lyr.SEL.SET_BUFSIZE.ui_buf_width   =  960;
		emu_disp_lyr.SEL.SET_BUFSIZE.ui_buf_height  =  240;
		emu_disp_lyr.SEL.SET_BUFSIZE.ui_buf_line_ofs = ((960 + 3) & 0xFFFFFFFC);
		p_emu_disp_obj->disp_lyr_ctrl(DISPLAY_VIDEO2_EN, DISPLAYER_OP_SET_BUFSIZE, &emu_disp_lyr);
		v_buf_attr.buf_w = emu_disp_lyr.SEL.SET_BUFSIZE.ui_buf_width;
		v_buf_attr.buf_h = emu_disp_lyr.SEL.SET_BUFSIZE.ui_buf_height;
		v_buf_attr.buf_lineoffset = emu_disp_lyr.SEL.SET_BUFSIZE.ui_buf_line_ofs;
		emu_disp_lyr.SEL.SET_WINSIZE.ui_win_width   = 960;
		emu_disp_lyr.SEL.SET_WINSIZE.ui_win_height  = 240;
		emu_disp_lyr.SEL.SET_WINSIZE.i_win_ofs_x     = 0;
		emu_disp_lyr.SEL.SET_WINSIZE.i_win_ofs_y     = 0;
		p_emu_disp_obj->disp_lyr_ctrl(DISPLAY_VIDEO2_EN, DISPLAYER_OP_SET_WINSIZE, &emu_disp_lyr);

		fill_video_block422p(v_buf_attr.buf_w, v_buf_attr.buf_h, v_buf_attr.buf_lineoffset, (UINT32)buf_info_y.va_addr, (UINT32)buf_info_cbcr.va_addr);

	} else if (format == 1) {
		nvt_dbg(WRN, "not support\r\n");
	} else {
		nvt_dbg(WRN, "not support\r\n");
	}

	emu_disp_ctrl.SEL.SET_ALL_LYR_EN.b_en      = TRUE;
	emu_disp_ctrl.SEL.SET_ALL_LYR_EN.disp_lyr  = DISPLAYER_VDO2;
	p_emu_disp_obj->disp_ctrl(DISPCTRL_SET_ALL_LYR_EN, &emu_disp_ctrl);
	p_emu_disp_obj->load(TRUE);
*/
	return 0;//READ_REG(pmodule_info->io_addr[0] + addr);

}

int nvt_dispobj_drv_fill_osd(PMODULE_INFO pmodule_info, unsigned int format)
{
	/*
	frammap_buf_t      buf_info = {0};
	frammap_buf_t      buf_info_y = {0};
	frammap_buf_t      buf_info_cbcr = {0};
	VIDEO_BUF_ATTR     v_buf_attr;
	unsigned long phys_addr = 0;
	char __iomem *virt_addr[4] = {0};
	unsigned int buffer_len = 0;
	struct fb_var_screeninfo var = {0};
	struct fb_fix_screeninfo fix;

	buf_info_y.size = 960*240 * 2;
	buf_info_y.align = 64;      ///< address alignment
	buf_info_y.name = "video_y";
	buf_info_y.alloc_type = ALLOC_CACHEABLE;
	//pdrv_info->vaddr = buf_info.va_addr;
	//pdrv_info->paddr = (void*)buf_info.phy_addr;

	buf_info_cbcr.size = 960 * 240;
	buf_info_cbcr.align = 64;      ///< address alignment
	buf_info_cbcr.name = "video_uv";
	buf_info_cbcr.alloc_type = ALLOC_CACHEABLE;

	buf_info.size = buf_info_cbcr.size + buf_info_y.size;
	buf_info.align = 64;      ///< address alignment
	buf_info.name = "video";
	buf_info.alloc_type = ALLOC_CACHEABLE;

	frm_get_buf_ddr(DDR_ID0, &buf_info);


	buf_info_y.phy_addr = buf_info.phy_addr;
	buf_info_y.va_addr = buf_info.va_addr;

	buf_info_cbcr.phy_addr = buf_info.phy_addr + buf_info_y.size;
	buf_info_cbcr.va_addr = buf_info.va_addr + buf_info_y.size;
	//pdrv_info->vaddr = buf_info.va_addr;
	//pdrv_info->paddr = (void*)buf_info.phy_addr;
	nvt_dbg(IND, "buf_info_cbcr.va_addr = 0x%08x, buf_info_cbcr.phy_addr = 0x%08x\r\n", (UINT32)buf_info_cbcr.va_addr, (UINT32)buf_info_cbcr.phy_addr);
	nvt_dbg(IND, "buf_info_y.va_addr = 0x%08x, buf_info_y.phy_addr = 0x%08x\r\n", (UINT32)buf_info_y.va_addr, (UINT32)buf_info_y.phy_addr);
	emu_disp_lyr.SEL.SET_OSDBUFADDR.buf_sel    = DISPBUFADR_0;
	emu_disp_lyr.SEL.SET_OSDBUFADDR.ui_addr_buf0  = (UINT32)buf_info_y.phy_addr;
	emu_disp_lyr.SEL.SET_OSDBUFADDR.ui_addr_alpha0 = (UINT32)buf_info_cbcr.phy_addr;

	p_emu_disp_obj->disp_lyr_ctrl(DISPLAYER_OSD1, DISPLAYER_OP_SET_OSDBUFADDR, &emu_disp_lyr);

	if (format == 0) {
		emu_disp_lyr.SEL.SET_MODE.buf_format   = DISPBUFFORMAT_ARGB8565;
		emu_disp_lyr.SEL.SET_MODE.buf_mode     = DISPBUFMODE_BUFFER_REPEAT;
		emu_disp_lyr.SEL.SET_MODE.buf_number   = DISPBUFNUM_3;
		p_emu_disp_obj->disp_lyr_ctrl(DISPLAY_OSD1_EN, DISPLAYER_OP_SET_MODE, &emu_disp_lyr);

		emu_disp_lyr.SEL.SET_BUFSIZE.ui_buf_width   =  960;
		emu_disp_lyr.SEL.SET_BUFSIZE.ui_buf_height  =  240;
		emu_disp_lyr.SEL.SET_BUFSIZE.ui_buf_line_ofs = ((960 * 2 + 3) & 0xFFFFFFFC);
		p_emu_disp_obj->disp_lyr_ctrl(DISPLAY_OSD1_EN, DISPLAYER_OP_SET_BUFSIZE, &emu_disp_lyr);
		v_buf_attr.buf_w = emu_disp_lyr.SEL.SET_BUFSIZE.ui_buf_width;
		v_buf_attr.buf_h = emu_disp_lyr.SEL.SET_BUFSIZE.ui_buf_height;
		v_buf_attr.buf_lineoffset = emu_disp_lyr.SEL.SET_BUFSIZE.ui_buf_line_ofs;
		emu_disp_lyr.SEL.SET_WINSIZE.ui_win_width   = 960;
		emu_disp_lyr.SEL.SET_WINSIZE.ui_win_height  = 240;
		emu_disp_lyr.SEL.SET_WINSIZE.i_win_ofs_x     = 0;
		emu_disp_lyr.SEL.SET_WINSIZE.i_win_ofs_y     = 0;
		p_emu_disp_obj->disp_lyr_ctrl(DISPLAY_OSD1_EN, DISPLAYER_OP_SET_WINSIZE, &emu_disp_lyr);

		fill_osd_block(v_buf_attr.buf_w, v_buf_attr.buf_h, v_buf_attr.buf_lineoffset, (UINT32)buf_info_y.va_addr);

	} else if (format == 1) {
		nvt_dbg(WRN, "not support\r\n");
	} else {
		nvt_dbg(WRN, "not support\r\n");
	}

	emu_disp_ctrl.SEL.SET_ALL_LYR_EN.b_en      = TRUE;
	emu_disp_ctrl.SEL.SET_ALL_LYR_EN.disp_lyr  = DISPLAYER_OSD1;
	p_emu_disp_obj->disp_ctrl(DISPCTRL_SET_ALL_LYR_EN, &emu_disp_ctrl);
	p_emu_disp_obj->load(TRUE);

	msleep(5000);

	display_obj_set_osd_blank(DISP_1, 1);

	msleep(5000);

	display_obj_set_osd_blank(DISP_1, 0);

	display_obj_get_osd_buf_addr(DISP_1, &phys_addr, &virt_addr[0], &buffer_len);

	display_obj_get_osd_param(DISP_1, &var, &fix);

	display_obj_set_osd_palette(DISP_1, 0, 0xf80000ff);
*/
	return 0;//READ_REG(pmodule_info->io_addr[0] + addr);

}

int nvt_dispobj_drv_v1_scaling(PMODULE_INFO pmodule_info, unsigned int w, unsigned int h, unsigned int window)
{
	if (window == 1) {
		emu_disp_lyr.SEL.SET_WINSIZE.ui_win_width   = w;
		emu_disp_lyr.SEL.SET_WINSIZE.ui_win_height  = h;
		emu_disp_lyr.SEL.SET_WINSIZE.i_win_ofs_x     = 0;
		emu_disp_lyr.SEL.SET_WINSIZE.i_win_ofs_y     = 0;
		p_emu_disp_obj->disp_lyr_ctrl(DISPLAY_VIDEO1_EN, DISPLAYER_OP_SET_WINSIZE, &emu_disp_lyr);
	} else {
		emu_disp_lyr.SEL.SET_BUFSIZE.ui_buf_width   =  w;
		emu_disp_lyr.SEL.SET_BUFSIZE.ui_buf_height  =  h;
		emu_disp_lyr.SEL.SET_BUFSIZE.ui_buf_line_ofs = ((960 + 3) & 0xFFFFFFFC);
		p_emu_disp_obj->disp_lyr_ctrl(DISPLAY_VIDEO1_EN, DISPLAYER_OP_SET_BUFSIZE, &emu_disp_lyr);
	}

	p_emu_disp_obj->load(TRUE);

	return 0;//READ_REG(pmodule_info->io_addr[0] + addr);
}

int nvt_dispobj_drv_v2_scaling(PMODULE_INFO pmodule_info, unsigned int w, unsigned int h, unsigned int window)
{
	if (window == 1) {
		emu_disp_lyr.SEL.SET_WINSIZE.ui_win_width   = w;
		emu_disp_lyr.SEL.SET_WINSIZE.ui_win_height  = h;
		emu_disp_lyr.SEL.SET_WINSIZE.i_win_ofs_x     = 0;
		emu_disp_lyr.SEL.SET_WINSIZE.i_win_ofs_y     = 0;
		p_emu_disp_obj->disp_lyr_ctrl(DISPLAY_VIDEO2_EN, DISPLAYER_OP_SET_WINSIZE, &emu_disp_lyr);
	} else {
		emu_disp_lyr.SEL.SET_BUFSIZE.ui_buf_width   =  w;
		emu_disp_lyr.SEL.SET_BUFSIZE.ui_buf_height  =  h;
		emu_disp_lyr.SEL.SET_BUFSIZE.ui_buf_line_ofs = ((960 + 3) & 0xFFFFFFFC);
		p_emu_disp_obj->disp_lyr_ctrl(DISPLAY_VIDEO2_EN, DISPLAYER_OP_SET_BUFSIZE, &emu_disp_lyr);
	}

	p_emu_disp_obj->load(TRUE);

	return 0;//READ_REG(pmodule_info->io_addr[0] + addr);
}

int nvt_dispobj_drv_osd_scaling(PMODULE_INFO pmodule_info, unsigned int w, unsigned int h, unsigned int window)
{
	if (window == 1) {
		emu_disp_lyr.SEL.SET_WINSIZE.ui_win_width   = w;
		emu_disp_lyr.SEL.SET_WINSIZE.ui_win_height  = h;
		emu_disp_lyr.SEL.SET_WINSIZE.i_win_ofs_x     = 0;
		emu_disp_lyr.SEL.SET_WINSIZE.i_win_ofs_y     = 0;
		p_emu_disp_obj->disp_lyr_ctrl(DISPLAY_OSD1_EN, DISPLAYER_OP_SET_WINSIZE, &emu_disp_lyr);
	} else {
		emu_disp_lyr.SEL.SET_BUFSIZE.ui_buf_width   =  w;
		emu_disp_lyr.SEL.SET_BUFSIZE.ui_buf_height  =  h;
		emu_disp_lyr.SEL.SET_BUFSIZE.ui_buf_line_ofs = ((960 * 2 + 3) & 0xFFFFFFFC);
		p_emu_disp_obj->disp_lyr_ctrl(DISPLAY_OSD1_EN, DISPLAYER_OP_SET_BUFSIZE, &emu_disp_lyr);
	}

	p_emu_disp_obj->load(TRUE);

	return 0;//READ_REG(pmodule_info->io_addr[0] + addr);
}
#endif
