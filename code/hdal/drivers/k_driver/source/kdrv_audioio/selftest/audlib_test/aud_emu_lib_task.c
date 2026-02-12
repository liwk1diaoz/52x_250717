#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/io.h>
#include <linux/of_device.h>
#include <linux/kdev_t.h>
#include <linux/clk.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/random.h>
#include <asm/signal.h>

#include "audlib_emu_drv.h"
#include "audlib_emu_main.h"
#include "audlib_emu_dbg.h"

#include "../../../kdrv_builtin/audio/dai/include/dai.h"
#include "Audio.h"
#include "kdrv_audioio/audlib_agc.h"
#include "kdrv_audioio/audlib_aec.h"
#include "kdrv_audioio/audlib_src.h"
#include "kdrv_audioio/audlib_anr.h"
#include "kdrv_audioio/audlib_aac.h"
#include "kdrv_audioio/audlib_g711.h"
#include "kdrv_audioio/audlib_adpcm.h"
#include "kdrv_audioio/kdrv_audioio.h"
//#include "audio/aud_aacd_api.h" //redefinition of struct, extern the decode api to use
//#include "frammap/frammap_if.h"

#define TEST_LIB 1

#define emu_aec_msg printk
#define AEC_PRELOAD_FORE_BUFSIZE  0x4000 // 48KHz Stereo case.sizeof(short)
#define AEC_PRELOAD_BACK_BUFSIZE  0x8000 // 48KHz Stereo case.sizeof(int)
#define EMU_AUDAEC_TESTSIZE       0x100000 // 32MB
#define EMU_AUDLIB_BUFSIZE        0x20000

static BOOL dma_done_rx=0,dma_done_tx=0,dma_done_txlb=0,dma_done_cnt=0;
static UINT32 audio_test_ext_codec = 0;
static UINT32 uiAecPreForeAddr,uiAecPreBackAddr,uiAudAecPlyBufAdr,uiAudAecRecBufAdr,uiAudAecOutBufAdr,AecLibBufAddr;
UINT32 *main_buffer;

#if TEST_LIB
struct ANR_CONFIG gstemuANR;
ADPCM_STATE            adpcm_state;


typedef struct
{
    UINT32 SampleRate;
    UINT32 Mic_CH;
    UINT32 Spk_CH;
    BOOL   bNoiseSuppress_EN;
    INT32  NoiseSuppress_LVL;
    INT32  EchoCancel_LVL;
    BOOL   bNLP_EN;
    BOOL   bAR_EN;
    BOOL   bPreload_EN;
    UINT32 FilterLength;
    UINT32 FrameSize;

}AEC_AUTOTEST_SETTING,*PAEC_AUTOTEST_SETTING;
#endif

#if 1
INT32 reserve_buf_test(UINT32 phy_addr)
{
   // emu_aec_msg("reserve_buf_test callback       =  [%x]\r\n",phy_addr);
    return 0;
}

INT32 free_buf_test(UINT32 phy_addr)
{
   // emu_aec_msg("free_buf_test callback          =  [%x]\r\n",phy_addr);
    return 0;
}
INT32 kdrv_cb_test(void *cb_info, void *user_data)
{

    emu_aec_msg("<<%d>> kdrv_cb_test callback,  cb_info =  [%x]  user_data = [%d]\r\n",dma_done_cnt/3+1,*(UINT32 *)cb_info,*(UINT32 *)user_data);
    dma_done_cnt++;
    if(*(UINT32 *)cb_info == 0)
        dma_done_rx = 1;
    else if (*(UINT32 *)cb_info == 2)
        dma_done_tx = 1;
    else if (*(UINT32 *)cb_info == 1)
        dma_done_txlb =1;
    return 0;
}
#endif

#if 1
int testcode_read_file(CHAR* file_name, CHAR* addr)
{
    struct 	file* 	file_handle;
    int 			error = 0;
    int             file_size;
    struct 	kstat 	stat;

    file_handle = filp_open(file_name, O_RDONLY, 0);
	if (IS_ERR(file_handle)) {
		DBG_ERR("unable to open backing file: %s\n", file_name);
		return 0;
	}
	error = vfs_stat(file_name, &stat);
	if (error) {
		DBG_ERR("%s: Failed to stat file %s \n", __FUNCTION__, file_name);
		return 0;
	}

	file_size = kernel_read(file_handle,(char *)addr, stat.size, &file_handle->f_pos);
	filp_close(file_handle, NULL);

	emu_aec_msg("file %s size is 0x%08X  \r\n", file_name, file_size);

    return file_size;
}

void testcode_write_file(CHAR* file_name, CHAR* addr,UINT32 file_size)
{
    struct 	file* 	file_handle;
    loff_t			file_offset_tmp;

    file_offset_tmp = 0;

	file_handle = filp_open(file_name, O_RDWR | O_LARGEFILE | O_CREAT, 0);
	if (IS_ERR(file_handle)) {
		DBG_ERR("unable to open backing file: %s\n", file_name);
	}

	file_offset_tmp = 0;
	kernel_write(file_handle, (char __user *)addr, (UINT32)file_size, &file_offset_tmp);

	vfs_fsync(file_handle, 1);
	filp_close(file_handle, NULL);

}


