#include <comm/nvtmem.h>
#include <mach/fmem.h>

#include "kwrap/perf.h"

#include "kdrv_videoenc/kdrv_videoenc.h"

#include "kdrv_vdocdc_dbg.h"
#include "kdrv_vdocdc_api.h"

#include "h26x_common.h"
#include "h26xenc_api.h"
#include "h26x.h"

#include "sim_avc_main.h"
#include "sim_hevc_main.h"

#ifdef VDOCDC_SIM

int kdrv_vdocdc_api_wt_sim(unsigned char argc, char **argv)
{
	unsigned long input;

	if (argc != 1) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtoul (argv[0], 0, &input)) {
		nvt_dbg(ERR, "invalid input:%s\n", argv[0]);
		return -EINVAL;
	}

	nvt_dbg(INFO, "sim input %lu\n", input);

	kdrv_videoenc_open(0, KDRV_VIDEOCDC_ENGINE0);

	switch(input) {
		case 1:
			sim_avc_enc_main(352, 288, "/mnt/sd/src_yuv/352x288/canoa_352x288_220.pak");
			//sim_avc_enc_main(352, 288, "/mnt/sd/sde_352x288.dat");
			//sim_avc_enc_main(1920, 1080, "/mnt/sd/src_yuv/1920x1080/pedestrianarea_1920x1080_375.pak");
			//sim_avc_enc_main(3840, 2160, "/mnt/sd/src_yuv/3840x2160/IceRiver_3840x2160_30fps_420_8bit.pak");
			break;
		case 2:
			sim_avc_dec_main(352, 288, "/mnt/sd/rec_264.pak");
			break;
		case 3:
			sim_hevc_enc_main(352, 288, "/mnt/sd/src_yuv/352x288/foreman_352x288_400.pak");
			//sim_hevc_enc_main(3840, 2160, "/mnt/sd/src_yuv/3840x2160/IceRiver_3840x2160_30fps_420_8bit.pak");
			//sde
			//sim_hevc_enc_main(352, 288, "/mnt/sd/src_yuv/sde/sde_352x288.dat", input);
			//
			//sim_hevc_enc_main(1920, 1080, "/mnt/sd/src_yuv/1920x1080/pedestrianarea_1920x1080_375.pak");
			break;
		case 4:
			sim_hevc_dec_main(352, 288, "/mnt/sd/rec_265.pak");
			break;
		case 5:
			sim_avc_enc_main_pwm(3840, 2160, "/mnt/sd/src_yuv/3840x2160/IceRiver_3840x2160_30fps_420_8bit.pak");
			break;
		case 6:
			//sim_avc_enc_main_pwm(1920, 1080, "/mnt/sd/src_yuv/1920x1080/8M_g5.pak");
			sim_avc_enc_main_pwm(1920, 1080, "/mnt/sd/src_yuv/1920x1080/64mbps_outdoor_motion_v1_1920x1080.pak");
			break;
		case 7:
			sim_hevc_enc_main_pwm(3840, 2160, "/mnt/sd/src_yuv/3840x2160/IceRiver_3840x2160_30fps_420_8bit.pak");
			break;
		case 8:
			//sim_hevc_enc_main_pwm(1920, 1080, "/mnt/sd/src_yuv/1920x1080/8M_g5.pak");
			sim_hevc_enc_main_pwm(1920, 1080, "/mnt/sd/src_yuv/1920x1080/64mbps_outdoor_motion_v1_1920x1080.pak");
			break;
		default:
			nvt_dbg(ERR, "wt_sim intput(%ld) not support\r\n", input);
			break;
	}

	kdrv_videoenc_close(0, KDRV_VIDEOCDC_ENGINE0);

	return 0;
}
#else
int kdrv_vdocdc_api_wt_sim(unsigned char argc, char **argv)
{
	nvt_dbg(ERR, "VDOCDC_SIM not define, please check Makefile\r\n");
	return 0;
}
#endif

int kdrv_vdocdc_api_wt_dbg(unsigned char argc, char **argv)
{
	unsigned long input = 0;

	if (argc != 1) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtoul (argv[0], 0, &input)) {
		nvt_dbg(ERR, "invalid input:%s\n", argv[0]);
		return -EINVAL;
	}

	//g_rc_dump_log = (int)input;
#if H26X_SET_PROC_PARAM
    h26xEnc_setRCDumpLog((int)input);
#endif

	return 0;
}

int kdrv_vdocdc_api_wt_perf(unsigned char argc, char **argv)
{
	unsigned long input;
	
	static VOS_TICK tick_begin = 0, tick_end = 0;

	if (argc != 1) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtoul (argv[0], 0, &input)) {
		nvt_dbg(ERR, "invalid input:%s\n", argv[0]);
		return -EINVAL;
	}

	if (input == 0) {
		h26x_tick_close();
	}

	if (input == 1) {
		VOS_TICK duration[2];
		
		vos_perf_mark(&tick_end);
		duration[0] = vos_perf_duration(tick_begin, tick_end);
		duration[1] = h26x_tick_result();
		DBG_DUMP("tick : %d, %d, %d\r\n", duration[0], duration[1], (duration[1]*100)/duration[0]);
		vos_perf_mark(&tick_begin);
		h26x_tick_open();
	}

	return 0;
}
