#include <kwrap/cpu.h>
#include "vpe_dbg.h"
//#include "vpe_drv_util_int.h"
#include "vpe_dce_cal.h"
//#include "kdrv_videoprocess/vpe/vpe_drv_limit.h"
//#include "vpe_eng.h"
#include <linux/math64.h>



#define VPE_DRV_DCE_CONV2_2D_VAL(x_val, y_val) (((y_val & 0xffff) << 16) | (x_val & 0xffff))
#define VPE_DRV_DCE_SET_2D_VAL(x_pos, y_pos, align_w, buf_va, val) buf_va[(y_pos * align_w) + x_pos] = val
#define VPE_DRV_DCE_GET_2D_VAL(x_pos, y_pos, align_w, buf_va) (buf_va[(y_pos * align_w) + x_pos])
#define VPE_DRV_DCE_GET_2D_Y_VAL(val) ((val >> 16) & 0xffff)
#define VPE_DRV_DCE_GET_2D_X_VAL(val) (val & 0xffff)
#define VPE_DRV_DCE_SVAL_MAX_MASK 0x7fff

struct vpe_drv_dce_point {
	int x;
	int y;
};

#define RATIO_BASE 100
#define TAB_BASE 100000
#define TAB_NUM 256
#define TAB_PI 205887	//pi (3.1415926 * (1 << 16)

static const int sin_tab[256] = {
	0, 2454, 4907, 7356, 9802, 12241, 14673, 17096, 19509, 21910, 24298, 26671, 29028, 31368, 33689, 35990,
	38268, 40524, 42756, 44961, 47140, 49290, 51410, 53500, 55557, 57581, 59570, 61523, 63439, 65317, 67156, 68954,
	70711, 72425, 74095, 75721, 77301, 78835, 80321, 81758, 83147, 84485, 85773, 87009, 88192, 89322, 90399, 91421,
	92388, 93299, 94154, 94953, 95694, 96378, 97003, 97570, 98079, 98528, 98918, 99248, 99518, 99729, 99880, 99970,
	100000, 99970, 99880, 99729, 99518, 99248, 98918, 98528, 98079, 97570, 97003, 96378, 95694, 94953, 94154, 93299,
	92388, 91421, 90399, 89322, 88192, 87009, 85773, 84485, 83147, 81758, 80321, 78835, 77301, 75721, 74095, 72425,
	70711, 68954, 67156, 65317, 63439, 61523, 59570, 57581, 55557, 53500, 51410, 49290, 47140, 44961, 42756, 40524,
	38268, 35990, 33689, 31368, 29028, 26671, 24298, 21910, 19509, 17096, 14673, 12241, 9802, 7356, 4907, 2454,
	0, -2454, -4907, -7356, -9802, -12241, -14673, -17096, -19509, -21910, -24298, -26671, -29028, -31368, -33689, -35990,
	-38268, -40524, -42756, -44961, -47140, -49290, -51410, -53500, -55557, -57581, -59570, -61523, -63439, -65317, -67156, -68954,
	-70711, -72425, -74095, -75721, -77301, -78835, -80321, -81758, -83147, -84485, -85773, -87009, -88192, -89322, -90399, -91421,
	-92388, -93299, -94154, -94953, -95694, -96378, -97003, -97570, -98079, -98528, -98918, -99248, -99518, -99729, -99880, -99970,
	-100000, -99970, -99880, -99729, -99518, -99248, -98918, -98528, -98079, -97570, -97003, -96378, -95694, -94953, -94154, -93299,
	-92388, -91421, -90399, -89322, -88192, -87009, -85773, -84485, -83147, -81758, -80321, -78835, -77301, -75721, -74095, -72425,
	-70711, -68954, -67156, -65317, -63439, -61523, -59570, -57581, -55557, -53500, -51410, -49290, -47140, -44961, -42756, -40524,
	-38268, -35990, -33689, -31368, -29028, -26671, -24298, -21910, -19509, -17096, -14673, -12241, -9802, -7356, -4907, -2454
};
static const int cos_tab[256] = {
	100000, 99970, 99880, 99729, 99518, 99248, 98918, 98528, 98079, 97570, 97003, 96378, 95694, 94953, 94154, 93299,
	92388, 91421, 90399, 89322, 88192, 87009, 85773, 84485, 83147, 81758, 80321, 78835, 77301, 75721, 74095, 72425,
	70711, 68954, 67156, 65317, 63439, 61523, 59570, 57581, 55557, 53500, 51410, 49290, 47140, 44961, 42756, 40524,
	38268, 35990, 33689, 31368, 29028, 26671, 24298, 21910, 19509, 17096, 14673, 12241, 9802, 7356, 4907, 2454,
	0, -2454, -4907, -7356, -9802, -12241, -14673, -17096, -19509, -21910, -24298, -26671, -29028, -31368, -33689, -35990,
	-38268, -40524, -42756, -44961, -47140, -49290, -51410, -53500, -55557, -57581, -59570, -61523, -63439, -65317, -67156, -68954,
	-70711, -72425, -74095, -75721, -77301, -78835, -80321, -81758, -83147, -84485, -85773, -87009, -88192, -89322, -90399, -91421,
	-92388, -93299, -94154, -94953, -95694, -96378, -97003, -97570, -98079, -98528, -98918, -99248, -99518, -99729, -99880, -99970,
	-100000, -99970, -99880, -99729, -99518, -99248, -98918, -98528, -98079, -97570, -97003, -96378, -95694, -94953, -94154, -93299,
	-92388, -91421, -90399, -89322, -88192, -87009, -85773, -84485, -83147, -81758, -80321, -78835, -77301, -75721, -74095, -72425,
	-70711, -68954, -67156, -65317, -63439, -61523, -59570, -57581, -55557, -53500, -51410, -49290, -47140, -44961, -42756, -40524,
	-38268, -35990, -33689, -31368, -29028, -26671, -24298, -21910, -19509, -17096, -14673, -12241, -9802, -7356, -4907, -2454,
	0, 2454, 4907, 7356, 9802, 12241, 14673, 17096, 19509, 21910, 24298, 26671, 29028, 31368, 33689, 35990,
	38268, 40524, 42756, 44961, 47140, 49290, 51410, 53500, 55557, 57581, 59570, 61523, 63439, 65317, 67156, 68954,
	70711, 72425, 74095, 75721, 77301, 78835, 80321, 81758, 83147, 84485, 85773, 87009, 88192, 89322, 90399, 91421,
	92388, 93299, 94154, 94953, 95694, 96378, 97003, 97570, 98079, 98528, 98918, 99248, 99518, 99729, 99880, 99970
};

