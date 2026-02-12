
#include <common.h>
#include <command.h>
#include <asm/byteorder.h>
#include <asm/io.h>
#include <part.h>
#include <asm/hardware.h>
#include <asm/nvt-common/nvt_types.h>
#include <asm/nvt-common/nvt_common.h>
#include <asm/nvt-common/shm_info.h>
#include <stdlib.h>
#include <linux/arm-smccc.h>
//#include "cmd_bootlogo.h"
//#include <asm/arch/display.h>
//#include <asm/arch/top.h>
#include <linux/libfdt.h>
#include "asm/arch/lcd310.h"
#include "asm/arch/lcd210.h"
#include "asm/arch/hdmi_if.h"


#include "logo_hdmi.dat"   //jpg bitstream binary

//#define DEMO_BOOT_LOGO_JPEG

#ifdef DEMO_BOOT_LOGO_JPEG
//extern void jpeg_setfmt(unsigned int fmt);
extern void jpeg_decode_central(unsigned int bg_w, unsigned int bg_h, unsigned char* inbuf, unsigned char* outbuf);
extern void jpeg_getdim(unsigned int* width, unsigned int* height);
#endif

/*---------------------------------------------------------------
 * Configurable
 *---------------------------------------------------------------
 */
/* logo base */
#define FRAME_BUFFER_BASE	     0x10000000   //configurable
#define FRAME_BUFFER_BASE_LCD210 0x10800000   //configurable
#define FRAME_BUFFER_SIZE	     (8 << 20)    //8M, configurable
#define FRAME_BUFFER_SIZE_LCD210 (1 << 20)    //1M, configurable
#define FRAME_BUFFER_BASE_JPEG   0x10900000


#ifdef DEMO_BOOT_LOGO_JPEG
static lcd_output_t g_output[1] = {OUTPUT_1920x1080};   //configurable
static lcd_vin_t    g_vin[1] = {VIN_1920x1080};         //configurable
static input_fmt_t  g_infmt[1] = {INPUT_FMT_YUV422 /* INPUT_FMT_RGB565 */};    //configurable
#else
static lcd_output_t g_output[1] = {OUTPUT_1280x720};   //configurable
static lcd_vin_t    g_vin[1] = {VIN_1024x768};         //configurable
static input_fmt_t  g_infmt[1] = {INPUT_FMT_ARGB1555 /* INPUT_FMT_RGB565 */};    //configurable
#endif

static u32 lcd_frame_base[1] = {FRAME_BUFFER_BASE};
static u32 lcd210_frame_base[1] = {FRAME_BUFFER_BASE_LCD210};


extern UINT32        otp_key_manager(UINT32 rowAddress);
extern BOOL  extract_trim_valid(UINT32 code, UINT32 *pData);
static int do_bootlogo(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
    int hdmi = 0, cvbs = 0;
	BOOL	 is_found;
	UINT32	 code;
	UINT32	 hdmi_trim;

    if (argc == 2) {
        char *p = argv[1];
        if (!strcmp("hdmi", p))
        	hdmi = 1;
        else if (!strcmp("cvbs", p))
        	cvbs = 1;
        else {
            printf("Usage: bootlogo {hdmi|cvbs} \n");
            return -1;
        }
    }
	
	if (hdmi) {
		code = otp_key_manager(4); // PackageUID addr[4] bit [11] = 1 HDMI(X)	bit[11] = 0 HDMI(O)

		if(code == 0) {
			
		} else if(code == -33) {
			printf("Read package UID error\r\n");
			hdmi = 0;
		} else {
			hdmi_trim = 0;
			is_found = extract_trim_valid(code, &hdmi_trim);
			if(is_found) {
				if(((hdmi_trim & (1<<11)) == (1<<11))) {
					printf("Failed to init hdmi, chip no support feature\n");
					hdmi = 0;
					return -1;
				} 
			} else {
				printf("Read package UID = 0x%x(NULL)\r\n", (uint)hdmi_trim);
			}
		}
	}

#ifdef DEMO_BOOT_LOGO_JPEG    
	{
		#include "logo_hdmi.dat"

		unsigned char *outbuf;	
		unsigned int img_width, img_height;
		unsigned char *inbuf;

		inbuf = (unsigned char*)FRAME_BUFFER_BASE_JPEG;
		outbuf = (unsigned char*)FRAME_BUFFER_BASE; 			
		printf("start JPEG decode\n");			
		dcache_enable();
		//jpeg_decode(inbuf, outbuf);
		jpeg_decode_central(1920, 1080, inbuf, outbuf);
		jpeg_getdim(&img_width, &img_height);
		printf("image size: %d x %d\n", img_width, img_height);
		dcache_disable();
		printf("end\n");	
	}
#endif

    flcd_main(LCD0_ID, g_vin[LCD0_ID], g_output[LCD0_ID], g_infmt[LCD0_ID], lcd_frame_base[LCD0_ID]);
#if 1
    /* this only for lcd0 */
    if (hdmi) {
		hdmi_video_t hdmi_video;

        switch (g_output[LCD0_ID]) {
		case OUTPUT_1280x720:
			hdmi_video = HDMI_VIDEO_720P;
			break;
		case OUTPUT_1920x1080:
			hdmi_video = HDMI_VIDEO_1080P;
			break;
		case OUTPUT_1024x768:
			hdmi_video = HDMI_VIDEO_1024x768;
			break;
		default:
			return -1;
			break;
		}
		hdmi_if_init(hdmi_video);
    }
#endif
	if (cvbs) {
		flcd210_main(cvbs, FRAME_BUFFER_BASE_LCD210);
	}

	return 0;
}



U_BOOT_CMD(
	bootlogo,	2,	1,	do_bootlogo,
	"show lcd bootlogo [hdmi|cvbs]",
	"no argument means VGA output only, \n"
	"[hdmi] - VGA and HDMI output simulataneously"
);