void test_buf_init(void)
{
	/*
    frammap_buf_t       aec_test_buff = {0};

    aec_test_buff.size = 0x400000;
    aec_test_buff.align = 64;      ///< address alignment
    aec_test_buff.name = "nvtmpp";
    aec_test_buff.alloc_type = ALLOC_CACHEABLE;
    frm_get_buf_ddr(DDR_ID0, &aec_test_buff);
    */


	main_buffer = kmalloc(0x400000, GFP_KERNEL);
	if (main_buffer == NULL){
		emu_aec_msg("init buffer error\r\n");
	}
	AecLibBufAddr = (UINT32)main_buffer;

	uiAudAecRecBufAdr = AecLibBufAddr + EMU_AUDLIB_BUFSIZE;

    uiAudAecPlyBufAdr = uiAudAecRecBufAdr+EMU_AUDAEC_TESTSIZE;
    uiAudAecOutBufAdr = uiAudAecPlyBufAdr+EMU_AUDAEC_TESTSIZE;

    uiAecPreForeAddr  = uiAudAecOutBufAdr+EMU_AUDAEC_TESTSIZE;
    uiAecPreBackAddr  = uiAecPreForeAddr +AEC_PRELOAD_FORE_BUFSIZE;
    emu_aec_msg("AecLibBufAddr     = [%x]\r\n",AecLibBufAddr);
    emu_aec_msg("uiAudAecRecBufAdr = [%x]\r\n",uiAudAecRecBufAdr);
    emu_aec_msg("uiAudAecPlyBufAdr = [%x]\r\n",uiAudAecPlyBufAdr);
    emu_aec_msg("uiAudAecOutBufAdr = [%x]\r\n",uiAudAecOutBufAdr);
	emu_aec_msg("uiAecPreForeAddr  = [%x]\r\n",uiAecPreForeAddr);
	emu_aec_msg("uiAecPreBackAddr  = [%x]\r\n",uiAecPreBackAddr);

}
#endif

#if TEST_LIB
int GetSpecLen(int SR)
{
    int Len=0;

    switch (SR)
    {
        case 8000:          Len = 129;            break;
        case 11025:         Len = 129;            break;
        case 16000:         Len = 257;            break;
        case 22050:         Len = 257;            break;
        case 32000:         Len = 513;            break;
        case 44100:         Len = 513;            break;
        case 48000:         Len = 513;            break;
        default:
            break;
    }
    return Len;
}


