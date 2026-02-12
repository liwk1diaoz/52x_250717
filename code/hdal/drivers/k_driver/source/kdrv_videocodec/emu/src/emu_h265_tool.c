#include "stdio.h" //
#include "stdlib.h"
#include "string.h"
#include "emu_h26x_common.h"

#include "emu_h265_enc.h"
#include "emu_h265_tool.h"
//#include "h26x_common.h"

//#if (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)

static const UINT32 prefix_mask[33] =
{
    0x00000000, 0x00000001, 0x00000003, 0x00000007,
    0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
    0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
    0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
    0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
    0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
    0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
    0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
    0xffffffff
};

// 352x288 [10] // 10
// 720x480 [9]  // 19
// 1280x720 [8] // 27
// 1920x1080[6] // 33
// 2560x720 [6] // 39
// 2592x2592 [6] // 45
// 2624x2624 [5] // 50
// 2880x2160 [5] // 55
// 3840x2160 [1] // 56
// 4096x2176 [4] // 60
// 2688x2688 [6] // 66
// 4096x2688 [3] // 69
// 5120x2688 [3] // 72


char gucH26XEncTestSrc_Cut[][2][128] = {


// 6 
{"1920x1080","sunflower_IPP"},
{"1920x1080","bluesky_IPP"},
{"1920x1080","pedestr_IPP"},
{"1920x1080","riverbed_IPP"},
{"1920x1080","rushhour_IPP"},
{"1920x1080","station_IPP"},

// 2
{"208x144","foreman_IPP"},
{"208x144","rdm208_IPP"},

// 2
{"240x320","foreman_IPP"},
{"240x320","rdm240_IPP"},

// 2
{"256x256", "IPP"},
{"256x256", "fps_1_2_IPP"},

{"4096x2160","Footage_IPP"},


#if 0


// 2
{"288x352","foreman_IPP"},
{"288x352","rdm288_IPP"},

// 2
{"272x144","foreman_IPP"},
{"272x144","rdm272_IPP"},




// 10
{"352x288","foreman_IPP"},
{"352x288","rdm352_IPP"},
{"352x288","akiyo_IPP"},
{"352x288","costg_IPP"},
{"352x288","hall_IPP"},
{"352x288","mobile_IPP"},
{"352x288","mother_IPP"},
{"352x288","silent_IPP"},
{"352x288","stefan_IPP"},
{"352x288","table_IPP"},

	// 2
	{"320x240","foreman_IPP"},
	{"320x240","rdm320_IPP"},

	// 9
    {"720x480","rdm720_IPP"},
    {"720x480","City2_IPP"},
    {"720x480","Crew_IPP"},
    {"720x480","NY111_IPP"},
    {"720x480","NY126_IPP"},
    {"720x480","Raven_IPP"},
    {"720x480","s3_IPP"},
    {"720x480","Shuttle_IPP"},
    {"720x480","Spincalendar_IPP"},


   {"480x720", "rdm480_IPP"},

	{"720x1280","parkrun50_IPP"},

	// 8
		{"1280x720","parkrun50_IPP"},
		{"1280x720","rdm_1280_IPP"},
	// 6
		{"1920x1080","bluesky_IPP"},
		{"1920x1080","pedestr_IPP"},
	// 1
    {"2048x1536","Kimono_2048x1536_IPP"},

// 2
{"2048x2048", "IPP"},
{"2048x2048", "fps_1_2_IPP"},
	//6
    {"2560x720","Chimei_2560_IPP"},
    {"2560x720","F65_IPP"},
	// 6
    {"2592x2592","AV_noise_IPP"},
    {"2592x2592","BQTerrace_IPP"},

#endif

#if 0
	 {"1920x1080","bluesky_IPP"},
	 {"1920x1080","bluesky_l_IPP"},
	 {"1920x1080","bluesky_r_IPP"},
	 {"1920x1080","bluesky_o_IPP"},
	 {"4096x2176","Violin_IPP"},
	 {"4096x2176","Violin_l_IPP"},
	 {"4096x2176","Violin_r_IPP"},
	 {"4096x2176","Violin_o_IPP"},
#endif





#if 0
	//7
	{"2688x2688","Fireworks_IPP"},
	{"2688x2688","Chimei_2688_IPP"},
    {"2688x2688","rdm_2688_IPP"},
    {"2688x2688","Traffic_2688_IPP"},
    {"2688x2688","People_2688x2688_IPP"},
    {"2688x2688","CrossRoad_IPP"},
    {"2688x2688","IceAerial_IPP"},
#endif


#if 0

{"1920x1080","Cactus_IPP"},
{"1920x1080","Cactus64_IPP"},
{"1920x1080","Cactus_sde_IPP"},
{"4096x2176","Chimei_IPP"},

#endif

	//{"1920x1080","Kimono1_IPP"},
#if 0
	// 29
	{"208x144","foreman_IPP"},
	{"240x320","foreman_IPP"},
	{"256x256", "IPP"},
	{"272x144","foreman_IPP"},
	{"288x352","foreman_IPP"},
	{"320x240","foreman_IPP"},
    {"352x288","foreman_IPP"},
	{"720x480","rdm720_IPP"},
	{"1280x720","parkrun50_IPP"},
	{"1920x1080","bluesky_IPP"},
	{"2048x1536","Kimono_2048x1536_IPP"},
	{"2048x2048", "IPP"},
	{"2560x720","Chimei_2560_IPP"},
	{"2592x2592","AV_noise_IPP"},
	{"2624x2624","pf8126_IPP"},
	{"2880x2160","Chimei2880_IPP"},
	{"2688x2688","Fireworks_IPP"},
	{"3840x2160","Chimei3840_IPP"},
	{"4096x2176","Footage_IPP"},
	{"4096x2688","Kimono4096x2688_IPP"},
	{"5120x2688","Cactus5120x2688_IPP"},
	{"4096x4096", "Chimei4096_IPP"},
	{"5120x144", "Break_IPP"},
	{"5120x5120", "Chimei5120_IPP"},
	{"480x720", "rdm480_IPP"},
	{"720x1280","parkrun50_IPP"},
	{"1080x1920","bluesky_IPP"},
	{"2160x3840","Chimei2160_IPP"},
	{"2176x4096","Footage_IPP"},
#else


#if 0
	{"3840x2160","Bridge_IPP"},
	{"3840x2160","Bridge64_IPP"},
	{"4096x2160","Rally_IPP"},
	{"4096x2160","Rally64_IPP"},
	{"4096x2160","Water_IPP"},
	{"4096x2160","Water64_IPP"},

#endif

#if 0
	{"3840x2160","Aerial_IPP"},
	{"3840x2160","Bridge_IPP"},
	{"3840x2160","Day_IPP"},
	{"3840x2160","Metro_IPP"},
	{"3840x2160","Night_IPP"},
	{"3840x2160","Square_IPP"},

#endif





#if 1
	// 30
	{"208x144","foreman_IPP"},//0
	{"240x320","foreman_IPP"},
	{"256x256", "fps_1_2_IPP"},
	{"272x144","foreman_IPP"},
	{"288x352","foreman_IPP"},
	{"320x240","foreman_IPP"},//5
	{"352x288","foreman_IPP"},
	{"720x480","s3_IPP"},
	{"1280x720","soccer_IPP"},
	{"1920x1080","sunflower_IPP"},
	{"2048x1536","Kimono_2048x1536_IPP"},//10
	{"2048x2048", "fps_1_2_IPP"},
	{"2560x720","Chimei_2560_IPP"},
	{"2592x2592","AV_noise_IPP"},
	{"2624x2624","rally_IPP"},
	{"2880x2160","Chimei2880_IPP"},//15
	{"2688x2688","Fireworks_IPP"},
	{"3840x2160","Chimei3840_IPP"},
	{"4096x2176","Footage_IPP"},
	{"4096x2688","Kimono4096x2688_IPP"},
	{"5120x2688","Cactus5120x2688_IPP"},//20
	{"4096x4096", "Chimei4096_IPP"},
	{"5120x144", "Break_IPP"},
	{"5120x5120", "Chimei5120_IPP"},
	{"480x720", "rdm480_IPP"},
	{"720x1280","parkrun50_IPP"},//25
	{"1080x1920","bluesky_IPP"},
	{"2160x3840","Chimei2160_IPP"},
	{"2176x4096","Footage_IPP"},
	{"4096x2160","Footage_IPP"},
#endif

#if 0
// 2
{"208x144","foreman_IPP"},
{"208x144","rdm208_IPP"},
#endif

#if 0

// 6
{"1920x1080","sunflower_IPP"},
{"1920x1080","bluesky_IPP"},
{"1920x1080","pedestr_IPP"},
{"1920x1080","riverbed_IPP"},
{"1920x1080","rushhour_IPP"},
{"1920x1080","station_IPP"},
#endif



#if 0


// 5
{"2880x2160","Chimei2880_IPP"},
{"2880x2160","Break_IPP"},
{"2880x2160","Cat_IPP"},
{"2880x2160","Lupe_IPP"},
{"2880x2160","jvcGy2880_IPP"},


#endif


	// 4
#if 0
	{"3840x2160","Chimei3840_IPP"},
	{"4096x2160","Footage_IPP"},
	{"4096x2160","Chimei_IPP"},
	{"4096x2160","Surf_IPP"},
	{"4096x2160","Violin_IPP"},
	{"5120x5120", "Chimei5120_IPP"},
#endif



#endif


	// 2
	{"208x144","foreman_IPP"},
	{"208x144","rdm208_IPP"},

	// 2
    {"272x144","foreman_IPP"},
    {"272x144","rdm272_IPP"},

	// 2
    {"272x272","foreman_IPP"},
    {"272x272","rdm272_IPP"},

	// 10
    {"352x288","foreman_IPP"},
    {"352x288","rdm352_IPP"},
    {"352x288","akiyo_IPP"},
    {"352x288","costg_IPP"},
    {"352x288","hall_IPP"},
    {"352x288","mobile_IPP"},
    {"352x288","mother_IPP"},
    {"352x288","silent_IPP"},
    {"352x288","stefan_IPP"},
    {"352x288","table_IPP"},

	// 9
    {"720x480","rdm720_IPP"},
    {"720x480","City2_IPP"},
    {"720x480","Crew_IPP"},
    {"720x480","NY111_IPP"},
    {"720x480","NY126_IPP"},
    {"720x480","Raven_IPP"},
    {"720x480","s3_IPP"},
    {"720x480","Shuttle_IPP"},
    {"720x480","Spincalendar_IPP"},

	// 8
    {"1280x720","parkrun50_IPP"},
    {"1280x720","rdm_1280_IPP"},
    {"1280x720","Knight_IPP"},
    {"1280x720","mobcal_IPP"},
    {"1280x720","SANYO011_IPP"},
    {"1280x720","shields50_IPP"},
    {"1280x720","soccer_IPP"},
    {"1280x720","Stockholm1_IPP"},

	// 6
    {"1920x1080","bluesky_IPP"},
    {"1920x1080","pedestr_IPP"},
    {"1920x1080","riverbed_IPP"},
    {"1920x1080","rushhour_IPP"},
    {"1920x1080","station_IPP"},
    {"1920x1080","sunflower_IPP"},

	// 1
    {"2048x1536","Kimono_2048x1536_IPP"},

	// 2
	{"2048x2048", "IPP"},
	{"2048x2048", "fps_1_2_IPP"},

	//6
    {"2560x720","Chimei_2560_IPP"},
    {"2560x720","F65_IPP"},
    {"2560x720","Footage_IPP"},
    {"2560x720","bf3_IPP"},
    {"2560x720","jvcGy_IPP"},
    {"2560x720","March_IPP"},

	// 6
    {"2592x2592","AV_noise_IPP"},
    {"2592x2592","BQTerrace_IPP"},
    {"2592x2592","ducks_IPP"},
    {"2592x2592","horsecab_IPP"},
    {"2592x2592","in_to_tree_IPP"},
    {"2592x2592","smile_IPP"},

	// 5
    {"2624x2624","pf8126_IPP"},
    {"2624x2624","rally_IPP"},
    {"2624x2624","SteamLoco_IPP"},
    {"2624x2624","Traffic_IPP"},
    {"2624x2624","waterskiing_IPP"},

	// 5
    {"2880x2160","Chimei2880_IPP"},
    {"2880x2160","Break_IPP"},
    {"2880x2160","Cat_IPP"},
    {"2880x2160","Lupe_IPP"},
    {"2880x2160","jvcGy2880_IPP"},

	// 6
    //{"3840x2160","Chimei3840_IPP"},
	{"3840x2160","Aerial_IPP"},
	{"3840x2160","Bridge_IPP"},
	{"3840x2160","Day_IPP"},
	{"3840x2160","Metro_IPP"},
	{"3840x2160","Night_IPP"},
	{"3840x2160","Square_IPP"},

	// 4
	#if 1
    {"4096x2176","Footage_IPP"},
    {"4096x2176","Chimei_IPP"},
    {"4096x2176","Surf_IPP"},
    {"4096x2176","Violin_IPP"},
	#endif

	//7
	{"2688x2688","Fireworks_IPP"},
	{"2688x2688","Chimei_2688_IPP"},
    {"2688x2688","rdm_2688_IPP"},
    {"2688x2688","Traffic_2688_IPP"},
    {"2688x2688","People_2688x2688_IPP"},
    {"2688x2688","CrossRoad_IPP"},
    {"2688x2688","IceAerial_IPP"},

	// 3
    {"4096x2688","Kimono4096x2688_IPP"},
    {"4096x2688","ParkScene4096x2688_IPP"},
    {"4096x2688","toys4096x2688_IPP"},

	// 2
	{"5120x144", "Break_IPP"},
	{"5120x144", "Lupe_IPP"},

	// 3
    {"5120x2688","Cactus5120x2688_IPP"},
    {"5120x2688","PeopleOnStreet5120x2688_IPP"},
    {"5120x2688","rdm5120x2688_IPP"},

	// 4
	{"5120x5120", "Chimei5120_IPP"},
	{"5120x5120", "Footage5120_IPP"},
	{"5120x5120", "Surf5120_IPP"},
	{"5120x5120", "Violin5120_IPP"},


};