static int vpe_drv_sin(int rad)
{
	int idx0, idx1, val0, val1, val;
	int rad0, rad1;

	idx0 = (rad * TAB_NUM) / 2 / TAB_PI;
	rad0 = ((2 * TAB_PI * idx0) + (TAB_NUM >> 1)) / TAB_NUM;
	if (idx0 >= TAB_NUM) {
		idx0 -= TAB_NUM;
	}
	val0 = sin_tab[idx0];

	idx1 = idx0 + 1;
	rad1 = ((2 * TAB_PI * idx1) + (TAB_NUM >> 1)) / TAB_NUM;
	if (idx1 >= TAB_NUM) {
		idx1 -= TAB_NUM;
	}
	val1 = sin_tab[idx1];
	val = val0 + (val1 - val0) * (rad - rad0) / (rad1 - rad0);
	return val;
}

int vpe_drv_cos(int rad)
{
	int idx0, idx1, val0, val1, val;
	int rad0, rad1;

	idx0 = (rad * TAB_NUM) / 2 / TAB_PI;
	rad0 = ((2 * TAB_PI * idx0) + (TAB_NUM >> 1)) / TAB_NUM;
	if (idx0 >= TAB_NUM) {
		idx0 -= TAB_NUM;
	}
	val0 = cos_tab[idx0];

	idx1 = idx0 + 1;
	rad1 = ((2 * TAB_PI * idx1) + (TAB_NUM >> 1)) / TAB_NUM;
	if (idx1 >= TAB_NUM) {
		idx1 -= TAB_NUM;
	}
	val1 = cos_tab[idx1];
	val = val0 + (val1 - val0) * (rad - rad0) / (rad1 - rad0);
	return val;
}

int vpe_drv_abs(int val)
{
	if (val < 0) {
		return -val;
	}
	return val;
}

static void vpe_drv_dce_dump_2dlut(unsigned int *lut2d_buf, unsigned int x_num, unsigned int y_num)
{
#if 0
#define MSG_SIZE 512
	char msg[MSG_SIZE];
	unsigned int i, j;
	int cnt;


	for (j = 0; j < y_num; j++) {
		cnt = 0;
		for (i = 0; i < DCE_2DLUT_ROT_W_ALIGN; i++) {
			cnt += snprintf(&msg[cnt], (MSG_SIZE - cnt), "%.8x ", lut2d_buf[i + (j * DCE_2DLUT_ROT_W_ALIGN)]);
		}
		DBG_ERR("%s\n", msg);
	}
#undef MSG_SIZE
#endif
}

static int vpe_drv_dce_gen_2dlut_by_coner_pt(unsigned long *buf_va, struct vpe_drv_dce_point *lt_pt, struct vpe_drv_dce_point *rt_pt, struct vpe_drv_dce_point *rb_pt, struct vpe_drv_dce_point *lb_pt)
{
	int tmp_x, tmp_y, overflow_flag, x_num, y_num;
	int i, j;
	unsigned int *lut2d_buf;
	struct vpe_drv_dce_point st_pt, end_pt;

	x_num = DCE_2DLUT_ROT_X_MAX;
	y_num = DCE_2DLUT_ROT_Y_MAX;

	//set to va_buf & interpolate remain points
	lut2d_buf = (unsigned int *)buf_va;
	for (j = 0; j < y_num; j++) {

		//cal next st_pt/end_pt
		tmp_x = ((lb_pt->x - lt_pt->x) * j) + ((x_num - 1) * lt_pt->x);
		st_pt.x = (tmp_x << 2) / (x_num - 1);
		tmp_y = ((lb_pt->y - lt_pt->y) * j) + ((x_num - 1) * lt_pt->y);
		st_pt.y = (tmp_y << 2) / (x_num - 1);

		tmp_x = ((rb_pt->x - rt_pt->x) * j) + ((x_num - 1) * rt_pt->x);
		end_pt.x = (tmp_x << 2) / (x_num - 1);
		tmp_y = ((rb_pt->y - rt_pt->y) * j) + ((x_num - 1) * rt_pt->y);
		end_pt.y = (tmp_y << 2) / (x_num - 1);

		for (i = 0; i < x_num; i++) {
			tmp_x = ((end_pt.x - st_pt.x) * i) + ((x_num - 1) * st_pt.x);
			tmp_x = tmp_x / (x_num - 1);

			tmp_y = ((end_pt.y - st_pt.y) * i) + ((x_num - 1) * st_pt.y);
			tmp_y = tmp_y / (x_num - 1);

			overflow_flag = 0;
			overflow_flag |= (vpe_drv_abs(tmp_x) & (~VPE_DRV_DCE_SVAL_MAX_MASK));
			overflow_flag |= (vpe_drv_abs(tmp_y) & (~VPE_DRV_DCE_SVAL_MAX_MASK));
			if (overflow_flag != 0) {
				DBG_ERR("2dlut value overflow\r\n");
				return -1;
			}
			//DBG_DUMP("%d %d\n", tmp_x, tmp_y);
			//add for limit
			if(tmp_x < 0){
				tmp_x = 0;
			}
			if(tmp_y < 0){
				tmp_y = 0;
			}
			VPE_DRV_DCE_SET_2D_VAL(i, j, DCE_2DLUT_ROT_W_ALIGN, lut2d_buf, VPE_DRV_DCE_CONV2_2D_VAL(tmp_x, tmp_y));
		}
	}
	vpe_drv_dce_dump_2dlut(lut2d_buf, x_num, y_num);
	return 0;
}

