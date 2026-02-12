#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

/*
    Parse AVI file (include index table).
    This is internal API.

    @param uiAVIFileSZ AVI file size (0 is special case, it means the size will be got through file system)
    @return iErrChk (DECODE_JPG_READERROR/DECODE_JPG_DECODEERROR or DECODE_JPG_PRIMARY/DECODE_JPG_AVIOK)
*/
INT32 PB_ParseAVIHeader(UINT32 AVIStartAddr)
{
	#if _TODO
	UINT16    *pdata, *pdata2, ChunkName;
	UINT32    FinishSearch, ChunkSize, tmpsize;
	PPLAY_WAVEHEAD_INFO    pWavHeader;
	PLAY_AVIHEAD_INFO *pAviParams;


	pAviParams = (PLAY_AVIHEAD_INFO *)&gPlayAVIHeaderInfo;
	pdata = (UINT16 *)AVIStartAddr;
	pAviParams->ucIsWithAudio = 0;
	pAviParams->ucIsWithVideo = 0;

	// "RI"FF
	if (*pdata != 0x4952) {
		return PLAYAVI_ERRHEADER;
	}
	pdata += 2;
	// RIFF-size
	ChunkSize = *pdata++;
	ChunkSize = ChunkSize | (*pdata++ << 16);
	pAviParams->uiTotalFileSize = ChunkSize + 8;    // "RIFF" & "RIFF-size" 8 bytes
	// "WAVE" or "AVI "
	ChunkName = *pdata;
	pdata += 2;
	if (ChunkName == 0x5641) {
		// AVI
		FinishSearch = 0;
		while (!FinishSearch) {
			ChunkName = *pdata;
			pdata += 2;
			ChunkSize = *pdata++;
			ChunkSize = ChunkSize | (*pdata++ << 16);
			if (ChunkName == 0x494C) {               // "LIST"
				ChunkName = *pdata;
				ChunkSize -= 4;
				pdata += 2;
				if (ChunkName == 0x6468) {           // "hdrl"
					while (ChunkSize) {
						ChunkName = *pdata;
						ChunkSize -= 4;
						pdata += 2;
						if (ChunkName == 0x494C) {           // "LIST"
							tmpsize = *pdata++;
							tmpsize = tmpsize | (*pdata++ << 16);
							ChunkSize -= (tmpsize + 4);
							pdata2 = pdata;
							pdata += (tmpsize >> 1); // SHORT
							pdata2 += 6;                    // skip "strl","strh", size
							ChunkName = *pdata2;
							pdata2 += 2;
							if (ChunkName == 0x6976) {       // "vids"
								if ((*pdata2 != 0x6A6D)    && (*pdata2 != 0x4A4D)) {   // "mjpg" & "MJPG"
									return PLAYAVI_ERRHEADER;
								}
								pdata2 += 8;
								pAviParams->ucIsWithVideo = PLAYAVI_VIDEOSUPPORT;
								tmpsize = *pdata2++;
								tmpsize = tmpsize | (*pdata2++ << 16);
								pAviParams->uiVidScale = tmpsize;
								tmpsize = *pdata2++;
								tmpsize = tmpsize | (*pdata2++ << 16);
								pAviParams->uiVidRate = tmpsize;
							} else if (ChunkName == 0x7561) { // "auds"
								pAviParams->ucIsWithAudio = PLAYAVI_AUDIOSUPPORT;
								pdata2 += 8;
								pAviParams->uiAudiRate = *(UINT32 *)(pdata2 + 2) / *(UINT32 *)pdata2;  // audio sample rate
								pdata2 += 18;
								if (*(UINT32 *)pdata2 != 0x66727473) {   // "strf"
									return PLAYAVI_ERRHEADER;
								}
								pdata2 += 5;
								pAviParams->ucAudChannels = *pdata2;
								pdata2 += 3;
								pAviParams->uiAudBytesPerSec = *(UINT32 *)pdata2;
								pdata2 += 3;
								pAviParams->ucAudBitsPerSample = *pdata2;
							}
						} else if (ChunkName == 0x7661) {         // "avih"
							tmpsize = *pdata++;
							tmpsize = tmpsize | (*pdata++ << 16);
							pdata2 = pdata;
							ChunkSize -= (tmpsize + 4);
							pdata += (tmpsize >> 1);   // SHORT
							pdata2 += 8;            // skip (frame-rate),(max-byte),(reserved),(flags)
							tmpsize = *pdata2++;    // point to (total-frames)
							tmpsize = tmpsize | (*pdata2 << 16);
							pAviParams->uiTotalFrames = tmpsize;
						} else {                                // "IDIT"... others
							tmpsize = *pdata++;
							tmpsize = tmpsize | (*pdata++ << 16);
							ChunkSize -= (tmpsize + 4);
							pdata += (tmpsize >> 1); // SHORT
						}

					}
				}    // End of if( ChunkName == 0x6468 )            // "hdrl"

				else if (ChunkName == 0x6F6D) {       // "movi"
					FinishSearch = 1;
				}    // End of else if( ChunkName == 0x6F6D )        // "movi"
				else {
					pdata += (ChunkSize >> 1);
				}
			} else { //if( ChunkName == 0x554A )        // "JUNK"
				pdata += (ChunkSize >> 1);
			}

		}    // End of while( !FinishSearch )

	}

	else if (ChunkName == 0x4157) {
		// WAVE
		pAviParams->ucIsWithAudio = PLAYAVI_AUDIOSUPPORT;
		pdata -= 6;
		pWavHeader = (PPLAY_WAVEHEAD_INFO)pdata;
		pAviParams->uiAudiRate                 = pWavHeader->uiWavSamplesPerSec;    // audio sample rate
		pAviParams->ucAudChannels             = pWavHeader->usWavChannels;
		pAviParams->uiAudBytesPerSec         = pWavHeader->uiWavAvgBytesPerSec;
		pAviParams->ucAudBitsPerSample        = pWavHeader->usWavBitsPerSample;
		//pAviParams->AudioStartAddr             = (UINT32)pdata + sizeof(PLAY_WAVEHEAD_INFO);
		pAviParams->uiAudWavTotalSamples    = (pWavHeader->uiWavDatChunkSize) / ((pWavHeader->usWavBitsPerSample >> 3) * (pWavHeader->usWavChannels));
	} else {
		return PLAYAVI_ERRHEADER;
	}

	if (pAviParams->ucIsWithAudio && pAviParams->ucIsWithVideo) {
		// have audio and video
		//pAviParams->uiToltalSecs = pAviParams->uiTotalFrames / ((pAviParams->uiVidRate/pAviParams->uiVidScale) + 1);    // +1 for audio
		pAviParams->uiToltalSecs = pAviParams->uiTotalFrames / ((pAviParams->uiVidRate / pAviParams->uiVidScale));
	} else if (pAviParams->ucIsWithAudio) {
		// audio only wav
		pAviParams->uiToltalSecs = (pAviParams->uiAudWavTotalSamples / pAviParams->uiAudiRate); // One audio frame per second
	} else {
		// video only
		pAviParams->uiToltalSecs = pAviParams->uiTotalFrames / ((pAviParams->uiVidRate) / (pAviParams->uiVidScale));
	}

	return PLAYAVI_OK;
	#else
	return 0;
	#endif
}