UINT8 item_name[H26X_REPORT_NUMBER][256] = {
	"CYCLE",
	"REC_CHKSUM",
	"H26X_SRC_Y_DMA_CHKSUM",
	"H26X_SRC_C_DMA_CHKSUM",
	"TNR_OUT_Y_CHKSUM",
	"SCD_REPORT",
	"BS_LEN",
	"BS_CHKSUM",
	"QP_CHKSUM",
	"ILF_DIS_CTB",
	"RRC_BIT_LEN",
	"RRC_RDOPT_COST_LSB",
	"RRC_RDOPT_COST_MSB",
	"RRC_SIZE",
	"RRC_FRM_COST_LSB",
	"RRC_FRM_COST_MSB",
	"RRC_FRM_COMPLEXITY_LSB",
	"RRC_FRM_COMPLEXITY_MSB",
	"RRC_COEFF",
	"RRC_QP_SUM",
	"RRC_SSE_DIST_LSB",
	"RRC_SSE_DIST_MSB",
	"SRAQ_ISUM_ACT_LOG",
	"PSNR_FRM_Y_LSB",
	"PSNR_FRM_Y_MSB",
	"PSNR_FRM_U_LSB",
	"PSNR_FRM_U_MSB",
	"PSNR_FRM_V_LSB",
	"PSNR_FRM_V_MSB",
	"PSNR_ROI_Y_LSB",
	"PSNR_ROI_Y_MSB",
	"PSNR_ROI_U_LSB",
	"PSNR_ROI_U_MSB",
	"PSNR_ROI_V_LSB",
	"PSNR_ROI_V_MSB",
	"IME_CHKSUM_LSB",
	"IME_CHKSUM_MSB",
	"EC_CHKSUM",
	"SIDE_INFO_CHKSUM",
	"ROI_CNT",
	"CRC_HIT_Y_CNT",
	"CRC_HIT_C_CNT",
	"STATS_INTER_CNT",
	"STATS_SKIP_CNT",
	"STATS_MERGE_CNT",
	"STATS_IRA4_CNT",
	"STATS_IRA8_CNT",
	"STATS_IRA16_CNT",
	"STATS_IRA32_CNT",
	"STATS_CU64_CNT",
	"STATS_CU32_CNT",
	"STATS_CU16_CNT",
	"STATS_SCD_INTER_CNT",
	"STATS_SCD_IRANG_CNT",
    "H26X_JND_Y_CHKSUM",
    "H26X_JND_C_CHKSUM",
    "H26X_OSG_0_Y_CHKSUM",
    "H26X_OSG_0_C_CHKSUM",
    "H26X_OSG_Y_CHKSUM",
    "H26X_OSG_C_CHKSUM",
	"RRC_COEFF2",
	"RRC_COEFF3",
};
UINT8 item_name_2[H26X_REPORT_2_NUMBER][256] = {
    "PSNR_MOTION_Y_LSB",
    "PSNR_MOTION_Y_MSB",
    "PSNR_MOTION_U_LSB",
    "PSNR_MOTION_U_MSB",
    "PSNR_MOTION_V_LSB",
    "PSNR_MOTION_V_MSB",
    "PSNR_BGR_Y_LSB",
    "PSNR_BGR_Y_MSB",
    "PSNR_BGR_U_LSB",
    "PSNR_BGR_U_MSB",
    "PSNR_BGR_V_LSB",
    "PSNR_BGR_V_MSB",
    "MOTION_NUM",
    "BGR_NUM",
    "JND_GRAD",
    "JND_GRAD_CNT",
};