#if 0
#endif
static int vpe_drv_ce_gen_coner_pt_fix(struct KDRV_VPE_ROI roi, enum KDRV_VPE_DCE_ROT rot, struct vpe_drv_dce_point *dst_pt)
{
	struct vpe_drv_dce_point org_pt[4];
	int rot_map_tab[VPE_DRV_DCE_ROT_MAX][4] = {
		{ 0, 1, 2, 3 }, //VPE_DRV_DCE_ROT_0
		{ 3, 0, 1, 2 }, //VPE_DRV_DCE_ROT_90,
		{ 2, 3, 0, 1 }, //VPE_DRV_DCE_ROT_180,
		{ 1, 2, 3, 0 }, //VPE_DRV_DCE_ROT_270,
		{ 1, 0, 3, 2 }, //VPE_DRV_DCE_ROT_0_H_FLIP,
		{ 0, 3, 2, 1 }, //VPE_DRV_DCE_ROT_90_H_FLIP,
		{ 3, 2, 1, 0 }, //VPE_DRV_DCE_ROT_180_H_FLIP,
		{ 2, 1, 0, 3 }, //VPE_DRV_DCE_ROT_270_H_FLIP,
	};

	//gen rot/flip pt
	/*
	0----1
	|    |
	3----2
	*/

	//gen default point
	org_pt[0].x = 0;
	org_pt[0].y = 0;
	org_pt[1].x = (roi.w - 1);
	org_pt[1].y = 0;
	org_pt[2].x = (roi.w - 1);
	org_pt[2].y = (roi.h - 1);
	org_pt[3].x = 0;
	org_pt[3].y = (roi.h - 1);
	//DBG_DUMP("org: (%d %d)-(%d %d)-(%d %d)-(%d %d)\r\n",
	//		org_pt[0].x, org_pt[0].y, org_pt[1].x, org_pt[1].y, org_pt[2].x, org_pt[2].y, org_pt[3].x, org_pt[3].y);
	//apply rot/flip
	dst_pt[0] = org_pt[rot_map_tab[rot][0]];
	dst_pt[1] = org_pt[rot_map_tab[rot][1]];
	dst_pt[2] = org_pt[rot_map_tab[rot][2]];
	dst_pt[3] = org_pt[rot_map_tab[rot][3]];
	//DBG_DUMP("rlt: (%d %d)-(%d %d)-(%d %d)-(%d %d)\r\n",
		//dst_pt[0].x, dst_pt[0].y, dst_pt[1].x, dst_pt[1].y, dst_pt[2].x, dst_pt[2].y, dst_pt[3].x, dst_pt[3].y);

	return 0;
}
/*static*/int vpe_drv_dce_gen_rot_2dlut_fix(unsigned long *buf_va, unsigned int buf_size, struct KDRV_VPE_ROI roi, enum KDRV_VPE_DCE_ROT rot)
{
	int rt;
	struct vpe_drv_dce_point new_pt[4];

	if (buf_va == 0) {
		DBG_ERR("err buf_va is null\r\n");
		return -1;
	}

	if (DCE_2DLUT_ALLOC_SIZE != buf_size) {
		DBG_ERR("err buf size 0x%x , 0x%x\r\n", buf_size, DCE_2DLUT_BUF_SIZE);
		return -1;
	}

	if ((roi.w == 0) || (roi.h == 0)) {
		DBG_ERR("err roi(%d %d)\r\n", roi.w, roi.h);
		return -1;
	}

	//DBG_DUMP("roi(%d %d) rot = %d 2dlut(%lx %x %dx%d)\r\n", roi.w, roi.h, rot, (unsigned long)buf_va, buf_size, DCE_2DLUT_ROT_X_MAX, DCE_2DLUT_ROT_Y_MAX);

	//get coner pt
	vpe_drv_ce_gen_coner_pt_fix(roi, rot, new_pt);

	//set to va_buf & interpolate remain points
	rt = vpe_drv_dce_gen_2dlut_by_coner_pt(buf_va, &new_pt[0], &new_pt[1], &new_pt[2], &new_pt[3]);
	if (rt < 0) {
		DBG_ERR("get 2dlut fail\n");
	}
	return rt;
}

