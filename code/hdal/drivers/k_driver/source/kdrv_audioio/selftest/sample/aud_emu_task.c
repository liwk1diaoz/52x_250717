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

#include "../../dai/include/dai.h"
#include "../../audio_common/include/Audio.h"
#include "../../audio_common/include/AudioCodec.h"
#include "kdrv_audioio/audlib_filt.h"
#include "kdrv_audioio/audlib_agc.h"
#include "kdrv_audioio/audlib_doa.h"
#include "frammap/frammap_if.h"
#include "comm/hwclock.h"


#define emu_msg(x) DBG_DUMP x
#define    			AUDTS_CH_RX2            AUDTS_CH_TOT
#define AUDTEST_BUF_SIZE                    0x177000         // 48000*8*4bytes
#define TestAUDIO_BUF_BLKNUM                5
#define DAI_DMA_BANDWIDTH_SIZE              (0x30000)

#define CPU_FREQ							800

typedef enum {
	EMUDAI_CODEC_TYPE_EAC,
	EMUDAI_CODEC_TYPE_HDMI,
	EMUDAI_CODEC_TYPE_EXT,

	ENUM_DUMMY4WORD(EMUDAI_CODEC_TYPE)
} EMUDAI_CODEC_TYPE;


static PAUDTS_OBJ 			pAudTsObj[AUDTS_CH_TOT + 1];
static UINT32           	gEmuDaiBufferAddr[AUDTS_CH_TOT + 1] = {0,   0,   0,   0,   0};
static UINT32           	gEmuDaiBufferSize[AUDTS_CH_TOT + 1] = {0,   0,   0,   0,   0};
static DAI_DRAMPCMLEN   	gEmuDaiPcmLen[AUDTS_CH_TOT + 1] = {DAI_DRAMPCM_16, DAI_DRAMPCM_16, DAI_DRAMPCM_16, DAI_DRAMPCM_16, DAI_DRAMPCM_16};
static DAI_CH           	gEmuDaiCH[AUDTS_CH_TOT + 1]     = {DAI_CH_STEREO, DAI_CH_STEREO, DAI_CH_STEREO, DAI_CH_STEREO, DAI_CH_STEREO};
volatile UINT32         	gTriggerVal[AUDTS_CH_TOT + 1];
static UINT32           	gEmuDaiNextFilePos[AUDTS_CH_TOT + 1];
static AUDIO_BUF_QUEUE  	gEmuDaiAudBufQueue[AUDTS_CH_TOT + 1];
static BOOL                 gAudExpand[AUDTS_CH_TOT + 1] = {0, 0, 0, 0, 0};
volatile UINT32         	gTrigStepVal[AUDTS_CH_TOT + 1];
static UINT32           	gEmuDaiNextBufId[AUDTS_CH_TOT + 1];
volatile UINT32         	gDaiDmaDoneNum[AUDTS_CH_TOT + 1];
volatile UINT32         	gTimeHitNum[AUDTS_CH_TOT + 1];
static UINT32           	gEmuDaiPreDMADoneNum[AUDTS_CH_TOT + 1];
static loff_t				file_rw_offset[AUDTS_CH_TOT + 1];
static UINT32           	gEmuDaiTxFileSize[AUDTS_CH_RX]    = {0,   0};
static UINT32           	gEmuDaitBufAddr[AUDTS_CH_TOT + 1][TestAUDIO_BUF_BLKNUM];

static AUDIO_CODEC_FUNC 	gExtCodecFunc = {0};
static AUDIO_SETTING        gAudioSetting;
static UINT32 				EmuDaiSizeOfs = 0;
BOOL                    	gAudSpkrOut = TRUE;
static struct file*         gEmuDaiFileHandle[AUDTS_CH_TOT + 1] = {NULL, NULL, NULL, NULL, NULL};
static UINT32 EmuDaiAddrOfs = 0;

CHAR                    gcDaiFileName[AUDTS_CH_TOT + 1][64] = {
	"/mnt/sd/STEO1.WAV",
	"/mnt/sd/STEO2.WAV",
	"/mnt/sd/RXPCM.PCM",
	"/mnt/sd/TXLBPCM.PCM",
	"/mnt/sd/RX2PCM.PCM"
};

AUDFILT_CONFIG  bandstop_filt = {
	AUDFILT_ORDER(10),                      // Filter Order
	AUDFILT_GAIN(130.68918637547981),       // Total Gain
	// Filter Coeficients
	{
		{AUDFILT_COEF(1.0), AUDFILT_COEF(-1.9587319789879971), AUDFILT_COEF(0.99999999999999967), AUDFILT_COEF(1.0), AUDFILT_COEF(-1.6243843372534204), AUDFILT_COEF(0.71451172570569921)},
		{AUDFILT_COEF(1.0), AUDFILT_COEF(-1.9587319789879971), AUDFILT_COEF(0.99999999999999967), AUDFILT_COEF(1.0), AUDFILT_COEF(-1.9507588655145636), AUDFILT_COEF(0.96192616145589149)},
		{AUDFILT_COEF(1.0), AUDFILT_COEF(-1.9587319789879971), AUDFILT_COEF(0.99999999999999967), AUDFILT_COEF(1.0), AUDFILT_COEF(-1.8615160780583855), AUDFILT_COEF(0.87656515752190167)},
		{AUDFILT_COEF(1.0), AUDFILT_COEF(-1.9587319789879969), AUDFILT_COEF(0.99999999999999956), AUDFILT_COEF(1.0), AUDFILT_COEF(-1.7345132170722297), AUDFILT_COEF(0.87182519659607749)},
		{AUDFILT_COEF(1.0), AUDFILT_COEF(-1.9587319789879973), AUDFILT_COEF(0.99999999999999967), AUDFILT_COEF(1.0), AUDFILT_COEF(-1.7106932608219678), AUDFILT_COEF(0.74673541778372188)}
	},
	// Section Gain
	{
		AUDFILT_GAIN(0.079103221417675729),
		AUDFILT_GAIN(0.140929325730601380),
		AUDFILT_GAIN(0.429863466098812540),
		AUDFILT_GAIN(1.0),
		AUDFILT_GAIN(1.0)
	}
};



#define EMUDAI_ERRFLAG_TX  (AUDIO_EVENT_BUF_FULL|AUDIO_EVENT_BUF_EMPTY|AUDIO_EVENT_FIFO_ERROR|AUDIO_EVENT_BUF_RTEMPTY|AUDIO_EVENT_BUF_FULL2|AUDIO_EVENT_DONEBUF_FULL2|AUDIO_EVENT_FIFO_ERROR2)
#define EMUDAI_ERRFLAG_RX  (AUDIO_EVENT_BUF_FULL|AUDIO_EVENT_BUF_EMPTY|AUDIO_EVENT_DONEBUF_FULL|AUDIO_EVENT_FIFO_ERROR|AUDIO_EVENT_BUF_RTEMPTY|AUDIO_EVENT_BUF_FULL2|AUDIO_EVENT_DONEBUF_FULL2|AUDIO_EVENT_FIFO_ERROR2)