void h26xMemSetAddr(UINT32 *puiVirAddr,UINT32 *puiVirMemAddr,UINT32 uiSize){
    //*puiVirAddr = *puiVirMemAddr; *puiVirMemAddr = *puiVirMemAddr + uiSize;
#if H26X_EMU_DRAM_2
	BOOL switch_dram = rand()%2;
	*puiVirAddr = switch_dram ?  (*puiVirMemAddr < 0x80000000 ? (*puiVirMemAddr + 0x40000000) : (*puiVirMemAddr - 0x40000000)) : *puiVirMemAddr;
	*puiVirMemAddr = *puiVirMemAddr + (((uiSize + 31)/32)*32);
#else
    *puiVirAddr = *puiVirMemAddr; *puiVirMemAddr = *puiVirMemAddr + (((uiSize + 31)/32)*32);
#endif

	//memset(puiVirAddr, 0, uiSize);
	//h26x_flushCache((unsigned int)puiVirAddr, SIZE_32X(uiSize));
}


void h26xMemSet(UINT32 uiMemAddr,UINT32 uiVal,UINT32 uiSize){
    UINT8 *pucMemAddr;
    UINT32 i;

    pucMemAddr = (UINT8 *)uiMemAddr;

    for (i=0;i<uiSize;i++){
        *(pucMemAddr + i) = uiVal;
    }
}