#if 0
#endif
struct vpe_drv_dce_point vpe_drv_dce_pt_apply_rot(struct vpe_drv_dce_point pt, struct vpe_drv_dce_point cen_pt, int rad, int ratio_x, int  ratio_y, int ratio_mode)
{
	struct vpe_drv_dce_point rt_pt;
	int cos_val, sin_val;
	long long tmp_x, tmp_y, div_x, div_y;
	//long mod;
	cos_val = vpe_drv_cos(rad);
	sin_val = vpe_drv_sin(rad);

	//* TAB_BASE
	tmp_x = ((long long)(pt.x - cen_pt.x) * cos_val) - ((long long)(pt.y - cen_pt.y) * sin_val);
	tmp_y = ((long long)(pt.x - cen_pt.x) * sin_val) + ((long long)(pt.y - cen_pt.y) * cos_val);
	div_x = TAB_BASE;
	div_y = TAB_BASE;
	if (ratio_mode == VPE_DRV_DCE_ROT_RAT_FIX_ASPECT_RATIO_UP) {
		div_x = div_x * ratio_x;
		div_y = div_y * ratio_y;
		tmp_x = tmp_x * RATIO_BASE;
		tmp_y = tmp_y * RATIO_BASE;
	}
	else if (ratio_mode == VPE_DRV_DCE_ROT_RAT_FIX_ASPECT_RATIO_DOWN) {
		div_x = div_x * RATIO_BASE;
		div_y = div_y * RATIO_BASE;
		tmp_x = tmp_x * ratio_x;
		tmp_y = tmp_y * ratio_y;
	}
	else {
		div_x = div_x * ratio_x;
		div_y = div_y * ratio_x;
		tmp_x = tmp_x * RATIO_BASE;
		tmp_y = tmp_y * RATIO_BASE;
	}

	if (tmp_x > 0) {
		tmp_x = tmp_x + (div_x >> 1);
	} else {
		tmp_x = tmp_x - (div_x >> 1);
	}

	if (tmp_y > 0) {
		tmp_y = tmp_y + (div_y >> 1);
	}
	else {
		tmp_y = tmp_y - (div_y >> 1);
	}
	tmp_x = div64_s64(tmp_x, div_x);
	tmp_y = div64_s64(tmp_y, div_y);
	//tmp_x = tmp_x / div_x;
	//tmp_y = tmp_y / div_y;
	rt_pt.x = cen_pt.x + (int)tmp_x;
	rt_pt.y = cen_pt.y + (int)tmp_y;
	return rt_pt;
}
static int vpe_drv_dce_gen_coner_pt_fix_aspect_ratio(struct KDRV_VPE_ROI roi, struct KDRV_VPE_DCE_ROT_PARAM rot_param, struct vpe_drv_dce_point *dst_pt)
{
	int ratio, rad, flip_idx, i;
	struct vpe_drv_dce_point org_pt[4], rot_pt[4], cen_pt;
	long long tmp, div;
	int flip_map_tab[4][4] = {
		{ 0, 1, 2, 3 }, //deg 0
		{ 1, 0, 3, 2 }, //deg 0 & h flip
		{ 3, 2, 1, 0 }, //deg 0 & v flip
		{ 2, 3, 0, 1 }, //deg 0 & hv flip
	};

	//apply flip
	rad = rot_param.rad;
	flip_idx = rot_param.flip;
	if (rot_param.flip == 1) {
		//h flip
		rad = VPE_DRV_DCE_ROT_RAD_MAX - rad;
	}
	else if (rot_param.flip == 2) {
		//v flip
		rad = VPE_DRV_DCE_ROT_RAD_MAX - rad;
	}

	//gen rot pt
	/*
	0----1
	|    |
	3----2
	*/

	//gen default point
	org_pt[0].x = 0;
	org_pt[0].y = 0;
	org_pt[1].x = (roi.w - 1);
	org_pt[1].y = 0;
	org_pt[2].x = (roi.w - 1);
	org_pt[2].y = (roi.h - 1);
	org_pt[3].x = 0;
	org_pt[3].y = (roi.h - 1);
	//DBG_DUMP("org: (%d %d)-(%d %d)-(%d %d)-(%d %d)\r\n",
		//org_pt[0].x, org_pt[0].y, org_pt[1].x, org_pt[1].y, org_pt[2].x, org_pt[2].y, org_pt[3].x, org_pt[3].y);

	//gen rot infor
	cen_pt.x = (roi.w >> 1);
	cen_pt.y = (roi.h >> 1);
	ratio = rot_param.ratio;
	if (rot_param.ratio_mode != VPE_DRV_DCE_ROT_RAT_MANUAL) {
		if (roi.w < roi.h) {
			//ratio = (vpe_drv_abs(vpe_drv_cos(rad)) + (vpe_drv_abs(vpe_drv_sin(rad)) * roi.h + (roi.w >> 1)) / roi.w) * RATIO_BASE / TAB_BASE;
			tmp = ((long long)vpe_drv_abs(vpe_drv_cos(rad)) * roi.w) + ((long long)vpe_drv_abs(vpe_drv_sin(rad)) * roi.h);
			tmp = tmp * RATIO_BASE;
			div = ((long long)roi.w * TAB_BASE);
		}
		else {
			//ratio = (vpe_drv_abs(vpe_drv_cos(rad)) + (vpe_drv_abs(vpe_drv_sin(rad)) * roi.w + (roi.h >> 1)) / roi.h) * RATIO_BASE / TAB_BASE;
			tmp = ((long long)vpe_drv_abs(vpe_drv_cos(rad)) * roi.h) + ((long long)vpe_drv_abs(vpe_drv_sin(rad)) * roi.w);
			tmp = tmp * RATIO_BASE;
			div = ((long long)roi.h * TAB_BASE);
		}
		
		//ratio = (int)((tmp + (div >> 1)) / div);
		//DBG_DUMP("tmp : (%lld)\r\n",tmp);
		tmp = (tmp + (div >> 1)) ;

		//DBG_DUMP("tmp div: (%lld %lld)\r\n",tmp,div);

		
		tmp = div64_s64(tmp, div);
		ratio = (int)(tmp);
		//DBG_DUMP("ratio : (%d )\r\n",ratio);

		
	}

	//apply rot
	for (i = 0; i < 4; i++) {
		rot_pt[i] = vpe_drv_dce_pt_apply_rot(org_pt[i], cen_pt, rad, ratio, ratio, rot_param.ratio_mode);
	}
	//DBG_DUMP("rot: (%d %d)-(%d %d)-(%d %d)-(%d %d)\r\n",
		//rot_pt[0].x, rot_pt[0].y, rot_pt[1].x, rot_pt[1].y, rot_pt[2].x, rot_pt[2].y, rot_pt[3].x, rot_pt[3].y);

	//apply flip
	dst_pt[0] = rot_pt[flip_map_tab[flip_idx][0]];
	dst_pt[1] = rot_pt[flip_map_tab[flip_idx][1]];
	dst_pt[2] = rot_pt[flip_map_tab[flip_idx][2]];
	dst_pt[3] = rot_pt[flip_map_tab[flip_idx][3]];
	//DBG_DUMP("rlt: (%d %d)-(%d %d)-(%d %d)-(%d %d)\r\n",
		//dst_pt[0].x, dst_pt[0].y, dst_pt[1].x, dst_pt[1].y, dst_pt[2].x, dst_pt[2].y, dst_pt[3].x, dst_pt[3].y);

	return 0;
}