static UINT32 max_time = 6000000;
module_param_named(max_time, max_time, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(max_time, "Max record/playback time in seconds");

static UINT32 msgonoff = 1;
module_param_named(msgonoff, msgonoff, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(msgonoff, "Print Messages druing record/playback");

static UINT32 mode = 0;
module_param_named(mode, mode, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mode, "0 is FullDuplex_Test. 1 is audfilt");


#if 1

static void InitAudio(EMUDAI_CODEC_TYPE Type)
{
	AUDIO_SETTING       AudioSetting;
	AUDIO_DEVICE_OBJ    AudioDevice;
	UINT32              i;

    frammap_buf_t       BufTx = {0};

	// The following settings are "Don't care" for NT96650 built-in audio codec.
	//AudioDevice.pEventHandler       = NULL;
	AudioDevice.uiGPIOColdReset     = 0;
	AudioDevice.uiI2SCtrl           = AUDIO_I2SCTRL_GPIO_I2C;
	AudioDevice.uiChannel           = 0;
	AudioDevice.uiGPIOData          = 0;//P_GPIO_0;
	AudioDevice.uiGPIOClk           = 0;//P_GPIO_2;
	AudioDevice.uiGPIOCS            = 0;
	AudioDevice.uiADCZero           = 0;

	AudioSetting.Clock.bClkExt      = FALSE;                        // Must be
	AudioSetting.I2S.bMaster        = TRUE;                         // Must be
	AudioSetting.I2S.I2SFmt         = AUDIO_I2SFMT_STANDARD;        // Must be
	AudioSetting.I2S.ClkRatio       = AUDIO_I2SCLKR_256FS_32BIT;    // Must be
	AudioSetting.Fmt                = AUDIO_FMT_I2S;                // Must be
	AudioSetting.Clock.Clk          = AUDIO_CLK_12288;              // Clock = Sampling rate * 256
	AudioSetting.SamplingRate       = AUDIO_SR_48000;               // Sampling rate = Clock / 256
	AudioSetting.Channel            = AUDIO_CH_STEREO;              // Audio channel
	AudioSetting.RecSrc             = AUDIO_RECSRC_MIC;             // Must be
	AudioSetting.Output             = AUDIO_OUTPUT_SPK;             // Output source
	AudioSetting.bEmbedded          = TRUE;                         // Must be. (Don't care in audio lib)

	if (Type == EMUDAI_CODEC_TYPE_EXT) {
		#if 1
		emu_msg(("Select Emualtion External Audio Codec\r\n"));
		aud_ext_codec_emu_get_fp(&gExtCodecFunc);
		aud_set_extcodec(&gExtCodecFunc);
		#else
		emu_msg(("Select AC108 External Audio Codec\r\n"));

		AudioSetting.I2S.bMaster        = DISABLE;
		AudioSetting.I2S.ClkRatio       = AUDIO_I2SCLKR_256FS_256BIT;
		AudioDevice.uiGPIOData = 1; //0 is I2C1, 1 is I2C2.
		aud_ext_ac108_get_fp(&gExtCodecFunc);
		aud_set_extcodec(&gExtCodecFunc);
		#endif
	} else {
		emu_msg(("Select Embedded Audio Codec\r\n"));
		audcodec_get_fp(&gExtCodecFunc);
		aud_set_extcodec(&gExtCodecFunc);
	}

	aud_set_device_object(&AudioDevice);

	// Open audio driver
	aud_close();
	//emu_msg(("open audio driver"));
	aud_open();

	// Get AUDIO Transceive Obj
	//emu_msg(("Init Audio TS OBJ..\r\n"));
	for (i = 0; i <= AUDTS_CH_TOT; i++) {
		pAudTsObj[i] = aud_get_transceive_object(i);

		if (pAudTsObj[i] == NULL) {
			emu_msg(("No such TranSceive Object - %d\r\n", i));
		}
	}
	pAudTsObj[AUDTS_CH_RX2] = pAudTsObj[AUDTS_CH_RX];

	for (i = AUDTS_CH_TX1; i <= AUDTS_CH_RX2; i++) {
		if (i == 0) {
		    BufTx.size = 0x3100000;
		    BufTx.align = 64;      ///< address alignment
		    BufTx.name = "audtest";
		    BufTx.alloc_type = ALLOC_CACHEABLE;
		    frm_get_buf_ddr(DDR_ID0, &BufTx);

			emu_msg(("PA=0x%08X VA=0x%08X\n",(UINT32)BufTx.phy_addr,(UINT32)BufTx.va_addr));

			gEmuDaiBufferAddr[i] = (UINT32)BufTx.va_addr;
		} else {
			gEmuDaiBufferAddr[i] = gEmuDaiBufferAddr[i - 1] + (AUDTEST_BUF_SIZE * TestAUDIO_BUF_BLKNUM);
		}
		gEmuDaiBufferAddr[i] = (gEmuDaiBufferAddr[i] + 0xFFFFF) & 0xFFF00000;
		//emu_msg(("gEmuDaiBufferAddr[%d]= 0x%08X\r\n", i, gEmuDaiBufferAddr[i]));
	}

	//emu_msg(("Enable CLK always ON..\r\n"));
	aud_set_feature(AUDIO_FEATURE_INTERFACE_ALWAYS_ACTIVE, TRUE);

	// Init Audio driver
	memcpy(&gAudioSetting, &AudioSetting, sizeof(gAudioSetting));
	aud_init(&AudioSetting);

	if(Type == EMUDAI_CODEC_TYPE_EAC) {
		//emu_msg(("aud_set_default_setting(AUDIO_DEFSET_20DB)\r\n"));
		aud_set_default_setting(AUDIO_DEFSET_20DB);
	}

	//if (Type == EMUDAI_CODEC_TYPE_HDMI) {
	//	aud_switch_codec(AUDIO_CODECSEL_HDMI);
	//} else {
	aud_switch_codec(AUDIO_CODECSEL_DEFAULT);
	//}

	// aud_close();
}


//For DAI testing
void AudTestEventTxISR(UINT32 AudioEventStatus)
{
	//static UINT32 PreTime = 0, CurTime = 0;

	//debug_err(("%s: 0x%lx\r\n", __func__, AudioEventStatus));
	if (AudioEventStatus & AUDIO_EVENT_DMADONE) {
		gDaiDmaDoneNum[AUDTS_CH_TX1]++;
		if (msgonoff) {
			DBG_ERR("TX1 gDMADoneNum =%d\r\n", gDaiDmaDoneNum[AUDTS_CH_TX1]);
		}
	}
	if (AudioEventStatus & AUDIO_EVENT_TCHIT) {
		UINT32 uiAddr = dai_get_tx_dma_curaddr(pAudTsObj[AUDTS_CH_TX1]->AudTSCH);

		//PreTime = CurTime;
		//CurTime = Perf_GetCurrent();

		gTimeHitNum[AUDTS_CH_TX1]++;

		if (msgonoff) {
			//DBG_ERR("Tx time code hit =%d  %d  %d\r\n", gTriggerVal[AUDTS_CH_TX1], CurTime, CurTime - PreTime);
			DBG_ERR("Tx time code hit =%d\r\n", gTriggerVal[AUDTS_CH_TX1]);
			DBG_ERR("Tx DMA curr adr = 0x%x\r\n", uiAddr);
		}
		gTriggerVal[AUDTS_CH_TX1] += gTrigStepVal[AUDTS_CH_TX1];
		pAudTsObj[AUDTS_CH_TX1]->setConfig(AUDTS_CFG_ID_TIMECODE_TRIGGER, gTriggerVal[AUDTS_CH_TX1]);
	}

	if (AudioEventStatus & AUDIO_EVENT_TCLATCH) {
		if (msgonoff) {
			DBG_ERR("Tx time code latch =%d\r\n", pAudTsObj[AUDTS_CH_TX1]->getConfig(AUDTS_CFG_ID_TIMECODE_VALUE));
		}
	}

	if (AudioEventStatus & EMUDAI_ERRFLAG_TX) {
		DBG_ERR("^RTx ERROR! Flag=0x%08X\r\n", (AudioEventStatus & EMUDAI_ERRFLAG_TX));
	}

}


//For DAI testing
void AudTestEventTx2ISR(UINT32 AudioEventStatus)
{
	//static UINT32 PreTime = 0, CurTime = 0;

	//debug_err(("%s: 0x%lx\r\n", __func__, AudioEventStatus));
	if (AudioEventStatus & AUDIO_EVENT_DMADONE) {
		//PreTime = CurTime;
		//CurTime = Perf_GetCurrent();

		gDaiDmaDoneNum[AUDTS_CH_TX2]++;
		if (msgonoff) {
			//DBG_ERR("Tx2 gDMADoneNum =%d  %d  %d\r\n", gDaiDmaDoneNum[AUDTS_CH_TX2], CurTime, CurTime - PreTime);
			DBG_ERR("Tx2 gDMADoneNum =%d\r\n", gDaiDmaDoneNum[AUDTS_CH_TX2]);
		}
	}
	if (AudioEventStatus & EMUDAI_ERRFLAG_TX) {
		DBG_ERR("^RTx2 ERROR! Flag=0x%08X\r\n", (AudioEventStatus & EMUDAI_ERRFLAG_TX));
	}


}


void AudTestEventRxISR(UINT32 AudioEventStatus)
{
	//static UINT32 PreTime = 0, CurTime = 0;

	//debug_err(("%s: 0x%lx\r\n", __func__, AudioEventStatus));
	if (AudioEventStatus & AUDIO_EVENT_DMADONE) {
		gDaiDmaDoneNum[AUDTS_CH_RX]++;
		if (msgonoff) {
			DBG_ERR("Rx gDMADoneNum =%d\r\n", gDaiDmaDoneNum[AUDTS_CH_RX]);
		}
	}

	if (AudioEventStatus & AUDIO_EVENT_TCHIT) {
		UINT32 uiAddr = dai_get_rx_dma_curaddr(0);

		//PreTime = CurTime;
		//CurTime = Perf_GetCurrent();

		gTimeHitNum[AUDTS_CH_RX]++;

		if (msgonoff) {
			//DBG_ERR("Rx time code hit =%d  %d  %d\r\n", gTriggerVal[AUDTS_CH_RX], CurTime, CurTime - PreTime);
			DBG_ERR("Rx time code hit =%d\r\n", gTriggerVal[AUDTS_CH_RX]);
			DBG_ERR("Rx DMA curr adr = 0x%x\r\n", uiAddr);
		}
		gTriggerVal[AUDTS_CH_RX] += gTrigStepVal[AUDTS_CH_RX];
		pAudTsObj[AUDTS_CH_RX]->setConfig(AUDTS_CFG_ID_TIMECODE_TRIGGER, gTriggerVal[AUDTS_CH_RX]);
	}

	if (AudioEventStatus & AUDIO_EVENT_TCLATCH) {
		if (msgonoff) {
			DBG_ERR("^RRx time code latch =%d\r\n", pAudTsObj[AUDTS_CH_RX]->getConfig(AUDTS_CFG_ID_TIMECODE_VALUE));
		}
	}

	if (AudioEventStatus & EMUDAI_ERRFLAG_RX) {
		DBG_ERR("^RRx ERROR! Flag=0x%08X\r\n", (AudioEventStatus & EMUDAI_ERRFLAG_RX));
	}

}



void AudTestEventTxlbISR(UINT32 AudioEventStatus)
{
	//static UINT32 PreTime = 0, CurTime = 0;

	//debug_err(("%s: 0x%lx\r\n", __func__, AudioEventStatus));
	if (AudioEventStatus & AUDIO_EVENT_DMADONE) {
		//PreTime = CurTime;
		//CurTime = Perf_GetCurrent();

		gDaiDmaDoneNum[AUDTS_CH_TXLB]++;
		if (msgonoff) {
			//DBG_ERR("TXLB gDMADoneNum =%d  %d  %d\r\n", gDaiDmaDoneNum[AUDTS_CH_TXLB], CurTime, CurTime - PreTime);
			DBG_ERR("TXLB gDMADoneNum =%d\r\n", gDaiDmaDoneNum[AUDTS_CH_TXLB]);
		}
	}

	if (AudioEventStatus & EMUDAI_ERRFLAG_RX) {
		DBG_ERR("^RTXLB ERROR! Flag=0x%08X\r\n", (AudioEventStatus & EMUDAI_ERRFLAG_RX));
	}

}


void emu_daiConfigureTxRx(AUDTS_CH TsCH, AUDIO_SR sampleRate, AUDIO_CH AudChannel)
{
	void                *pIsrCB[AUDTS_CH_TOT + 2] = {AudTestEventTxISR, AudTestEventTx2ISR, AudTestEventRxISR, AudTestEventTxlbISR, AudTestEventRxISR};
	UINT32               i, loop;
	//INT32               fstatus;

	i = TsCH;

	if (TsCH <= AUDTS_CH_TX2) {
		//init buffer size
		gEmuDaiBufferSize[i] = 1 << gEmuDaiPcmLen[i];
		if (gEmuDaiCH[i] == DAI_CH_STEREO) {
			gEmuDaiBufferSize[i] <<= 1;
		}

		gEmuDaiBufferSize[i] = gEmuDaiBufferSize[i] * ((dai_get_tx_config(TsCH, DAI_TXCFG_ID_TOTAL_CH) + 1));
		gTriggerVal[i]   = sampleRate;

		gEmuDaiBufferSize[i] = gEmuDaiBufferSize[i] * sampleRate;

		gEmuDaiBufferSize[i] += EmuDaiSizeOfs;
		gEmuDaiBufferSize[i] &= (~0x3F);

		{
			struct kstat stat;
			int error = 0;
			loff_t logstrs_size;

			logstrs_size = gEmuDaiBufferSize[i] * (TestAUDIO_BUF_BLKNUM-1);

			gEmuDaiFileHandle[i] = filp_open(gcDaiFileName[i], O_RDONLY, 0);
			if (IS_ERR(gEmuDaiFileHandle[i])) {
				DBG_ERR("unable to open backing file: %s\n", gcDaiFileName[i]);
			}
			file_rw_offset[i] = 0;

			error = vfs_stat(gcDaiFileName[i], &stat);
			if (error) {
				DBG_ERR("%s: Failed to stat file %s \n", __FUNCTION__, gcDaiFileName[i]);
			}
			gEmuDaiTxFileSize[i] = (UINT32) stat.size;
			//DBG_DUMP("%s: file %s size is 0x%08X \n", __FUNCTION__, gcDaiFileName[i], gEmuDaiTxFileSize[i]);

			if (vfs_read(gEmuDaiFileHandle[i],
(char *)(gEmuDaiBufferAddr[i] + EmuDaiAddrOfs), logstrs_size, &gEmuDaiFileHandle[i]->f_pos) !=	logstrs_size) {
				DBG_ERR("%s: Failed to read file %s", __FUNCTION__, gcDaiFileName[i]);
			}
		}

		/*
		    handle channel config
		*/
		pAudTsObj[i]->open();
		pAudTsObj[i]->setConfig(AUDTS_CFG_ID_EVENT_HANDLE, (UINT32)pIsrCB[i]);
		if (TsCH == AUDTS_CH_TX1) {
			pAudTsObj[i]->setConfig(AUDTS_CFG_ID_TIMECODE_OFFSET, 0);
		}


		gTrigStepVal[i]  = gTriggerVal[i];

		if (TsCH == AUDTS_CH_TX1) {
			pAudTsObj[i]->setConfig(AUDTS_CFG_ID_TIMECODE_TRIGGER, gTriggerVal[i]);
		}
		pAudTsObj[i]->setChannel(AudChannel);


		/*
		    handle playback buffer queue
		*/
		pAudTsObj[i]->setSamplingRate(sampleRate);
		pAudTsObj[i]->resetBufferQueue(0);

		gEmuDaiAudBufQueue[i].uiAddress   = (gEmuDaiBufferAddr[i] + EmuDaiAddrOfs);
		gEmuDaiAudBufQueue[i].uiSize      = gEmuDaiBufferSize[i];
		gEmuDaitBufAddr[i][0] = gEmuDaiAudBufQueue[i].uiAddress;
		loop = 1;
		while ((pAudTsObj[i]->isBufferQueueFull(0) == FALSE) && (loop < TestAUDIO_BUF_BLKNUM)) {
			// Add buffer to queue
			if (pAudTsObj[i]->addBufferToQueue(0, &gEmuDaiAudBufQueue[i]) == FALSE) {
				DBG_ERR("aud_addBufferToQueue TX%d failed\n", i + 1);
			}
			// Advance to next data buffer
			gEmuDaiAudBufQueue[i].uiAddress = gEmuDaiAudBufQueue[i].uiAddress + gEmuDaiBufferSize[i]; //0x40000;
			gEmuDaitBufAddr[i][loop++] = gEmuDaiAudBufQueue[i].uiAddress;
		}
		gEmuDaiNextFilePos[i]   =  gEmuDaiBufferSize[i] * (loop - 1);
		//DBG_DUMP("TX %s Size add to queue 0x%08X loop=%d\r\n",gcDaiFileName[i],gEmuDaiNextFilePos[i],(loop - 1));
	} else {
		UINT32 j = 0;

		//init buffer size
		gEmuDaiBufferSize[i] = 1 << gEmuDaiPcmLen[i];
		if (gEmuDaiCH[i] == DAI_CH_STEREO) {
			gEmuDaiBufferSize[i] <<= 1;
		}

		if (i == AUDTS_CH_TXLB) {
			gEmuDaiBufferSize[i] = gEmuDaiBufferSize[i] * ((dai_get_txlb_config(DAI_TXLBCFG_ID_TOTAL_CH) + 1));
			gTriggerVal[i]   = sampleRate;
		} else {
			gEmuDaiBufferSize[i] = gEmuDaiBufferSize[i] * ((dai_get_rx_config(DAI_RXCFG_ID_TOTAL_CH) + 1));
			gTriggerVal[i]   = sampleRate * (dai_get_rx_config(DAI_RXCFG_ID_TOTAL_CH) + 1);
		}
		gEmuDaiBufferSize[i] = gEmuDaiBufferSize[i] * sampleRate;

		if(mode == 3) {
			// DOA Detect 333ms once
			gEmuDaiBufferSize[i] = gEmuDaiBufferSize[i]/3;
		}

		gEmuDaiBufferSize[i] += EmuDaiSizeOfs;
		gEmuDaiBufferSize[i] &= (~0x3F);

		if ((i == AUDTS_CH_RX2) && (gEmuDaiCH[i] != DAI_CH_DUAL_MONO)) {
			return;
		}

		gEmuDaiFileHandle[i] = filp_open(gcDaiFileName[i], O_RDWR | O_LARGEFILE | O_CREAT, 0);
		if (IS_ERR(gEmuDaiFileHandle[i])) {
			DBG_ERR("unable to open backing file: %s\n", gcDaiFileName[i]);
		}
		file_rw_offset[i] = 0;

		/*
		    handle channel config
		*/
		pAudTsObj[i]->open();
		pAudTsObj[i]->setConfig(AUDTS_CFG_ID_EVENT_HANDLE, (UINT32)pIsrCB[i]);
		if (TsCH == AUDTS_CH_RX) {
			pAudTsObj[i]->setConfig(AUDTS_CFG_ID_TIMECODE_OFFSET, 0);
		}


		gTrigStepVal[i]  = gTriggerVal[i];

		if (TsCH == AUDTS_CH_RX) {
			pAudTsObj[i]->setConfig(AUDTS_CFG_ID_TIMECODE_TRIGGER, gTriggerVal[i]);
		}
		pAudTsObj[i]->setChannel(AudChannel);


		/*
		    handle record buffer queue
		*/
		if (i == AUDTS_CH_RX2) {
			j = 1;
		}

		pAudTsObj[i]->setSamplingRate(sampleRate);
		pAudTsObj[i]->resetBufferQueue(j);



		gEmuDaiAudBufQueue[i].uiAddress   = (gEmuDaiBufferAddr[i] + EmuDaiAddrOfs);
		gEmuDaiAudBufQueue[i].uiSize      = gEmuDaiBufferSize[i];
		loop = 1;
		while ((pAudTsObj[i]->isBufferQueueFull(j) == FALSE) && (loop < TestAUDIO_BUF_BLKNUM)) {
			// Add buffer to queue
			if (pAudTsObj[i]->addBufferToQueue(j, &gEmuDaiAudBufQueue[i]) == FALSE) {
				DBG_ERR("aud_addBufferToQueue RX - %d failed\n", i);
			}
			// Advance to next data buffer
			gEmuDaiAudBufQueue[i].uiAddress = gEmuDaiAudBufQueue[i].uiAddress + gEmuDaiBufferSize[i];//0x40000;
			loop++;
		}

		//gEmuDaiNextFilePos[i]       = 0;
	}

	gEmuDaiNextBufId[i]     = 0;
	gDaiDmaDoneNum[i]       = 0;
	gTimeHitNum[i]          = 0;
	gEmuDaiPreDMADoneNum[i] = 0;


}



BOOL emu_daiHandleTxRxEvent(AUDTS_CH TsCH)
{
	AUDIO_BUF_QUEUE     *pAudioBuf;
	UINT32              fsize2;
	//INT32               fstatus;

	if (TsCH <= AUDTS_CH_TX2) {
#if 1
		while (gEmuDaiPreDMADoneNum[TsCH] != gDaiDmaDoneNum[TsCH]) {

			//if (!msgonoff) {
			//	emu_msg(("t"));
			//}

			gEmuDaiPreDMADoneNum[TsCH]++;

			if (vfs_read(gEmuDaiFileHandle[TsCH], (char *)gEmuDaitBufAddr[TsCH][gEmuDaiNextBufId[TsCH]], gEmuDaiBufferSize[TsCH], &gEmuDaiFileHandle[TsCH]->f_pos) !=	gEmuDaiBufferSize[TsCH]) {
				DBG_ERR("%s: Failed to read file %s", __FUNCTION__, gcDaiFileName[TsCH]);
			}

			gEmuDaiAudBufQueue[TsCH].uiAddress = gEmuDaitBufAddr[TsCH][gEmuDaiNextBufId[TsCH]];

			pAudTsObj[TsCH]->addBufferToQueue(0, &gEmuDaiAudBufQueue[TsCH]);

			gEmuDaiNextFilePos[TsCH] += fsize2;
			if (gEmuDaiNextFilePos[TsCH] >= gEmuDaiTxFileSize[TsCH]) {
				gEmuDaiNextFilePos[TsCH] = 0;
			}

			gEmuDaiNextBufId[TsCH] = (gEmuDaiNextBufId[TsCH] + 1) % TestAUDIO_BUF_BLKNUM;

			//emu_msg(("CH%d Time=%d\r\n",TsCH,Perf_GetDuration()));

			return TRUE;
		}
#endif
	} else if (TsCH <= AUDTS_CH_RX2) {
		AUDTS_CH ChRemap;
		ssize_t			nwritten;
		loff_t			file_offset_tmp;

		ChRemap = TsCH;
		if (ChRemap == AUDTS_CH_RX2) {
			ChRemap = AUDTS_CH_RX;
		}

		while (gEmuDaiPreDMADoneNum[ChRemap] != gDaiDmaDoneNum[ChRemap]) {

			//if (!msgonoff) {
				//#if !AC108DOA_TEST
				//emu_msg(("r"));
				//#endif
			//}

			gEmuDaiPreDMADoneNum[ChRemap]++;
			pAudioBuf = pAudTsObj[ChRemap]->getDoneBufferFromQueue(0);

			if(mode == 3) {
				INT32	angle;
				UINT32	vad_act;
				UINT64  curtime1, curtime2;

				curtime1 = hwclock_get_longcounter();
				vad_act = audlib_doa_run_vad(pAudioBuf->uiAddress);
				curtime2 = hwclock_get_longcounter();
				DBG_DUMP(" %llu (us) ",(curtime2 - curtime1));



				if(vad_act == 100) {
					curtime1 = hwclock_get_longcounter();
					angle = audlib_doa_get_direction();
					curtime2 = hwclock_get_longcounter();
					DBG_DUMP("Ang=%d time=%llu (us)\r\n",angle,(curtime2 - curtime1));
				}
			}



			fsize2 = pAudioBuf->uiValidSize;
			file_offset_tmp = file_rw_offset[ChRemap];
			nwritten = vfs_write(gEmuDaiFileHandle[ChRemap], (char __user *)pAudioBuf->uiAddress, fsize2, &file_offset_tmp);
			file_rw_offset[ChRemap] += nwritten;

			pAudTsObj[ChRemap]->addBufferToQueue(0, pAudioBuf);

			#if 1
			if ((TsCH == AUDTS_CH_RX2) && (gEmuDaiCH[TsCH] == DAI_CH_DUAL_MONO)) {
				pAudioBuf = pAudTsObj[TsCH]->getDoneBufferFromQueue(1);
				fsize2 = pAudioBuf->uiValidSize;
				nwritten = vfs_write(gEmuDaiFileHandle[TsCH], (char __user *)pAudioBuf->uiAddress, fsize2, &file_offset_tmp);
				file_rw_offset[TsCH] += nwritten;

				pAudTsObj[TsCH]->addBufferToQueue(1, pAudioBuf);
			}
			#endif

			return TRUE;
		}
	}

	return FALSE;
}

void emu_daiHandleClose(AUDTS_CH TsCH)
{
	//emu_msg(("%d",TsCH));
	if (pAudTsObj[TsCH]->isBusy()) {
		pAudTsObj[TsCH]->stop();
	}
	//emu_msg(("%d",TsCH));

	if (pAudTsObj[TsCH]->isOpened()) {
		pAudTsObj[TsCH]->close();
	}
	//emu_msg(("%d",TsCH));
}

void emu_daiHandleRxRestSize(AUDTS_CH TsCH)
{
	AUDIO_BUF_QUEUE     *pAudioBuf;
	UINT32              i = 0;
	ssize_t				nwritten;
	loff_t				file_offset_tmp;
	UINT32              fsize2;

	if (TsCH <= AUDTS_CH_TX2) {
		filp_close(gEmuDaiFileHandle[TsCH], NULL);
		return;
	}

	if (TsCH == AUDTS_CH_RX2) {
		if ((gEmuDaiCH[AUDTS_CH_RX2] == DAI_CH_DUAL_MONO)) {
			i = 1;
		} else {
			return;
		}
	}

	// Handle RX rest size
	pAudioBuf = pAudTsObj[TsCH]->getDoneBufferFromQueue(i);
	if (pAudioBuf == NULL) {
		return;
	}

	fsize2 = pAudioBuf->uiValidSize;
	file_offset_tmp = file_rw_offset[TsCH];
	nwritten = vfs_write(gEmuDaiFileHandle[TsCH], (char __user *)pAudioBuf->uiAddress, fsize2, &file_offset_tmp);
	file_rw_offset[TsCH] += nwritten;

	vfs_fsync(gEmuDaiFileHandle[TsCH], 1);
	filp_close(gEmuDaiFileHandle[TsCH],NULL);

	DBG_DUMP("File %s total record size 0x%08X\n",gcDaiFileName[TsCH],(u32)file_rw_offset[TsCH]);

}



static void FullDuplex_Test(AUDIO_SR sampleRate)
{
	AUDIO_VOL           PlayVol    = AUDIO_VOL_63;
	AUDIO_GAIN          record_gain = AUDIO_GAIN_7;
	unsigned long timeout = 0;

	emu_daiConfigureTxRx(AUDTS_CH_TX1,   sampleRate, AUDIO_CH_STEREO);
	emu_daiConfigureTxRx(AUDTS_CH_TX2,   sampleRate, AUDIO_CH_STEREO);

	if (gEmuDaiCH[AUDTS_CH_RX] == DAI_CH_STEREO) {
		emu_daiConfigureTxRx(AUDTS_CH_RX,   sampleRate, AUDIO_CH_STEREO);
	} else if (gEmuDaiCH[AUDTS_CH_RX] == DAI_CH_MONO_LEFT) {
		emu_daiConfigureTxRx(AUDTS_CH_RX,   sampleRate, AUDIO_CH_LEFT);
	} else if (gEmuDaiCH[AUDTS_CH_RX] == DAI_CH_MONO_RIGHT) {
		emu_daiConfigureTxRx(AUDTS_CH_RX,   sampleRate, AUDIO_CH_RIGHT);
	} else {
		emu_daiConfigureTxRx(AUDTS_CH_RX,   sampleRate, AUDIO_CH_STEREO);
	}

	if (gEmuDaiCH[AUDTS_CH_TXLB] == DAI_CH_STEREO) {
		emu_daiConfigureTxRx(AUDTS_CH_TXLB,   sampleRate, AUDIO_CH_STEREO);
	} else if (gEmuDaiCH[AUDTS_CH_TXLB] == DAI_CH_MONO_LEFT) {
		emu_daiConfigureTxRx(AUDTS_CH_TXLB,   sampleRate, AUDIO_CH_LEFT);
	} else if (gEmuDaiCH[AUDTS_CH_TXLB] == DAI_CH_MONO_RIGHT) {
		emu_daiConfigureTxRx(AUDTS_CH_TXLB,   sampleRate, AUDIO_CH_RIGHT);
	} else {
		emu_daiConfigureTxRx(AUDTS_CH_TXLB,   sampleRate, AUDIO_CH_STEREO);
	}

	if (gEmuDaiCH[AUDTS_CH_RX] == DAI_CH_STEREO) {
		emu_daiConfigureTxRx(AUDTS_CH_RX2,   sampleRate, AUDIO_CH_STEREO);
	} else if (gEmuDaiCH[AUDTS_CH_RX] == DAI_CH_MONO_LEFT) {
		emu_daiConfigureTxRx(AUDTS_CH_RX2,   sampleRate, AUDIO_CH_LEFT);
	} else if (gEmuDaiCH[AUDTS_CH_RX] == DAI_CH_MONO_RIGHT) {
		emu_daiConfigureTxRx(AUDTS_CH_RX2,   sampleRate, AUDIO_CH_RIGHT);
	} else {
		emu_daiConfigureTxRx(AUDTS_CH_RX2,   sampleRate, AUDIO_CH_STEREO);
	}

	pAudTsObj[AUDTS_CH_TX1]->setFeature(AUDTS_FEATURE_TIMECODE_HIT,   TRUE);
	pAudTsObj[AUDTS_CH_RX]->setFeature(AUDTS_FEATURE_TIMECODE_HIT,   TRUE);

	aud_set_total_vol_level(AUDIO_VOL_LEVEL64);

	if (gAudSpkrOut) {
		aud_set_output(AUDIO_OUTPUT_SPK);
	} else {
		aud_set_output(AUDIO_OUTPUT_LINE);
	}

	aud_set_volume(PlayVol);
	aud_set_gain(record_gain);


	/*
	 *   Specail test config
	 */

	// RX
	dai_set_rx_config(DAI_RXCFG_ID_PCMLEN,    gEmuDaiPcmLen[AUDTS_CH_RX]);
	dai_set_rx_config(DAI_RXCFG_ID_CHANNEL,   gEmuDaiCH[AUDTS_CH_RX]);

	if (gEmuDaiCH[AUDTS_CH_RX] == DAI_CH_STEREO) {
		dai_set_rx_config(DAI_RXCFG_ID_DRAMCH,  DAI_DRAMPCM_STEREO);
	} else {
		dai_set_rx_config(DAI_RXCFG_ID_DRAMCH,  DAI_DRAMPCM_MONO);
	}

	if ((gEmuDaiCH[AUDTS_CH_RX] != DAI_CH_STEREO) && (gAudExpand[AUDTS_CH_RX])) {
		dai_set_rx_config(DAI_RXCFG_ID_DRAMCH,  DAI_DRAMPCM_STEREO);
	}


	// TXLB
	dai_set_txlb_config(DAI_TXLBCFG_ID_PCMLEN,    gEmuDaiPcmLen[AUDTS_CH_TXLB]);
	dai_set_txlb_config(DAI_TXLBCFG_ID_CHANNEL,   gEmuDaiCH[AUDTS_CH_TXLB]);

	if (gEmuDaiCH[AUDTS_CH_TXLB] == DAI_CH_STEREO) {
		dai_set_txlb_config(DAI_TXLBCFG_ID_DRAMCH,  DAI_DRAMPCM_STEREO);
	} else {
		dai_set_txlb_config(DAI_TXLBCFG_ID_DRAMCH,  DAI_DRAMPCM_MONO);
	}

	if ((gEmuDaiCH[AUDTS_CH_TXLB] != DAI_CH_STEREO) && (gAudExpand[AUDTS_CH_TXLB])) {
		dai_set_txlb_config(DAI_TXLBCFG_ID_DRAMCH,  DAI_DRAMPCM_STEREO);
	}

	// TX1
	dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_PCMLEN,  gEmuDaiPcmLen[AUDTS_CH_TX1]);
	dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_CHANNEL, gEmuDaiCH[AUDTS_CH_TX1]);

	if (gEmuDaiCH[AUDTS_CH_TX1] == DAI_CH_STEREO) {
		dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_DRAMCH,  DAI_DRAMPCM_STEREO);
	} else {
		dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_DRAMCH,  DAI_DRAMPCM_MONO);
	}

	if ((gEmuDaiCH[AUDTS_CH_TX1] != DAI_CH_STEREO) && (gAudExpand[AUDTS_CH_TX1])) {
		dai_set_tx_config(DAI_TXCH_TX1, DAI_TXCFG_ID_CHANNEL,   DAI_CH_STEREO);
	}

	// TX2
	dai_set_tx_config(DAI_TXCH_TX2, DAI_TXCFG_ID_PCMLEN,  gEmuDaiPcmLen[AUDTS_CH_TX2]);
	dai_set_tx_config(DAI_TXCH_TX2, DAI_TXCFG_ID_CHANNEL, gEmuDaiCH[AUDTS_CH_TX2]);

	if (gEmuDaiCH[AUDTS_CH_TX2] == DAI_CH_STEREO) {
		dai_set_tx_config(DAI_TXCH_TX2, DAI_TXCFG_ID_DRAMCH,  DAI_DRAMPCM_STEREO);
	} else {
		dai_set_tx_config(DAI_TXCH_TX2, DAI_TXCFG_ID_DRAMCH,  DAI_DRAMPCM_MONO);
	}

	if ((gEmuDaiCH[AUDTS_CH_TX2] != DAI_CH_STEREO) && (gAudExpand[AUDTS_CH_TX2])) {
		dai_set_tx_config(DAI_TXCH_TX2, DAI_TXCFG_ID_CHANNEL,   DAI_CH_STEREO);
	}

	#if 0
	// User must set WM8976 to 32bits per word for this testing
	// WM8976: audcodec_init:: line-375
	dai_set_i2s_config(DAI_I2SCONFIG_ID_CLKRATIO, DAI_I2SCLKR_256FS_64BIT);
	#endif

	//emu_msg(("START (1)TX1  (2)TX2  (3)RX  (4)TXLB\r\n"));
	//emu_msg(("Pause (6)TX1  (7)TX2  (8)RX  (9)TXLB\r\n"));
	//emu_msg(("Resume(q)TX1  (w)TX2  (e)RX  (r)TXLB\r\n"));
	//emu_msg(("HDMI-Audio    (m)SET  (n)Clear\r\n"));
	//emu_msg(("AudioPlayback Volume  (a)up  (s)Down\r\n"));
	//emu_msg(("AudioPlayback D-Gain  (j)up  (k)Down\r\n"));
	//emu_msg(("AudioRecord   Volume  (d)up  (f)Down\r\n"));
	//emu_msg(("AudioRecord   GainDB  (g)up  (h)Down\r\n"));
	//emu_msg(("Toggle ADC PatGen EN  (o)\r\n"));
	//emu_msg(("Toggle DAC PatGen EN  (p)\r\n"));
	//emu_msg(("Change HDMI CH SEL    (t)\r\n"));

	if(mode==3) {
		aud_set_gaindb(31);
		pAudTsObj[AUDTS_CH_RX]->record();
	} else {
		pAudTsObj[AUDTS_CH_TX1]->playback();
		pAudTsObj[AUDTS_CH_TXLB]->record();
		pAudTsObj[AUDTS_CH_RX]->record();
		pAudTsObj[AUDTS_CH_TX2]->playback();
	}
	timeout = jiffies + msecs_to_jiffies(1000*max_time);

	while (1) {
		emu_daiHandleTxRxEvent(AUDTS_CH_RX2);
		emu_daiHandleTxRxEvent(AUDTS_CH_TXLB);
		emu_daiHandleTxRxEvent(AUDTS_CH_TX1);
		emu_daiHandleTxRxEvent(AUDTS_CH_TX2);

		if (time_after(jiffies, timeout)) {
			break;
		}

		msleep(10);

		/*
			move the dynamic settings to proc
		*/
	}