void h26xMemCpy(UINT32 uiSrcMemAddr,UINT32 uiDesMemAddr,UINT32 uiSize){
    UINT8 *pucSrcMemAddr,*pucDesMemAddr;
    UINT32 i;

    pucSrcMemAddr = (UINT8 *)uiSrcMemAddr;
    pucDesMemAddr = (UINT8 *)uiDesMemAddr;

    for (i=0;i<uiSize;i++){
        *(pucDesMemAddr + i) = *(pucSrcMemAddr + i);
    }
}

#if 0
static unsigned int h26x_randseed = 1;

void h26xSrand(unsigned int seed)
{
    h26x_randseed = seed;
}
int h26xRand(void)
{
    register int x = h26x_randseed;
    register int t;

    t = x + (x << 23) + (x >> 3) + 12345;

    if (t <= 0)
        t += 0x7FFFFFFF;
    h26x_randseed = t;

	DBG_INFO("%s %d,t = %d  \r\n",__FUNCTION__,__LINE__,(int)t);


    return (t);
}
#else
void h26xSrand(unsigned int seed)
{
	srand(seed);
}
int h26xRand(void)
{
	return rand();
}
#endif

void h26xRotate(UINT32 x, UINT32 y, UINT32 w, UINT32 h, INT32 angle, UINT32 *new_x, UINT32 *new_y)
{
    if(angle == 0){
		*new_y = y;
		*new_x = x;
    }else if(angle == -90)
	{
		*new_y = w - x;
		*new_x = y;
	}else if(angle == 90)
	{
		*new_y = x;
		*new_x = h - y;
	}else{
		*new_y = h - y;
		*new_x = w - x;
    }
}