#if 0
#endif
struct vpe_drv_dce_point vpe_drv_dce_pt_apply_rot_fit_roi(struct vpe_drv_dce_point pt, int rad, int idx, struct KDRV_VPE_ROI roi)
{
	struct vpe_drv_dce_point rt_pt;
	int cos_val, sin_val;
	long long tmp_x, tmp_y, div_x, div_y;
	//long mod;

	cos_val = vpe_drv_cos(rad);
	sin_val = vpe_drv_sin(rad);

	div_x = ((long long)TAB_BASE * TAB_BASE);
	div_y = ((long long)TAB_BASE * TAB_BASE);
	tmp_x = 0;
	tmp_y = 0;
	if (idx == 3) {
		tmp_x = (roi.h - 1);
		tmp_x = -(tmp_x * sin_val * cos_val);
		tmp_y = (roi.h - 1);
		tmp_y = -(tmp_y * sin_val * sin_val);
		//rt_pt.x = pt.x - ((long long)(roi.h - 1) * sin_val * cos_val) / TAB_BASE / TAB_BASE;
		//rt_pt.y = pt.y - ((long long)(roi.h - 1) * sin_val * sin_val) / TAB_BASE / TAB_BASE;
	} else if (idx == 2) {
		tmp_x = (roi.w - 1);
		tmp_x = -(tmp_x * sin_val * sin_val);
		tmp_y = (roi.w - 1);
		tmp_y = (tmp_y * sin_val * cos_val);
		//rt_pt.x = pt.x - ((long long)(roi.w - 1) * sin_val * sin_val) / TAB_BASE / TAB_BASE;
		//rt_pt.y = pt.y + ((long long)(roi.w - 1) * sin_val * cos_val) / TAB_BASE / TAB_BASE;
	} else if (idx == 1) {
		tmp_x = (roi.h - 1);
		tmp_x = (tmp_x * sin_val * cos_val);
		tmp_y = (roi.h - 1);
		tmp_y = (tmp_y * sin_val * sin_val);
		//rt_pt.x = pt.x + ((long long)(roi.h - 1) * sin_val * cos_val) / TAB_BASE / TAB_BASE;
		//rt_pt.y = pt.y + ((long long)(roi.h - 1) * sin_val * sin_val) / TAB_BASE / TAB_BASE;
	} else if (idx == 0) {
		tmp_x = (roi.w - 1);
		tmp_x = (tmp_x * sin_val * sin_val);
		tmp_y = (roi.w - 1);
		tmp_y = -(tmp_y * sin_val * cos_val);
		//rt_pt.x = pt.x + ((long long)(roi.w - 1) * sin_val * sin_val) / TAB_BASE / TAB_BASE;
		//rt_pt.y = pt.y - ((long long)(roi.w - 1) * sin_val * cos_val) / TAB_BASE / TAB_BASE;
	}

	if (tmp_x > 0) {
		tmp_x = tmp_x + (div_x >> 1);
	}
	else {
		tmp_x = tmp_x - (div_x >> 1);
	}