int config_anr_test(int BlkSzW, int sampling_rate, int Channels, struct ANR_CONFIG * sANR)
{
    int i,SpecLen,tone_min_time;


    // ===========================================================================
    // Note: Replace the following 4 lines by the result of audlib_anr_detect()
    unsigned short default_spec = 0; //unsigned short default_spec[513] = {0}; to prevent warning
    sANR->max_bias = 106;

    sANR->default_bias = 64;

    sANR->default_eng  = 0;

    // ===========================================================================

    // User Configurations
    sANR->blk_size_w     = BlkSzW;
    sANR->sampling_rate = sampling_rate;

    sANR->stereo= Channels;

    sANR->nr_db         = 15;        // 3~35 dB, The magnitude suppression in dB value of Noise segments

    sANR->hpf_cutoff_freq      = 70; // Hz

    sANR->bias_sensitive       = 1;  // 1(non-sensitive)~9(very sensitive)


    // Professional Configurations (Please don't modify)
    sANR->noise_est_hold_time = 5;  // 1~5 second, The minimum noise consequence in seconds of detection module

	tone_min_time = 5;
    switch (sampling_rate)
    {
        case 8000:
            sANR->tone_min_time = (int)(tone_min_time * 8000 / 128);
            sANR->spec_bias_low  = 3-1;
            sANR->spec_bias_high = 128-1;
            break;
        case 11025:
            sANR->tone_min_time = (int)(tone_min_time * 11025 / 128);
            sANR->spec_bias_low = 3-1;
            sANR->spec_bias_high = 128-1;
            break;
        case 16000:
            sANR->tone_min_time = (int)(tone_min_time * 16000 / 256);
            sANR->spec_bias_low = 3-1;
            sANR->spec_bias_high = 256-1;
            break;
        case 22050:
            sANR->tone_min_time = (int)(tone_min_time * 22050 / 256);
            sANR->spec_bias_low = 3-1;
            sANR->spec_bias_high = 256-1;
            break;
        case 32000:
            sANR->tone_min_time = (int)(tone_min_time * 32000 / 512);
            sANR->spec_bias_low = 3-1;
            sANR->spec_bias_high = 512-1;
            break;
        case 44100:
            sANR->tone_min_time = (int)(tone_min_time * 44100 / 512);
            sANR->spec_bias_low = 3-1;
            sANR->spec_bias_high = 512-1;
            break;
        case 48000:
            sANR->tone_min_time = (int)(tone_min_time * 48000 / 512);
            sANR->spec_bias_low = 3-1;
            sANR->spec_bias_high = 512-1;
            break;
        default:
            emu_aec_msg("ANR Config Error\r\n");
            return 0;
            break;
    }
	sANR->m_curve_n1_level = 0;
	sANR->m_curve_n2_level = 0;


	SpecLen = GetSpecLen(sampling_rate);
    sANR->max_bias_limit = (unsigned int)(2.0*(1<<6));


    for (i=0;i<SpecLen;i++)
        sANR->default_spec[i] = default_spec;
    return 1;
}
void _run_aec_test(AEC_AUTOTEST_SETTING setting)
{
    int  file_sec;
    int  file_size;
    CHAR nearend[20]   = "/mnt/sd/mic.PCM";
    CHAR farend[20]    = "/mnt/sd/spk.PCM";
    CHAR aecOutput[20] = "/mnt/sd/OUT0001.PCM";
    AEC_BITSTREAM AecIO;

    emu_aec_msg("\r\nIn run_aec_test \r\n");

    file_size = testcode_read_file(nearend, (CHAR *) uiAudAecRecBufAdr);
    file_size = testcode_read_file(farend,  (CHAR *) uiAudAecPlyBufAdr);
    file_sec = file_size / (setting.SampleRate * setting.Mic_CH * 2);

    emu_aec_msg("file_sec = [%d] file_size = [%x]\r\n",file_sec,file_size);

    audlib_aec_open();


    /*AEC Setting*/
    audlib_aec_set_config(AEC_CONFIG_ID_SAMPLERATE,           setting.SampleRate);
    audlib_aec_set_config(AEC_CONFIG_ID_PLAYBACK_CH_NO,       setting.Spk_CH);
    audlib_aec_set_config(AEC_CONFIG_ID_RECORD_CH_NO,         setting.Mic_CH);
    audlib_aec_set_config(AEC_CONFIG_ID_FILTER_LEN,           setting.FilterLength);
    audlib_aec_set_config(AEC_CONFIG_ID_FRAME_SIZE,           setting.FrameSize);
    audlib_aec_set_config(AEC_CONFIG_ID_NONLINEAR_PROCESS_EN, setting.bNLP_EN);
    audlib_aec_set_config(AEC_CONFIG_ID_PRELOAD_EN,           setting.bPreload_EN);
    audlib_aec_set_config(AEC_CONFIG_ID_AR_MODULE_EN,         setting.bAR_EN);
    audlib_aec_set_config(AEC_CONFIG_ID_NOISE_CANEL_EN,       setting.bNoiseSuppress_EN);
    audlib_aec_set_config(AEC_CONFIG_ID_NOISE_CANCEL_LVL,     setting.NoiseSuppress_LVL);
    audlib_aec_set_config(AEC_CONFIG_ID_ECHO_CANCEL_LVL,      setting.EchoCancel_LVL);
	audlib_aec_set_config(AEC_CONFIG_ID_SPK_NUMBER,           2);

    audlib_aec_set_config(AEC_CONFIG_ID_FOREADDR, uiAecPreForeAddr);
    audlib_aec_set_config(AEC_CONFIG_ID_BACKADDR, uiAecPreBackAddr);
    audlib_aec_set_config(AEC_CONFIG_ID_FORESIZE, AEC_PRELOAD_FORE_BUFSIZE);
    audlib_aec_set_config(AEC_CONFIG_ID_BACKSIZE, AEC_PRELOAD_BACK_BUFSIZE);





    AecIO.bitstream_buffer_play_in    = uiAudAecPlyBufAdr;
    AecIO.bitstream_buffer_record_in  = uiAudAecRecBufAdr;
    AecIO.bitstram_buffer_out       = uiAudAecOutBufAdr;
    AecIO.bitstram_buffer_length        = ((file_sec * setting.SampleRate)+0x3FF) & ~0x3FF;


	audlib_aec_set_config(AEC_CONFIG_ID_BUF_ADDR,           (INT32)AecLibBufAddr);
	audlib_aec_set_config(AEC_CONFIG_ID_BUF_SIZE,           audlib_aec_get_required_buffer_size(AEC_BUFINFO_ID_INTERNAL));
	emu_aec_msg("Aec lib Internal buf size need = [%x] \r\n",audlib_aec_get_required_buffer_size(AEC_BUFINFO_ID_INTERNAL));

    audlib_aec_init();

    emu_aec_msg("In Aec_run \r\n");

    audlib_aec_run(&AecIO);

    testcode_write_file(aecOutput, (CHAR *) uiAudAecOutBufAdr, file_size);

    emu_aec_msg("Out run_aec_test \r\n");

}