void h26xEmuRotateSrc(H26XFile *pH26XSrcFile,UINT8 *ucHwSrcAddr,UINT8 *ucEmuSrcAddr,UINT32 uiSrcSize,
                    UINT32 uiWidth,UINT32 uiHeight,UINT32 uiLineOffset,UINT32 uiMaxBuf, INT32 iAngle, UINT32 isY, UINT32 write_out, UINT32 uv_swap){
    UINT32 i,j,uiUseSize;
    UINT32 x_offset = 0,y_offset = 0;
	UINT32 ui16XWidth =  uiWidth == 1080 ? 1088 : uiWidth; //SIZE_16X(uiWidth);

	//DBG_ERR("uiWidth=%d ui16XWidth=%d uiLineOffset=%d\r\n",(int)uiWidth,(int)ui16XWidth,(int)uiLineOffset);


    if(write_out == 0){
        h26xFileSeek(pH26XSrcFile, uiSrcSize, H26XF_SET_CUR);
        return;
    }
    if(isY)
    {
        uiUseSize = (iAngle == 180 || iAngle == 0)? uiHeight: ui16XWidth;
        uiUseSize *= (uiLineOffset);
        if(uiUseSize > uiMaxBuf)
        {
            DBG_ERR("Error at %s ! uiUseSize = 0x%08x,malloc = 0x%08x\r\n",__FUNCTION__,(int)uiUseSize,(int)uiMaxBuf);
        }
    }else{
        uiUseSize = (iAngle == 180 || iAngle == 0)? uiHeight: (ui16XWidth/2);
        uiUseSize *= (uiLineOffset);
        if(uiUseSize > uiMaxBuf)
        {
            DBG_ERR("Error at %s ! uiUseSize = 0x%08x,malloc = 0x%08x\r\n",__FUNCTION__,(int)uiUseSize,(int)uiMaxBuf);
        }
    }

    if (ui16XWidth == uiLineOffset && iAngle == 0 && uv_swap == 0)
    {
        h26xFileReadFlush(pH26XSrcFile,(uiSrcSize),(UINT32)ucHwSrcAddr);
    }else if(iAngle == 0 && uv_swap == 0){

        UINT32 h;
        INT32 flush_size;

        for (h = 0; h < uiHeight; h++){
            h26xFileRead(pH26XSrcFile,(ui16XWidth),(UINT32)(ucHwSrcAddr + (h*(uiLineOffset))));
			//h26x_flushCache((UINT32)(ucHwSrcAddr + (h*(uiLineOffset))),SIZE_32X(ui16XWidth));
        }
		h26x_flushCache((UINT32)(ucHwSrcAddr),(ui16XWidth*uiHeight));


        flush_size = uiSrcSize - (uiHeight*ui16XWidth);
        if(flush_size > 0) h26xFileSeek(pH26XSrcFile, flush_size, H26XF_SET_CUR);

    }else{
        h26xFileReadFlush(pH26XSrcFile,(uiSrcSize),(UINT32)ucEmuSrcAddr);
        if(isY)
        {
            // Luma
        	for(j = 0; j < uiHeight; j++)
        	{
        		for(i = 0; i< uiWidth; i++)
        		{

        			UINT32 c,d,new_x = 0,new_y = 0;
        			h26xRotate( i, j, uiWidth-1, uiHeight-1, iAngle, &new_x, &new_y);
                    c = ui16XWidth*j + i;
                    d = (new_y + y_offset) * uiLineOffset + new_x + x_offset;
        			ucHwSrcAddr[d] = ucEmuSrcAddr[c];
        		}
        	}

        }else{
            // Chroma
        	for(j = 0; j < uiHeight; j++)
        	{
        		for(i = 0; i< uiWidth/2; i++)
        		{

        			UINT32 c,d,new_x = 0,new_y = 0;
        			h26xRotate( i, j, (uiWidth/2)-1, uiHeight-1, iAngle, &new_x, &new_y);
        			c = ui16XWidth*j + (i*2);
                    //d = (new_y + (y_offset / 2)) * uiLineOffset + (new_x*2) + x_offset;
                    d = ((new_y*2 + y_offset) * uiLineOffset / 2) + (new_x*2) + x_offset;
                    if(uv_swap == 0){
                        ucHwSrcAddr[d] = ucEmuSrcAddr[c];
                        ucHwSrcAddr[d+1] = ucEmuSrcAddr[c+1];
                    }else{
                        ucHwSrcAddr[d+1] = ucEmuSrcAddr[c];
                        ucHwSrcAddr[d] = ucEmuSrcAddr[c+1];
                    }
        		}
        	}
        }
    }
    h26x_flushCache((UINT32)ucHwSrcAddr, SIZE_32X(uiUseSize));

    //emuh26x_msg("  0x%08x\r\n",uiHwSrcAddr);
}