	if (tmp_y > 0) {
		tmp_y = tmp_y + (div_y >> 1);
	}
	else {
		tmp_y = tmp_y - (div_y >> 1);
	}
	//DBG_DUMP("tmp_x div_x: (%lld %lld)\r\n",tmp_x,div_x);
	//DBG_DUMP("tmp_y div_y: (%lld %lld)\r\n",tmp_y,div_y);

	
	tmp_x = div64_s64(tmp_x, div_x);
	tmp_y = div64_s64(tmp_y, div_y);
	//tmp_x = tmp_x / div_x;
	//tmp_y = tmp_y / div_y;
	//DBG_DUMP("tmp_x : (%lld)\r\n",tmp_x);
	//DBG_DUMP("tmp_y : (%lld)\r\n",tmp_y);

	
	rt_pt.x = pt.x + (int)tmp_x;
	rt_pt.y = pt.y + (int)tmp_y;
	return rt_pt;
}
static int vpe_drv_dce_gen_coner_pt_fit_roi(struct KDRV_VPE_ROI roi, struct KDRV_VPE_DCE_ROT_PARAM rot_param, struct vpe_drv_dce_point *dst_pt)
{
	int rad, i, cir_ofs_idx, flip_idx;
	struct vpe_drv_dce_point org_pt[4], rot_pt[4], flip_pt[4];

	int flip_map_tab[3][4] = {
		{ 0, 1, 2, 3 }, //deg 0
		{ 1, 0, 3, 2 }, //deg 0 & h flip
		{ 3, 2, 1, 0 }, //deg 0 & v flip
	};

	int cir_ofs_map_tab[4][4] = {
		{ 0, 1, 2, 3 }, //0
		{ 1, 2, 3, 0 }, //90,
		{ 2, 3, 0, 1 }, //180,
		{ 3, 0, 1, 2 }, //270,
	};

	rad = rot_param.rad;
	//apply flip
	flip_idx = 0;
	if (rot_param.flip == 1) {
		//h flip
		flip_idx = 1;
		rad = VPE_DRV_DCE_ROT_RAD_MAX - rad;
	} else if (rot_param.flip == 2) {
		//v flip
		flip_idx = 2;
		rad = VPE_DRV_DCE_ROT_RAD_MAX - rad;
	} else if (rot_param.flip == 3) {
		//hv flip
		rad += 205887;
	}

	if (rad >= VPE_DRV_DCE_ROT_RAD_MAX) {
		rad -= VPE_DRV_DCE_ROT_RAD_MAX;
	}

	cir_ofs_idx = 0;
	//deg 270 ~ 360
	if ((rad >= 308831) && (rad < VPE_DRV_DCE_ROT_RAD_MAX)) {
		rad -= 308831;
		cir_ofs_idx = 3;
	}
	else if ((rad >= 205887) && (rad < 308831)) {
		//deg 180 ~ 270
		rad -= 205887;
		cir_ofs_idx = 2;
	}
	else if ((rad >= 102994) && (rad < 205887)) {
		//deg 90 ~ 180
		rad -= 102994;
		cir_ofs_idx = 1;
	}

	//gen rot pt
	/*
	0----1
	|    |
	3----2
	*/

	//gen default point
	org_pt[0].x = 0;
	org_pt[0].y = 0;
	org_pt[1].x = (roi.w - 1);
	org_pt[1].y = 0;
	org_pt[2].x = (roi.w - 1);
	org_pt[2].y = (roi.h - 1);
	org_pt[3].x = 0;
	org_pt[3].y = (roi.h - 1);
	//DBG_DUMP("org: (%d %d)-(%d %d)-(%d %d)-(%d %d)\r\n",
		//org_pt[0].x, org_pt[0].y, org_pt[1].x, org_pt[1].y, org_pt[2].x, org_pt[2].y, org_pt[3].x, org_pt[3].y);

	//apply rot
	for (i = 0; i < 4; i++) {
		rot_pt[i] = vpe_drv_dce_pt_apply_rot_fit_roi(org_pt[i], rad, i, roi);
	}

	flip_pt[0] = rot_pt[cir_ofs_map_tab[cir_ofs_idx][0]];
	flip_pt[1] = rot_pt[cir_ofs_map_tab[cir_ofs_idx][1]];
	flip_pt[2] = rot_pt[cir_ofs_map_tab[cir_ofs_idx][2]];
	flip_pt[3] = rot_pt[cir_ofs_map_tab[cir_ofs_idx][3]];

	dst_pt[0] = flip_pt[flip_map_tab[flip_idx][0]];
	dst_pt[1] = flip_pt[flip_map_tab[flip_idx][1]];
	dst_pt[2] = flip_pt[flip_map_tab[flip_idx][2]];
	dst_pt[3] = flip_pt[flip_map_tab[flip_idx][3]];

	//DBG_DUMP("rlt: (%d %d)-(%d %d)-(%d %d)-(%d %d)\r\n",
		//dst_pt[0].x, dst_pt[0].y, dst_pt[1].x, dst_pt[1].y, dst_pt[2].x, dst_pt[2].y, dst_pt[3].x, dst_pt[3].y);

	return 0;
}