void _run_anr_test(int block_size)
{

    int             file_size;
    CHAR            anr_input[20]     = "/mnt/sd/anr_in.PCM";
    CHAR            anr_output[20]    = "/mnt/sd/anr_out.PCM";
    int             Ret_open=0;
    int             anr_handle=0;
    int             i=0;
    UINT32          buf_in,buf_out;

    file_size = testcode_read_file(anr_input, (CHAR *) uiAudAecRecBufAdr);

    emu_aec_msg("In run_anr_test / file_size = [%x]\r\n",file_size);

    buf_in = uiAudAecRecBufAdr;
    buf_out= uiAudAecOutBufAdr;

    gstemuANR.memory_needed = audlib_anr_pre_init(&gstemuANR);
    gstemuANR.p_mem_buffer   = (void *)AecLibBufAddr;

    Ret_open = audlib_anr_init(&anr_handle, &gstemuANR);
    if(Ret_open){
        emu_aec_msg("ANR_INIT Error , Code = [%d]\r\n",Ret_open);
    }

    do
    {

        //emu_msg(("uiAudANRBufIn = %x \r\n",pBufIn));

        audlib_anr_run(anr_handle,(INT16 *)buf_in, (INT16 *)buf_out);
        buf_in += (block_size);
        buf_out+= (block_size);

        i++;
    }while((i*block_size)<file_size);// blksize means 16bit pcm sample number, blksize = 1 = 2bytes


    testcode_write_file(anr_output, (CHAR *) uiAudAecOutBufAdr, file_size);
    emu_aec_msg("Out run_anr_test\r\n");


}

void _run_src_test(void)
{

    int             file_size;
    CHAR            src_input[20]     = "/mnt/sd/src_48k.PCM";
    CHAR            src_output[20]    = "/mnt/sd/src_08k.PCM";
    int             in_samplerate     = AUDIO_SR_48000;
    int             out_samplerate    = AUDIO_SR_8000;
    int             rq_buf_size;
    int             Ret,handle=0;
    int             channel = 1;//mono
    int             i=0;
    UINT32          buf_in,buf_out;



    file_size = testcode_read_file(src_input, (CHAR *) uiAudAecRecBufAdr);

    emu_aec_msg("In run_src_test / file_size = [%x]\r\n",file_size);

    buf_in = uiAudAecRecBufAdr;
    buf_out= uiAudAecOutBufAdr;

    in_samplerate /=100;
    out_samplerate/=100;

    rq_buf_size = audlib_src_pre_init(channel, in_samplerate, out_samplerate, 0);
    if(rq_buf_size <= EMU_AUDLIB_BUFSIZE){
        emu_aec_msg("audlib_src_pre_init done required BufSize = %x.....\r\n",rq_buf_size);
    }
    else{
        emu_aec_msg("audlib_src_pre_init fail .....\r\n");
        return;
    }

    Ret = audlib_src_init(&handle , channel , in_samplerate, out_samplerate, 0, (short *)AecLibBufAddr);
    if (Ret!=0){
        emu_aec_msg("audlib_src_init fail .... returns %d\r\n",Ret);
		return ;
	}

    do{
        buf_in  += in_samplerate*2*channel;//16bits per sample -> *2 to bytes
        buf_out += out_samplerate*2*channel;

        //emu_aec_msg("[%d] Bufin = %x , BufOut = %x \r\n",i,buf_in,buf_out);
        audlib_src_run(handle, (void *) buf_in, (void *) buf_out);
        i++;



    }while((i*in_samplerate*channel*2)<file_size);

    file_size *= out_samplerate;
    file_size /= in_samplerate;

    testcode_write_file(src_output, (CHAR *) uiAudAecOutBufAdr, file_size);
    audlib_src_destroy(handle);
    emu_aec_msg("Out run_src_test \r\n");

}