#if 1

	// Stop TXLB and RX first before TX1/TX2 to prevent data compare mismatch at file end
	emu_daiHandleClose(AUDTS_CH_RX);
	emu_daiHandleClose(AUDTS_CH_TXLB);
	emu_daiHandleClose(AUDTS_CH_TX2);
	emu_daiHandleClose(AUDTS_CH_TX1);
	emu_msg(("obj close done\r\n"));


	emu_daiHandleRxRestSize(AUDTS_CH_RX);
	emu_daiHandleRxRestSize(AUDTS_CH_TXLB);
	emu_daiHandleRxRestSize(AUDTS_CH_RX2);
	emu_daiHandleRxRestSize(AUDTS_CH_TX1);
	emu_daiHandleRxRestSize(AUDTS_CH_TX2);
#endif
}

void audio_filter_test(AUDIO_SR sampleRate)
{
	struct 	kstat 	stat;
	int 			error = 0;
	loff_t 			logstrs_size;
	UINT32 			buf1,buf2;
	CHAR 			input_name[]  = "/mnt/sd/test_in.pcm";
	CHAR 			output_name[] = "/mnt/sd/test_out.pcm";
	struct 	file* 	input_handle;
	struct 	file* 	output_handle;
	ssize_t 		file_size;
	loff_t			file_offset_tmp;
	UINT32     		seconds;
	UINT64          curtime1, curtime2;
	long			period;
	UINT32			perf;

	buf1 = gEmuDaiBufferAddr[0];
	logstrs_size = 0x1800000;
	buf2 = buf1+logstrs_size;

	seconds = ((UINT32)logstrs_size)/sampleRate/4;
	logstrs_size = seconds*sampleRate*4;
	logstrs_size = (logstrs_size & ~0x3FF);

	/*
		Read Input File
	*/

	input_handle = filp_open(input_name, O_RDONLY, 0);
	if (IS_ERR(input_handle)) {
		DBG_ERR("unable to open backing file: %s\n", input_name);
		return;
	}
	error = vfs_stat(input_name, &stat);
	if (error) {
		DBG_ERR("%s: Failed to stat file %s \n", __FUNCTION__, input_name);
		return;
	}

	file_size = vfs_read(input_handle, (char *)buf1, logstrs_size, &input_handle->f_pos);
	filp_close(input_handle, NULL);

	DBG_DUMP("%s: file %s size is 0x%08X ReadSize=0x%08X seconds=%d\n", __FUNCTION__, input_name, (UINT32) stat.size, (UINT32)file_size,seconds);

	/*
		Apply Testing
	*/

	if(mode == 1)
	{
		AUDFILT_BITSTREAM   AudBitStream;
		AUDFILT_INIT    	FiltInit;

		FiltInit.filt_ch     = AUDFILT_CH_STEREO;
		FiltInit.smooth_enable  = ENABLE;

		DBG_DUMP("START audio filter IIR test(%d): \n",mode);

		audlib_filt_open(&FiltInit);
		audlib_filt_init();

		if (!audlib_filt_set_config(AUDFILT_SEL_FILT0, &bandstop_filt)) {
			emu_msg(("audFilt Config ERROR! \r\n"));
		}

		audlib_filt_enable_filt(AUDFILT_SEL_FILT0, TRUE);

		AudBitStream.bitstram_buffer_in     =   buf1;
		AudBitStream.bitstram_buffer_out    =   buf1;
		AudBitStream.bitstram_buffer_length     =   (UINT32)logstrs_size;

		curtime1 = hwclock_get_longcounter();
		if (!audlib_filt_run(&AudBitStream)) {
			emu_msg(("audFilt Filtering FAIL!\r\n"));
		}
		curtime2 = hwclock_get_longcounter();

		audlib_filt_close();
	}else if (mode ==2) {
		AGC_BITSTREAM AgcIO;

		DBG_DUMP("START audio AGC test(%d): \n",mode);

		audlib_agc_open();

		audlib_agc_set_config(AGC_CONFIG_ID_SAMPLERATE, (INT32)sampleRate);
		audlib_agc_set_config(AGC_CONFIG_ID_CHANNEL_NO, 2);
		audlib_agc_set_config(AGC_CONFIG_ID_TARGET_LVL, AGC_DB(-12));
		audlib_agc_set_config(AGC_CONFIG_ID_MAXGAIN, 	AGC_DB(12));
		audlib_agc_set_config(AGC_CONFIG_ID_MINGAIN,    AGC_DB(-12));
		audlib_agc_set_config(AGC_CONFIG_ID_NG_THD,     AGC_DB(-35));

		audlib_agc_set_config(AGC_CONFIG_ID_MSGDUMP,    ENABLE);

		audlib_agc_init();

		AgcIO.bitstram_buffer_in        = buf1;
		AgcIO.bitstram_buffer_out       = buf1;
		AgcIO.bitstram_buffer_length        = seconds * sampleRate;

		curtime1 = hwclock_get_longcounter();
		audlib_agc_run(&AgcIO);
		curtime2 = hwclock_get_longcounter();

		audlib_agc_close();

	}


	period = (curtime2 - curtime1)*1000000;
	perf = (UINT32)CPU_FREQ*(UINT32)period/(UINT32)seconds;
	printk("Time = %ld us; Perf = %d Hertz\n", period,perf);


	/*
		Write File Output
	*/

	output_handle = filp_open(output_name, O_RDWR | O_LARGEFILE | O_CREAT, 0);
	if (IS_ERR(output_handle)) {
		DBG_ERR("unable to open backing file: %s\n", output_name);
	}

	file_offset_tmp = 0;
	vfs_write(output_handle, (char __user *)buf1, (UINT32)logstrs_size, &file_offset_tmp);

	vfs_fsync(output_handle, 1);
	filp_close(output_handle, NULL);

}