/*static*/ int vpe_drv_dce_gen_rot_2dlut_deg(unsigned long *buf_va, unsigned int buf_size, struct KDRV_VPE_ROI roi, struct KDRV_VPE_DCE_ROT_PARAM rot_param)
{
	int rt;
	struct vpe_drv_dce_point new_pt[4];

	if (buf_va == 0) {
		DBG_ERR("err buf_va is null\r\n");
		return -1;
	}

	if (DCE_2DLUT_ALLOC_SIZE != buf_size) {
		DBG_ERR("err buf size 0x%x , 0x%x\r\n", buf_size, DCE_2DLUT_ALLOC_SIZE);
		return -1;
	}

	if ((roi.w == 0) || (roi.h == 0)) {
		DBG_ERR("err roi(%d %d)\r\n", roi.w, roi.h);
		return -1;
	}

	if (rot_param.ratio_mode == VPE_DRV_DCE_ROT_RAT_MANUAL) {
		if (rot_param.ratio == 0) {
			DBG_ERR("err rot_param.ratio = 0\r\n");
			return -1;
		}
	}

	if (rot_param.rad > VPE_DRV_DCE_ROT_RAD_MAX) {
		DBG_ERR("err rot_param->rad(%d > %d) overlfow\r\n", rot_param.rad, VPE_DRV_DCE_ROT_RAD_MAX);
		return -1;
	}

	//DBG_DUMP("roi(%d %d) rot_param(%d %d %d %d) 2dlut(%lx %x %dx%d)\r\n", roi.w, roi.h, rot_param.rad,
	//			rot_param.flip, rot_param.ratio_mode, rot_param.ratio, (unsigned long)buf_va, buf_size, DCE_2DLUT_ROT_X_MAX, DCE_2DLUT_ROT_Y_MAX);

	//get coner pt
	if (rot_param.ratio_mode == VPE_DRV_DCE_ROT_RAT_FIT_ROI) {
		vpe_drv_dce_gen_coner_pt_fit_roi(roi, rot_param, new_pt);
	} else {
		vpe_drv_dce_gen_coner_pt_fix_aspect_ratio(roi, rot_param, new_pt);
	}

	//set to va_buf & interpolate remain points
	rt = vpe_drv_dce_gen_2dlut_by_coner_pt(buf_va, &new_pt[0], &new_pt[1], &new_pt[2], &new_pt[3]);
	if (rt < 0) {
		DBG_ERR("get 2dlut fail\n");
	}
	return 0;
}