void _run_aac_test(void)
{

    int                     file_size;
    CHAR                    aac_input[20]        = "/mnt/sd/aac_in.PCM";
    CHAR                    aac_output[20]       = "/mnt/sd/aac_out.PCM";
    CHAR                    aac_bitstream[20]    = "/mnt/sd/aac_bs.PCM";
    UINT32                  buf_in,buf_out,buf_bsout;
    int                     channel              = 2;
    int                     aac_samplerate       = AUDIO_SR_48000;
    int                     BitRate              = 192000;
    int                     header_enable        = 0;
    int                     encode_stop_freq     = 20000;
	AUDLIB_AAC_CFG          aac_encode_cfg;
	AUDLIB_AAC_BUFINFO      aac_encode_buf_info;
    AUDLIB_AACE_RTNINFO     aac_encode_rtn_size;

	AUDLIB_AAC_CFG          aac_decode_cfg;
	AUDLIB_AAC_BUFINFO      aac_decode_buf_info;
	AUDLIB_AACD_RTNINFO     aac_decode_rtn;
    int                     Ret;
    UINT32                  i = 0, Proc = 0, EncodeSize = 0,DecodeSize=0;

    emu_aec_msg("In run_aac_test \r\n");
    file_size = testcode_read_file(aac_input, (CHAR *) uiAudAecRecBufAdr);

    buf_in   = uiAudAecRecBufAdr;
    buf_out  = uiAudAecOutBufAdr;
    buf_bsout= uiAudAecPlyBufAdr;


/*
AAC Encode
*/
    aac_encode_cfg.sample_rate      = aac_samplerate;
    aac_encode_cfg.channel_number   = channel;
    aac_encode_cfg.encode_bit_rate  = BitRate;
    aac_encode_cfg.header_enable    = header_enable;
    aac_encode_cfg.encode_stop_freq = encode_stop_freq;
    Ret = audlib_aac_encode_init(&aac_encode_cfg);
    if (Ret != 0) {
        emu_aec_msg("aac_encode_init ErrCode = %d\r\n", Ret);
	}

	Proc = file_size /(2 * channel); //aac needs sample number
	while (Proc >= 1024) {

		aac_encode_buf_info.bitstram_buffer_length       = 1024 * channel;
		aac_encode_buf_info.bitstram_buffer_in  = (int)(buf_in + (1024 * i * 2 * channel));
		aac_encode_buf_info.bitstram_buffer_out = (int)(buf_bsout + EncodeSize);
		Ret = audlib_aac_encode_one_frame(&aac_encode_buf_info, &aac_encode_rtn_size);
		if (Ret != 0) {
			emu_aec_msg("aac_encode_one_frame ErrCode = %d\r\n", Ret);
		}

		EncodeSize += aac_encode_rtn_size.output_size;
		Proc -= 1024;
		i++;
	}



    testcode_write_file(aac_bitstream, (CHAR *) buf_bsout, EncodeSize);

/*
AAC decode
*/

	aac_decode_cfg.sample_rate = aac_samplerate;
	aac_decode_cfg.channel_number = channel;
	aac_decode_cfg.header_enable = header_enable;
	Ret = audlib_aac_decode_init(&aac_decode_cfg);
	if (Ret != 0) {
	    emu_aec_msg("aac_decode_init ErrCode = %d\r\n", Ret);
	}


	Proc = 0;
	while (Proc < EncodeSize) {
	    aac_decode_buf_info.bitstram_buffer_length    = EncodeSize - Proc;
		aac_decode_buf_info.bitstram_buffer_in = (int)(buf_bsout + Proc);
		aac_decode_buf_info.bitstram_buffer_out = (int)(buf_out + DecodeSize);
		Ret = audlib_aac_decode_one_frame(&aac_decode_buf_info, &aac_decode_rtn);
		if (Ret != 0) {
		    emu_aec_msg("aac_decode_one_frame ErrCode = %d\r\n", Ret);
		}

		DecodeSize += (aac_decode_rtn.output_size * aac_decode_rtn.channel_number * 2);
		Proc += aac_decode_rtn.one_frame_consume_bytes;
	}

    testcode_write_file(aac_output,    (CHAR *) buf_out, DecodeSize);

    emu_aec_msg("EncodeSize = [%d] DecodeSize = [%d]\r\n",EncodeSize,DecodeSize);

    emu_aec_msg("Out run_aac_test \r\n");


}

void _run_g711_test(void)
{

    int                     file_size;
    CHAR                    g711_input[20]         = "/mnt/sd/g711_in.PCM";
    CHAR                    g711_output_alaw[30]   = "/mnt/sd/g711_out_alaw.PCM";
    CHAR                    g711_output_ulaw[30]   = "/mnt/sd/g711_out_ulaw.PCM";
    CHAR                    g711_alaw_bitstream[25]= "/mnt/sd/g711_bs_alaw.PCM";
    CHAR                    g711_ulaw_bitstream[25]= "/mnt/sd/g711_bs_ulaw.PCM";
    INT16                   *buf_in,*buf_out;
    UINT8                   *buf_bsout;
    UINT32                  pcm_sample_count;


    emu_aec_msg("In run_g711_test \r\n");

    file_size = testcode_read_file(g711_input, (CHAR *) uiAudAecRecBufAdr);

    pcm_sample_count = (file_size>>1);

    buf_in   = (INT16 *)uiAudAecRecBufAdr;
    buf_out  = (INT16 *)uiAudAecOutBufAdr;
    buf_bsout= (UINT8 *)uiAudAecPlyBufAdr;

    emu_aec_msg("g711 pattern sample = [%d] \r\n",pcm_sample_count);

/*
alaw encode
*/

    g711_alaw_encode(buf_in, buf_bsout, pcm_sample_count, 0);

    testcode_write_file(g711_alaw_bitstream,    (CHAR *) buf_bsout, file_size>>1);

    emu_aec_msg("a-law encode done\r\n");
/*
alaw decode
*/

    g711_alaw_decode(buf_bsout, buf_out, pcm_sample_count, 0, 0);

    testcode_write_file(g711_output_alaw, (CHAR *) buf_out, file_size);

    memset((void *)buf_out, 0x00, file_size);
    memset((void *)buf_bsout, 0x00, file_size>>1);

    emu_aec_msg("a-law decode done\r\n");



/*
ulaw encode
*/
    file_size = testcode_read_file(g711_input, (CHAR *) uiAudAecRecBufAdr);
    pcm_sample_count = (file_size>>1);

    g711_ulaw_encode(buf_in, buf_bsout, pcm_sample_count, 0);

    testcode_write_file(g711_ulaw_bitstream,    (CHAR *) buf_bsout, file_size>>1);

    emu_aec_msg("u-law encode done\r\n");
/*
ulaw decode
*/

    g711_ulaw_decode(buf_bsout, buf_out, pcm_sample_count, 0, 0);

    testcode_write_file(g711_output_ulaw, (CHAR *) buf_out, file_size);

    memset((void *)buf_out, 0x00, file_size);
    memset((void *)buf_bsout, 0x00, file_size>>1);

    emu_aec_msg("u-law decode done\r\n");


}