#endif

int nvt_audlib_emu_thread(void * __audlib_emu)
{
	if(mode == 3) {
		DBG_DUMP("To Test DOA 2CH change AC108.c 's MIC location mapping<ac108.c around line484>.\n");
		// This is TDM2CH DOA test
		InitAudio(EMUDAI_CODEC_TYPE_EXT);
	} else {
		InitAudio(EMUDAI_CODEC_TYPE_EAC);
	}

	if (mode == 0) {
		FullDuplex_Test(AUDIO_SR_48000);
	} else if ((mode == 1)||(mode == 2)) {
		audio_filter_test(AUDIO_SR_48000);
	} else if (mode == 3) {
    	frammap_buf_t       BufTx = {0};

	    BufTx.size = 0x100000;
	    BufTx.align = 64;
	    BufTx.name = "adoa";
	    BufTx.alloc_type = ALLOC_CACHEABLE;
	    frm_get_buf_ddr(DDR_ID0, &BufTx);
		audlib_doa_set_config(AUDDOA_CONFIG_ID_BUFFER_ADDR, (UINT32)BufTx.va_addr);
		audlib_doa_set_config(AUDDOA_CONFIG_ID_BUFFER_SIZE, 0x100000);

		audlib_doa_close();
		audlib_doa_open();

		audlib_doa_set_config(AUDDOA_CONFIG_ID_CHANNEL_NO,  2);

		audlib_doa_set_config(AUDDOA_CONFIG_ID_INDEX_MIC0,		0);
		audlib_doa_set_config(AUDDOA_CONFIG_ID_INDEX_MIC1,		1);
		audlib_doa_set_config(AUDDOA_CONFIG_ID_INDEX_MIC2,		2);
		audlib_doa_set_config(AUDDOA_CONFIG_ID_INDEX_MIC3,		3);

		audlib_doa_set_config(AUDDOA_CONFIG_ID_SAMPLERATE,		AUDIO_SR_16000);//Samplerate
		audlib_doa_set_config(AUDDOA_CONFIG_ID_MIC_DISTANCE,	AUDDOA_MIC_DISTANCE(0.08127));//respeaker mic distance 8.127 cm

		audlib_doa_set_config(AUDDOA_CONFIG_ID_OPERATION_SIZE, 	AUDIO_SR_16000/3);//Samplerate

		audlib_doa_set_config(AUDDOA_CONFIG_ID_FRAME_SIZE, 		1024);
		audlib_doa_set_config(AUDDOA_CONFIG_ID_AVERGARE, 		2);//1~4

		audlib_doa_set_config(AUDDOA_CONFIG_ID_VAD_SENSITIVITY, 8);

		//audlib_doa_set_config(AUDDOA_CONFIG_ID_DBG_MSG, 		ENABLE);
		audlib_doa_init();

		msgonoff = 0;

		FullDuplex_Test(AUDIO_SR_16000);

		frm_free_buf_ddr(BufTx.va_addr);

	}

	frm_free_buf_ddr((void *)gEmuDaiBufferAddr[0]);

	DBG_WRN("nvt_audlib_emu_thread thread closed\n");
	return 0;

}