#if 0
int vpe_drv_dce_proc(VPE_ENG_HANDLE *p_eng_hdl, struct vpe_drv_job_cfg *job_cfg, struct vpe_drv_lut2d_info *lut2d)
{
    VPE_ENG_DCE_PARAM dce_param;
    VPE_ENG_DCE_GEO_CFG geo_lut;
    UINT8 i, dce_2d_lut_en;
	uintptr_t pcie_paddr;
	int rt;

    if (job_cfg->in.path != VPE_DRV_DCE_PATH) {
        return 0;
    }
    dce_param.dce_mode = job_cfg->dce_param.dce_mode;

    if (job_cfg->dctg_param.dctg_en)
        dce_param.lut2d_sz = job_cfg->dctg_param.lut2d_sz;
    else
        dce_param.lut2d_sz = job_cfg->dce_param.lut2d_sz;

    dce_param.lsb_rand = job_cfg->dce_param.lsb_rand;
    dce_param.fovbound = job_cfg->dce_param.fov_setting.fovbound;
    dce_param.boundy = job_cfg->dce_param.fov_setting.boundy;
    dce_param.boundu = job_cfg->dce_param.fov_setting.boundu;
    dce_param.boundv = job_cfg->dce_param.fov_setting.boundv;
    dce_param.fovgain = job_cfg->dce_param.fov_setting.fovgain;
    dce_param.cent_x_s = job_cfg->dce_param.cent_x_s;
    dce_param.cent_y_s = job_cfg->dce_param.cent_y_s;
    dce_param.xdist_a1 = job_cfg->dce_param.xdist_a1;
    dce_param.ydist_a1 = job_cfg->dce_param.ydist_a1;
    dce_param.xofs_i = job_cfg->dce_param.xofs_i;
    dce_param.xofs_f = job_cfg->dce_param.xofs_f;
    dce_param.yofs_i = job_cfg->dce_param.yofs_i;
    dce_param.yofs_f = job_cfg->dce_param.yofs_f;
    dce_param.dewarp_in_width = job_cfg->in.crop.w;
    dce_param.dewarp_in_height = job_cfg->in.crop.h;
    dce_param.dewarp_out_width = job_cfg->in.crop.w;
    dce_param.dewarp_out_height = job_cfg->in.crop.h;

	dce_2d_lut_en = job_cfg->dce_param.dce_2d_lut_en;

    if (job_cfg->dce_param.dce_mode == VPE_DRV_DCE_2D_ONLY) {

		if (dce_2d_lut_en == 1) {
			if ((job_cfg->dce_param.out.w != 0) && (job_cfg->dce_param.out.h != 0)) {
				//set new dce out size
				dce_param.dewarp_out_width = job_cfg->dce_param.out.w;
				dce_param.dewarp_out_height = job_cfg->dce_param.out.h;
			}
		} else {

			if (job_cfg->dce_param.rot != VPE_DRV_DCE_ROT_NONE) {
				dce_param.lut2d_sz = DCE_2DLUT_TAB_IDX;

				if ((job_cfg->dce_param.rot == VPE_DRV_DCE_ROT_90) ||
					   (job_cfg->dce_param.rot == VPE_DRV_DCE_ROT_270) ||
					   (job_cfg->dce_param.rot == VPE_DRV_DCE_ROT_90_H_FLIP) ||
					   (job_cfg->dce_param.rot == VPE_DRV_DCE_ROT_270_H_FLIP)) {

					//set new dce out size
				    dce_param.dewarp_out_width = job_cfg->in.crop.h;
				    dce_param.dewarp_out_height = job_cfg->in.crop.w;
				}
				//set 2d lut tab & force dce_2d_lut_en = 1
				memset(lut2d->va_addr, 0, lut2d->size);
				vpe_drv_info("lut2d %lx %lx %x\n", (unsigned long)lut2d->va_addr, lut2d->pa_addr, lut2d->size);

				if (job_cfg->dce_param.rot != VPE_DRV_DCE_ROT_MANUAL) {
					rt = vpe_drv_dce_gen_rot_2dlut_fix(lut2d->va_addr, lut2d->size, job_cfg->in.crop, job_cfg->dce_param.rot);
				} else {
					rt = vpe_drv_dce_gen_rot_2dlut_deg(lut2d->va_addr, lut2d->size, job_cfg->in.crop, job_cfg->dce_param.rot_param);
				}
				if (rt < 0) {
					vpe_drv_err("lut2d rot(%d) (%d %d) (%d %d %d %d) apply fail\n", job_cfg->dce_param.rot, job_cfg->in.crop.w, job_cfg->in.crop.h,
										job_cfg->dce_param.rot_param.rad, job_cfg->dce_param.rot_param.flip,
										job_cfg->dce_param.rot_param.ratio_mode, job_cfg->dce_param.rot_param.ratio);
					return -1;
				}

#if (COHERENT_CACHE_BUF == 1)
		    	vos_cpu_dcache_sync((VOS_ADDR)lut2d->va_addr, lut2d->size, VOS_DMA_TO_DEVICE);
#endif
				job_cfg->dce_param.dce_2d_addr = lut2d->pa_addr;
				job_cfg->dce_param.dce_2d_ddr_id = lut2d->ddr_id;
				dce_2d_lut_en = 1;
			} else {
				if (job_cfg->dctg_param.dctg_en == 0) {
		        	vpe_drv_err("err 2dlut=0 2dlut_rot=0 dctg_en=0\n");
					return -1;
				}
			}
		}
    }
	vpe_drv_info("dewarp size (%d %d) to (%d %d)\r\n", dce_param.dewarp_in_width, dce_param.dewarp_in_height, dce_param.dewarp_out_width, dce_param.dewarp_out_height);

    vpe_eng_set_global_dce_size_buf_reg(p_eng_hdl, dce_param.dewarp_in_width, dce_param.dewarp_in_height);

    vpe_eng_set_dce_param_buf_reg(p_eng_hdl, &dce_param);

    if (job_cfg->dce_param.dce_mode != VPE_DRV_DCE_2D_ONLY) {
        for (i = 0; i < VPE_DRV_GEO_LUT_NUMS; i++) {
            geo_lut.geo_lut[i] = job_cfg->dce_param.geo_lut[i];
        }
        vpe_eng_set_dce_geo_lut_buf_reg(p_eng_hdl, &geo_lut);
    }

    if (job_cfg->dce_param.dce_mode != VPE_DRV_DCE_GDC_ONLY) {

		//2d lut addr
		if (job_cfg->dce_param.dce_2d_addr & (VPE_DRV_ALIGN_DMA_2DLUT - 1)) {
	        vpe_drv_err("2dlut addr need align(%d) error! (chip%d, ddr%d, pa 0x%lx)\n",
				VPE_DRV_ALIGN_DMA_2DLUT, p_eng_hdl->chip_id, job_cfg->dce_param.dce_2d_ddr_id, job_cfg->dce_param.dce_2d_addr);
			return -1;
		}

        pcie_paddr = vpe_drv_nvtpcie_get_pcie_addr(p_eng_hdl->chip_id, job_cfg->dce_param.dce_2d_ddr_id, job_cfg->dce_param.dce_2d_addr);
        vpe_eng_set_dma_dce_2dlut_addr_buf_reg(p_eng_hdl, pcie_paddr);
        vpe_eng_set_dc_2d_lut_load_enable_buf_reg(p_eng_hdl, dce_2d_lut_en);
    }
	return 0;
}

void vpe_drv_dce_dump_info(struct vpe_drv_dce_param *dce)
{
	const char *msg;

	msg = vpe_drv_uti_get_lut2d_rot_str(dce->rot);
	vpe_drv_log("dce_mode(%d) out(%d %d) %s\n", dce->dce_mode, dce->out.w, dce->out.h, VPE_DRV_UTI_CHK_MSG(msg));
	vpe_drv_log("manual rot(%d %d %d %d)\n", dce->rot_param.rad, dce->rot_param.flip, dce->rot_param.ratio_mode, dce->rot_param.ratio);
	vpe_drv_log("2d(en:%d sz:%d addr:0x%lx ddr_id:%d)\n", dce->dce_2d_lut_en, dce->lut2d_sz, dce->dce_2d_addr, dce->dce_2d_ddr_id);
	vpe_drv_log("fov(%d) yuvg(%d %d %d %d)\n", dce->fov_setting.fovbound, dce->fov_setting.boundy,
												dce->fov_setting.boundu, dce->fov_setting.boundv, dce->fov_setting.fovgain);
	vpe_drv_log("cen(%d %d) lens_r(%d)\n", dce->cent_x_s, dce->cent_y_s, dce->lens_r);
	vpe_drv_log("gdc x(%d %d %d) y(%d %d %d) lsb_rand:%d\n", dce->xdist_a1, dce->xofs_i, dce->xofs_f,
												 dce->ydist_a1, dce->yofs_i, dce->yofs_f, dce->lsb_rand);
}
#endif