void _run_adpcm_test(void)
{

    int                     file_size;
    CHAR                    adpcm_input[30]    = "/mnt/sd/adpcm_in.PCM";
    CHAR                    adpcm_output[30]   = "/mnt/sd/adpcm_out.PCM";
    INT16                   *buf_in,*buf_out;
    INT8                   *buf_bsout;
    UINT32                  pcm_sample_count = ADPCM_PACKET_SAMPLES_8K,pcm_sample_count_total;
    UINT32                  i=0;
    PADPCM_STATE            p_adpcm_state = &adpcm_state;


    emu_aec_msg("In run adpcm test\r\n");
    p_adpcm_state->l_index   = 0;
    p_adpcm_state->l_val_prev = 0;
    p_adpcm_state->r_index   = 0;
    p_adpcm_state->r_val_prev = 0;



    file_size = testcode_read_file(adpcm_input, (CHAR *) uiAudAecRecBufAdr);

    pcm_sample_count_total = (file_size>>1);

    emu_aec_msg("adpcm pattern size = [%d] sample count = [%d] \r\n",file_size,pcm_sample_count_total);

    buf_in   = (INT16 *)uiAudAecRecBufAdr;
    buf_out  = (INT16 *)uiAudAecOutBufAdr;
    buf_bsout= (INT8 *)uiAudAecPlyBufAdr;

/*
adpcm encode
*/
    do{
        if(pcm_sample_count_total - i < pcm_sample_count)
            pcm_sample_count = pcm_sample_count_total - i;
        audlib_adpcm_encode_packet_mono(buf_in+i, buf_bsout+i, pcm_sample_count, p_adpcm_state);
        i += pcm_sample_count;
    }while(i<pcm_sample_count_total);




    emu_aec_msg("adpcm encode done\r\n");


/*
adpcm decode
*/

    pcm_sample_count = ADPCM_PACKET_SAMPLES_8K;
    i = 0;
    do{
        if(pcm_sample_count_total - i < pcm_sample_count)
            pcm_sample_count = pcm_sample_count_total - i;
        audlib_adpcm_decode_packet_mono(buf_bsout+i, buf_out+i, pcm_sample_count);
        i += pcm_sample_count;
    }while(i<pcm_sample_count_total);

    emu_aec_msg("adpcm decode done\r\n");

    testcode_write_file(adpcm_output, (CHAR *) buf_out, file_size);


    emu_aec_msg("Out run_adpcm_test \r\n");


}

void _run_agc_test(void)
{
    int                     file_size;
    CHAR                    agc_input[30]    = "/mnt/sd/agc_in.PCM";
    CHAR                    agc_output[30]   = "/mnt/sd/agc_out.PCM";
    INT16                   *buf_in,*buf_out;
	AGC_BITSTREAM 			AgcIO;
	INT32					sampleRate = 8000,seconds; 



    emu_aec_msg("In run agc test\r\n");


    file_size = testcode_read_file(agc_input, (CHAR *) uiAudAecRecBufAdr);
	
	seconds = file_size/sampleRate/4;

    emu_aec_msg("agc pattern size = [%d]  \r\n",file_size);

    buf_in   = (INT16 *)uiAudAecRecBufAdr;
    buf_out  = (INT16 *)uiAudAecOutBufAdr;
	
	emu_aec_msg("START audio AGC test \n");

	audlib_agc_open();

	audlib_agc_set_config(AGC_CONFIG_ID_SAMPLERATE, (INT32)sampleRate);
	audlib_agc_set_config(AGC_CONFIG_ID_CHANNEL_NO, 2);
	audlib_agc_set_config(AGC_CONFIG_ID_TARGET_LVL, AGC_DB(-12));
	audlib_agc_set_config(AGC_CONFIG_ID_MAXGAIN, 	AGC_DB(12));
	audlib_agc_set_config(AGC_CONFIG_ID_MINGAIN,    AGC_DB(-12));
	audlib_agc_set_config(AGC_CONFIG_ID_NG_THD,     AGC_DB(-35));

	audlib_agc_set_config(AGC_CONFIG_ID_MSGDUMP,    ENABLE);

	audlib_agc_init();

	AgcIO.bitstram_buffer_in        = (UINT32)buf_in;
	AgcIO.bitstram_buffer_out       = (UINT32)buf_out;
	AgcIO.bitstram_buffer_length    = seconds * sampleRate;

	audlib_agc_run(&AgcIO);

	audlib_agc_close();

	testcode_write_file(agc_output, (CHAR *) buf_out, file_size);
	
	emu_aec_msg("DONE audio AGC test \n");
}

