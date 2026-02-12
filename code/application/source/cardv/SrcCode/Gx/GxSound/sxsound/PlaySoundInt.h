/**
    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.

    @file       PlaySoundInt.h
    @ingroup    mIPRJAPKey

    @brief      Internal header file of Play Sound Task
                Internal Header file of task handles the sound playback of startup, keypad ...

    @note       Nothing.

    @date       2006/01/23
*/

/** \addtogroup mIPRJAPKey */
//@{

#ifndef _PLAYSOUNDINT_H
#define _PLAYSOUNDINT_H

#include "../GxSoundIntr.h"
#include "GxSound.h"
#include "vendor_audioout.h"


#define FLGSOUND_STOP               0x00000001
#define FLGSOUND_PLAY               0x00000002
#define FLGSOUND_STOPPED            0x00000004
#define FLGSOUND_QUIT               0x00000008
#define FLGSOUND_EXIT               0x00000010
#define FLGSOUND_ALL                0xFFFFFFFF

#define FLGDATA_STOP               0x00000001
#define FLGDATA_PLAY               0x00000002
#define FLGDATA_STOPPED            0x00000004
#define FLGDATA_QUIT               0x00000008
#define FLGDATA_EXIT               0x00000010
#define FLGDATA_ALL                0xFFFFFFFF

#define PLAYSOUND_STS_STOPPED       0
#define PLAYSOUND_STS_PLAYING       1

#define GXSOUND_NONTBL_SND_ID           0xFFFFFFFF
#define GXSOUND_WAV_HEADER_RIFF         0x46464952
#define GXSOUND_WAV_RIFFTYPE_WAVE       0x45564157
#define GXSOUND_WAV_FORMAT_ID           0x20746D66

/**
    PCM Wave Header

    @note Size is 44 bytes
*/
typedef struct {
	UINT32      uiHeaderID;         ///< Wave Header
	///< @note Should be "RIFF" (0x46464952)
	UINT32      uiHeaderSize;       ///< Header Size
	///< @note = FileSize - 8 or DataChunkSize + 36 + UserDataSize
	UINT32      uiRIFFType;         ///< RIFF Type
	///< @note Should be "WAVE" (0x45564157)
	UINT32      uiFormatID;         ///< Format Chunk ID
	///< @note Should be "fmt " (0x20746D66)
	UINT32      uiFormatSize;       ///< Format Chunk Size
	///< @note Should be 0x00000010
	UINT16      uiCompressionCode;  ///< Compression Code
	///< @note Should be 0x0001 for PCM format WAV file
	UINT16      uiNumOfChannels;    ///< Number Of Channels
	///< - @b 0x0001: Mono
	///< - @b 0x0002: Stereo
	UINT32      uiSamplingRate;     ///< Sampling rate
	UINT32      uiAvgBytePerSec;    ///< Average Byte Per Second
	///< @note = Sampling Rate * Block Align
	UINT16      uiBlockAlign;       ///< Block Align
	///< @note = Significant Bits Per Sample * Number Of Channles / 8
	UINT16      uiSigBitsPerSample; ///< Significant Bits Per Sample
	///< @note Should be 0x0010 or 0x0008, PCM is 8/16 bits per sample
	UINT32      uiDataID;           ///< Data Chunk ID
	///< @note Should be "data" (0x61746164)
	UINT32      uiDataSize;         ///< Data Chunk Size
	///<
	///< Data byte count
} WAV_PCMHEADER, *PWAV_PCMHEADER;

extern HD_PATH_ID gxsound_ctrl_id;
extern HD_PATH_ID gxsound_path_id;
extern SOUND_MEM gxsound_mem;

extern void PlaySound_AudioHdl(UINT32 uiEventID);
extern void GxSound_ResetParam(void);
extern ER GxSound_TxfWav2PCM(SOUND_DATA *pSndData);
extern void _GxSound_Stop(void);
extern void _GxSound_Play(SOUND_DATA *pSoundData);
extern THREAD_RETTYPE PlaySoundTsk(void);
extern THREAD_RETTYPE PlayDataTsk(void);

#endif

//@}