UINT8 emu_bit_reverse(UINT8 pix)
{
    INT32 i;
    INT8 temp_pix=0;
    for(i=7;i>=0;i--)
    {
        temp_pix|=((pix>>i)&prefix_mask[1])<<(7-i);
    }

    if(pix&0xffffff00)
    {
        DBG_ERR("warning: pix value is over 8 bit\n");
        while(1);
    }
    return temp_pix;
}

UINT32 emu_buf_chk_sum(UINT8 *buf, UINT32 size, UINT32 format)
{
	UINT32 i;
	UINT32 chk_sum = 0;

	for (i = 0; i < size; i++)
		chk_sum += (format) ? emu_bit_reverse(*(buf + i)) : *(buf + i);

	return chk_sum;
}

void h26xSwapRecAndRefIdx(UINT32 *a, UINT32 *b)
{
    UINT32 c;
    c = *a;
    *a = *b;
    *b = c;
}

#define DEBUG_WORKINGBUF_LEN    512  // from Debug.c

void h26xPrintString(UINT8 *s, INT32 iLen)
{
    //debug_msg only print string length < DEBUG_WORKINGBUF_LEN
    //if we want to print very long string, we need to call it many times

    DBG_INFO("\r\n");

    while(iLen > 0)
    {
    	UINT8 ss[DEBUG_WORKINGBUF_LEN];
		strncpy ( (char *)ss, (char *)s, DEBUG_WORKINGBUF_LEN );
        DBG_INFO("%s", ss);
        iLen -= DEBUG_WORKINGBUF_LEN;
        s += (DEBUG_WORKINGBUF_LEN);
    }
    DBG_INFO("\r\n");

}

//#endif //if (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)