void aec_test_flow(void)
{

    AEC_AUTOTEST_SETTING aec_setting;
    aec_setting.SampleRate          = AUDIO_SR_8000;
    aec_setting.Mic_CH              = 2;
    aec_setting.Spk_CH              = 2;
    aec_setting.FilterLength        = 1024;
    aec_setting.FrameSize           = 128;

    aec_setting.bAR_EN              = TRUE;
    aec_setting.bNLP_EN             = FALSE;
    aec_setting.bNoiseSuppress_EN   = TRUE;
    aec_setting.bPreload_EN         = FALSE;
    aec_setting.NoiseSuppress_LVL   = -20;
    aec_setting.EchoCancel_LVL      = -50;

    test_buf_init();
    _run_aec_test(aec_setting);

    memset((void *)AecLibBufAddr, 0x00, 0x400000);

}

void agc_test_flow(void)
{
	_run_agc_test();

	memset((void *)AecLibBufAddr, 0x00, 0x400000);
}

void src_test_flow(void)
{

    _run_src_test();

    memset((void *)AecLibBufAddr, 0x00, 0x400000);

}

void aac_test_flow(void)
{

    _run_aac_test();

    memset((void *)AecLibBufAddr, 0x00, 0x400000);

}

void anr_test_flow(void)
{

    int ret_anr;
    int test_samplerate = AUDIO_SR_8000;
    int block_size = 2048;
    int anr_channel = 0 ;// mono

    ret_anr = config_anr_test(block_size, test_samplerate, anr_channel, &gstemuANR);

    if(ret_anr)
        _run_anr_test(block_size);

}

void g711_test_flow(void)
{

    _run_g711_test();

    memset((void *)AecLibBufAddr, 0x00, 0x400000);

}

void adpcm_test_flow(void)
{

    _run_adpcm_test();

    memset((void *)AecLibBufAddr, 0x00, 0x400000);

}
#endif

#if 1

void _run_kdrv_test(void)
{
    int                     file_size,file_sec = 15;
    int                     init_buf_cnt = 4,test_data = 712;
    CHAR                    kdrv_tx_input[40]          = "/mnt/sd/kdrv_tone.pcm";
    CHAR                    kdrv_rx_output[40]         = "/mnt/sd/record_out.PCM";
    CHAR                    kdrv_txlb_output[40]       = "/mnt/sd/loopback_out.PCM";
    UINT32                  tx_ID = 0,rx_ID = 0, txlb_ID = 0;

    void                   *param,*param1,*param_setting;
    KDRV_CALLBACK_FUNC      test_cb = {0};
    KDRV_BUFFER_INFO        test_buf = {0};
    UINT32                  trigger_cnt_rx=0,trigger_cnt_tx=0,trigger_cnt_txlb=0;

    INT32                  ret;



    file_size = testcode_read_file(kdrv_tx_input, (CHAR *) uiAudAecPlyBufAdr);

    test_cb.reserve_buf = &reserve_buf_test;
    test_cb.free_buf    = &free_buf_test;
    test_cb.callback    = &kdrv_cb_test;

    param = (KDRV_BUFFER_INFO *)AecLibBufAddr;
    param1 = (KDRV_CALLBACK_FUNC *)(AecLibBufAddr+0x100);
    param_setting = (UINT32 *)(AecLibBufAddr+0x200);

    param1= &test_cb;


    tx_ID = KDRV_DEV_ID(0, KDRV_AUDOUT_ENGINE0, 0);
    emu_aec_msg("get handler 0x%x\r\n",tx_ID);
    ret = kdrv_audioio_open(0, KDRV_AUDOUT_ENGINE0);
    if(ret == -1)
        emu_aec_msg("tx open fail\r\n");

    rx_ID = KDRV_DEV_ID(0, KDRV_AUDCAP_ENGINE0, 0);
    emu_aec_msg("get handler 0x%x\r\n",rx_ID);
    ret  = kdrv_audioio_open(0, KDRV_AUDCAP_ENGINE0);
    if(ret == -1)
        emu_aec_msg("rx open fail\r\n");

    txlb_ID = KDRV_DEV_ID(0, KDRV_AUDCAP_ENGINE1, 0);
    emu_aec_msg("get handler 0x%x\r\n",txlb_ID);
    ret  = kdrv_audioio_open(0, KDRV_AUDCAP_ENGINE1);
    if(ret == -1)
        emu_aec_msg("txlb open fail\r\n");

	if (audio_test_ext_codec) {
		*(KDRV_AUDIO_OUT_PATH *)param_setting = KDRV_AUDIO_CAP_PATH_I2S;
		kdrv_audioio_set(rx_ID, KDRV_AUDIOIO_CAP_PATH, (void *)param_setting);
	}


    //kdrv_audioio_set(handler, KDRV_AUDIOIO_CAP_TIMECODE_HIT_CB, param1);


    test_buf.size    = 48000 * 1 * 2 * 2;
    for(trigger_cnt_rx = 0;trigger_cnt_rx < init_buf_cnt;trigger_cnt_rx++){
		if (!audio_test_ext_codec) {
	        /*txlb*/
	        test_buf.addr_pa = (INT32)(uiAudAecOutBufAdr + (test_buf.size * trigger_cnt_rx));
	        param = &test_buf;
	        kdrv_audioio_trigger(txlb_ID, param, param1, &test_data);
		}
        /*rx*/
        test_buf.addr_pa = (INT32)(uiAudAecRecBufAdr + (test_buf.size * trigger_cnt_rx));
        param = &test_buf;
        kdrv_audioio_trigger(rx_ID, param, param1, &test_data);
        /*tx*/
        test_buf.addr_pa = (INT32)(uiAudAecPlyBufAdr + (test_buf.size * trigger_cnt_rx));
        param = &test_buf;
        kdrv_audioio_trigger(tx_ID, param, param1, &test_data);
    }
        trigger_cnt_tx = trigger_cnt_txlb = trigger_cnt_rx;

	
    while(1){

            if(!(dai_is_rx_enable()||dai_is_tx_enable(DAI_TXCH_TX1)||dai_is_txlb_enable())){

                    emu_aec_msg("rx/tx buffer all done! \r\n");

                    break;
            }

            if (dma_done_tx) {
                if(trigger_cnt_tx < file_sec) {
                    test_buf.addr_pa = (INT32)(uiAudAecPlyBufAdr + (test_buf.size * trigger_cnt_tx));
                    param = &test_buf;
                    kdrv_audioio_trigger(tx_ID, param, param1, &test_data);
                }
                trigger_cnt_tx++;
                dma_done_tx = 0;
            }

            if (dma_done_rx) {
                if(trigger_cnt_rx < file_sec) {
                    test_buf.addr_pa = (INT32)(uiAudAecRecBufAdr + (test_buf.size * trigger_cnt_rx));
                    param = &test_buf;
                    kdrv_audioio_trigger(rx_ID, param, param1, &test_data);
                }
                trigger_cnt_rx++;
                dma_done_rx = 0;
            }

            if (dma_done_txlb) {
                if(trigger_cnt_txlb < file_sec) {
                    test_buf.addr_pa = (INT32)(uiAudAecOutBufAdr + (test_buf.size * trigger_cnt_txlb));
                    param = &test_buf;
                    kdrv_audioio_trigger(txlb_ID, param, param1, &test_data);
                }
                trigger_cnt_txlb++;
                dma_done_txlb = 0;
            }

            if(trigger_cnt_tx == 7) {
                *(UINT32 *)param_setting = 50;
                kdrv_audioio_set(tx_ID, KDRV_AUDIOIO_OUT_VOLUME, (void *)param_setting);
            }
            if(trigger_cnt_tx == 8) {
                *(UINT32 *)param_setting = 100;
                kdrv_audioio_set(tx_ID, KDRV_AUDIOIO_OUT_VOLUME, (void *)param_setting);
            }

            if(trigger_cnt_rx == 10) {
				*(UINT32 *)param_setting = 20;
                kdrv_audioio_set(rx_ID, KDRV_AUDIOIO_CAP_VOLUME, (void *)param_setting);
            }
            if(trigger_cnt_rx == 13) {
				*(UINT32 *)param_setting = 100;
                kdrv_audioio_set(rx_ID, KDRV_AUDIOIO_CAP_VOLUME, (void *)param_setting);
            }


        }

        testcode_write_file(kdrv_rx_output, (CHAR *) uiAudAecRecBufAdr, file_size);
        testcode_write_file(kdrv_txlb_output, (CHAR *) uiAudAecOutBufAdr, file_size);
}

void kdrv_test_flow(void)
{
    _run_kdrv_test();

    memset((void *)AecLibBufAddr, 0x00, 0x400000);
}
#endif



int nvt_audlib_emu_lib_thread(void * __audlib_emu)
{
#if TEST_LIB
		test_buf_init();
		agc_test_flow();
/*
        aec_test_flow();
        src_test_flow();
        anr_test_flow();
        aac_test_flow();
        g711_test_flow();
        adpcm_test_flow();
*/
#else
        test_buf_init();
        kdrv_test_flow();
#endif

	kfree(main_buffer);

	//frm_free_buf_ddr((void *)AecLibBufAddr);

	emu_aec_msg("nvt_audlib_emu_aec_thread thread closed\r\n");
	return 0;

}