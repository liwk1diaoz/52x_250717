/*
    Copyright   Novatek Microelectronics Corp. 2004.  All rights reserved.

    @file       jpgparseheader.c
    @ingroup    mIAVJPEG

    @brief      Jpeg parser header

*/
#if 0
#include <string.h>
#include "JpgHeader.h"
#include "JpgDec.h"
#include "Exif.h"
#include "Utility.h"
#include "ExifDef.h"
#include "jpgint.h"
#else
#include "jpg_int.h"
#endif
#include "../jpeg_platform.h"

#ifndef NULL
#define NULL        ((void*)0)
#endif

//to do
//UINT8 GetCurByte(UINT8 **addr);
//extern UINT16 GetCurSHORT(UINT8 **buf_p, BOOL Swap);
/** \addtogroup mIAVJPEG
*/
//@{

UINT32    RIFF2FFD8_Size;

/**
    Parse EXIF Information.

    These elements are filled with EXIF information in JPEG header parsing.
*/
typedef struct {
	UINT32          OffsetQTabY;
	UINT32          OffsetQTabUV;
	UINT32          ThumbExifHeaderSize;
	UINT32          ThumbJPGHeaderSize;
	UINT32          OffsetHiddenJPG;
	UINT32          OffsetHiddenQTabY;
	UINT32          OffsetHiddenQTabUV;
	UINT32          ThumbJPGSize;
	UINT32          ScreeNailSize;
} EXIFHeaderInfo, *PEXIFHeaderInfo;

//
// This variable can be put in function jpg_parse_header
//
static EXIFHeaderInfo  EXIFData = {0};

#if 0
//sample code for exif parser callback
JPG_HEADER_ER parse_exif_cb(UINT32 buf, UINT32 *p_thumboffset, UINT32 *p_thumbsize )
{
	EXIF_ER readexif;
	EXIF_GETTAG exifTag;
	BOOL bIsLittleEndian = FALSE;
	MEM_RANGE ExifData;
	UINT32 uiTiffOffsetBase = 0;

	ExifData.Addr = buf;
	ExifData.Size = MAX_APP1_SIZE;
	readexif = EXIF_ParseExif(EXIF_HDL_ID_1, &ExifData);
	if (readexif) {
		JPG_HEADER_ER result;
		//DBG_ERR("ReadExif fails...0x%x!\r\n", (unsigned int)(readexif));
		switch (readexif) {
		case EXIF_ER_SOI_NF:
			result = JPG_HEADER_ER_SOI_NF;
			break;
		case EXIF_ER_APP1_NF:
			result = JPG_HEADER_ER_APP1;
			break;
		default:
			result = JPG_HEADER_ER_MARKER;
			break;
		}
		return result;
	}
	EXIF_GetInfo(EXIF_HDL_ID_1, EXIFINFO_BYTEORDER, &bIsLittleEndian, sizeof(bIsLittleEndian));
	EXIF_GetInfo(EXIF_HDL_ID_1, EXIFINFO_TIFF_OFFSET_BASE, &uiTiffOffsetBase, sizeof(uiTiffOffsetBase));

	exifTag.uiTagIfd = EXIF_IFD_1ST;
	exifTag.uiTagId = TAG_ID_INTERCHANGE_FORMAT_LENGTH;
	if (EXIF_ER_OK == EXIF_GetTag(EXIF_HDL_ID_1, &exifTag)) {
		*p_thumbsize = Get32BitsData(exifTag.uiTagDataAddr, bIsLittleEndian);;
	} else {
		*p_thumbsize = 0;
		return JPG_HEADER_ER_APP1;
	}
	exifTag.uiTagIfd = EXIF_IFD_1ST;
	exifTag.uiTagId = TAG_ID_INTERCHANGE_FORMAT;
	if (EXIF_ER_OK == EXIF_GetTag(EXIF_HDL_ID_1, &exifTag)) {
		*p_thumboffset = Get32BitsData(exifTag.uiTagDataAddr, bIsLittleEndian) + uiTiffOffsetBase;
	} else {
		DBG_ERR("No Thumbnail!\r\n");
		return JPG_HEADER_ER_APP1;
	}
	return JPG_HEADER_ER_OK;
}
#endif
/**
    Check the image format if supported.

    Check the image format if supported.

    @param[in] comp_num - Component number
    @param[in] scan_freq[3][2] - Component format
    @param[out] p_file_format - Supported format
    @return result
        - @b FALSE - Format not support
        - @b TRUE - Format support
*/
BOOL jpg_supported_format(UINT16 comp_num, UINT16 scan_freq[3][2], JPG_YUV_FORMAT *p_file_format)
{
	BOOL bResult = TRUE;

	if (comp_num == 3) { // Y,U,V
		if ((scan_freq[0][0] == 2) && (scan_freq[0][1] == 1)) {
			if ((scan_freq[1][0] == 1) && (scan_freq[1][1] == 1)) {
				*p_file_format = JPG_FMT_YUV211; // 0x21 0x11 0x11 -> YUV211
			} else {
				bResult = FALSE;
			}
		} else if ((scan_freq[0][0] == 2) && (scan_freq[0][1] == 2)) {
			if ((scan_freq[1][0] == 1) && (scan_freq[1][1] == 1)) {
				*p_file_format = JPG_FMT_YUV420; // 0x22 0x11 0x11 -> YUV420
			} else if ((scan_freq[1][0] == 1) && (scan_freq[1][1] == 2)) {
				*p_file_format = JPG_FMT_YUV422; // 0x22 0x12 0x12 -> YUV422V
			} else {
				bResult = FALSE;
			}
		} else if ((scan_freq[0][0] == 1) && (scan_freq[0][1] == 2)) {
			bResult = FALSE;
		} else {
			bResult = FALSE;
		}
	} else {
		bResult = TRUE;
	}

	return bResult;
}



/**
    Gets a 8-bit data.

    Gets a 8-bit data from the specified address and then the address will point to next byte.

    @param[in,out] addr The address of data address.
    @return A 8-bit data.
*/
static UINT8 GetCurByte(UINT8 **addr)
{
	UINT8 getdata;

	getdata = **addr;
	*addr += 1;
	return (getdata);
}

/**
    Gets a 16-bit data.

    Gets a 16-bit data from the specified address and then the address will point to next 16-bit data.

    @param[in,out] buf_p The address of data address.
    @param[in] Swap      Byte Order.
                            - @b TRUE: Little endian.
                            - @b FALSE: Big endian.
    @return A 16-bit data.
*/
static UINT16 GetCurSHORT(UINT8 **buf_p, BOOL Swap)
{
	UINT16  data;
	if (Swap == 0) {
		data = (GetCurByte(buf_p) << 8);
		data |= GetCurByte(buf_p);
	} else { //if(Swap==0)
		data = GetCurByte(buf_p);
		data |= (GetCurByte(buf_p) << 8);
	}
	return data;
}

JPG_HEADER_ER jpg_parse_header(JPGHEAD_DEC_CFG *jdcfg_p, JPG_DEC_TYPE dec_type, PARSE_EXIF_CB parse_exif_cb)
{
	UINT8 *buf_p, cbyte, index, *pucStarAddr;
	UINT32 headerlen, i, temp, total;
	INT32 ms_len;
	//AVIHEAD_INFO JpgDec_AVIHeaderInfo;
	BOOL bSOFflag = FALSE, bDHTflag = FALSE;
//#NT#2010/06/03#Scottie -begin
//#NT#Support QTable index2
//    UINT8   *pQTabIndex0 = NULL, *pQTabIndex1 = NULL;
	UINT8 *pQTabIndex0 = NULL, *pQTabIndex1 = NULL, *pQTabIndex2 = NULL;
//#NT#2010/06/03#Scottie -end
	JPG_HEADER_ER Result;
	//jdcfg_p->pExifInfo = (PEXIFHeaderInfo)&EXIFData;
	pucStarAddr = buf_p = jdcfg_p->inbuf;
	jdcfg_p->headerlen = 0;
	headerlen = 0;
	jdcfg_p->p_huff_dc0th = (UINT8 *)&JPGHeader.mark_dht.huff_dc_0th.symbol_nums[0];
	jdcfg_p->p_huff_ac0th = (UINT8 *)&JPGHeader.mark_dht.huff_ac_0th.symbol_nums[0];
	jdcfg_p->p_huff_dc1th = (UINT8 *)&JPGHeader.mark_dht.huff_dc_1th.symbol_nums[0];
	jdcfg_p->p_huff_ac1th = (UINT8 *)&JPGHeader.mark_dht.huff_ac_1th.symbol_nums[0];
	jdcfg_p->rstinterval = 0;

	//------------------------------------------------------
	// Parse_Exif() will modify:
	//          jdcfg_p->headerlen = 2+2+APP1Size
	//          buf_p = point to primary-image "FFD8"
	//------------------------------------------------------
	if (dec_type == DEC_THUMBNAIL) {
		if (parse_exif_cb == NULL) {
			DBG_ERR("No exif parser handler!\r\n");
			return JPG_HEADER_ER_APP1;
		}
		Result = parse_exif_cb((UINT32)jdcfg_p->inbuf, &EXIFData.ThumbExifHeaderSize, &EXIFData.ThumbJPGSize);
		if (Result  != JPG_HEADER_ER_OK) {
			return Result;
		}
		headerlen = EXIFData.ThumbExifHeaderSize;
		buf_p += headerlen;

	}
#if 0
	else if (dec_type == DEC_AVI) {
		if (AVIUti_ParseHeaderWithTargetSize(g_uiAVIFileSZ, (UINT32)jdcfg_p->inbuf, &JpgDec_AVIHeaderInfo, FST_READ_THUMB_BUF_SIZE + FST_READ_SOMEAVI_SIZE) != AVI_PARSEOK) {
			return (JPG_HEADER_ER_AVI_NF);
		}

		jdcfg_p->headerlen = (UINT32)JpgDec_AVIHeaderInfo.uiFirstVideoOffset + 8;
		buf_p = jdcfg_p->inbuf + jdcfg_p->headerlen;
		headerlen = jdcfg_p->headerlen;
		RIFF2FFD8_Size = jdcfg_p->headerlen;
	}
#endif
	//#NT#2012/12/21#Ben Wang -begin
	//#NT#The mechanism of DEC_HIDDEN should be the same with DEC_PRIMARY.
#if 0
	// decode hidden thumb
	else if (dec_type == DEC_HIDDEN) {
		if (EXIFData.OffsetHiddenJPG != 0) {
			// hidden JPG
//#NT#2008/12/03#Scottie -begin
//#NT#Modify for new Screennail decoding mechanism
			if ((EXIFData.ScreeNailSize < 0x00070000) &&
				(jdcfg_p->speedup_sn)) {
				pucStarAddr = buf_p = jdcfg_p->inbuf;
			} else
//#NT#2008/12/03#Scottie -end
			{
				buf_p = jdcfg_p->inbuf + EXIFData.OffsetHiddenJPG;
				headerlen = EXIFData.OffsetHiddenJPG;
			}
		} else {
			return (JPG_HEADER_ER_SOI_NF);
		}
	}
	//#NT#2012/12/21#Ben Wang -end
	else if (dec_type == DEC_MOVMJPG) {
		jdcfg_p->headerlen = (UINT32)0x3E90;
		buf_p = jdcfg_p->inbuf + jdcfg_p->headerlen;
		headerlen = jdcfg_p->headerlen;
		RIFF2FFD8_Size = 0x3E90;
		DBG_MSG("headerlen=%lx, buf_p=%lx, RIFF2FFD8_Size=%lx,", (unsigned long)(jdcfg_p->headerlen), (unsigned long)(buf_p), (unsigned long)(RIFF2FFD8_Size));
	}
#endif
	//if( (GetCurByte(&buf_p) != 0xFF) || (GetCurByte(&buf_p) != 0xD8) )
	if (JPEG_MARKER_SOI != GetCurSHORT(&buf_p, 1)) {
		UINT32 j, temp;

		buf_p -= 2;
		//filter unexpected data
		for (j = 0; j < JPG_HEADER_SIZE; j++) {
			if (GetCurByte(&buf_p) != 0xFF) {
				continue;
			} else {
				//in the case of "FF FF"
				if (*(buf_p) == 0xFF) {
					continue;
				}
			}
			temp = GetCurByte(&buf_p);
			if (temp == 0xDB || temp == 0xC0 || temp == 0xC4) {
				//let buf_p pointer to standard "FF XX"
				buf_p -= 2;
				break;
			} else {
				return (JPG_HEADER_ER_MARKER);
			}
		}
		//headerlen += j;
		headerlen = buf_p - jdcfg_p->inbuf;
		//return(JPG_HEADER_ER_SOI_NF);      // no SOI marker at start of bitstream
	} else {
		headerlen += 2;
	}

	while (GetCurByte(&buf_p) == 0xFF) {
		cbyte = GetCurByte(&buf_p);
		// marker segment length
		ms_len = GetCurByte(&buf_p) << 8;
		ms_len |= GetCurByte(&buf_p);
		ms_len -= 2;
		headerlen += 4;

		if ((cbyte <= 0xbf)
			|| (cbyte >= 0xf0 && cbyte <= 0xfd)
			|| (cbyte >= 0xc1 && cbyte <= 0xc3)
			|| (cbyte >= 0xc5 && cbyte <= 0xd7)
			|| cbyte == 0xdc || cbyte == 0xde || cbyte == 0xdf) {
			return (JPG_HEADER_ER_MARKER);
		}

		switch (cbyte) {
		case 0xDB:      // DQT marker
			headerlen += ms_len;
			while (ms_len) {
				// decode hidden thumb
				index = GetCurByte(&buf_p);
				if (index == 0x01) {        // 1th-Qtable
//#NT#2008/12/03#Scottie -begin
//#NT#Use pucStarAddr instead of jdcfg_p->inbuf for new Screennail decoding mechanism
					pQTabIndex1 = buf_p;
				} else if (index == 0x00) { // 0th-Qtable
//#NT#2008/12/03#Scottie -end
					pQTabIndex0 = buf_p;
				}
//#NT#2010/06/03#Scottie -begin
//#NT#Support Quantization table 2
				else if (index == 0x02) {
					pQTabIndex2 = buf_p;
				}
//#NT#2010/06/03#Scottie -end
				buf_p  += 64;
				ms_len -= 65;
				if (ms_len < 0) {
					// Prevent bad jpeg bring about system hang..
					return JPG_HEADER_ER_MARKER;
				}
			}
			break;

		case 0xC4:      // DHT marker
			bDHTflag = TRUE;
			headerlen += ms_len;
			while (ms_len) {
				index = GetCurByte(&buf_p);
				if (index == 0x00) {    // 0th-DC-table
					jdcfg_p->p_huff_dc0th = buf_p;
				} else if (index == 0x01) { // 1th-DC-table
					jdcfg_p->p_huff_dc1th = buf_p;
				} else if (index == 0x10) { // 0th-AC-table
					jdcfg_p->p_huff_ac0th = buf_p;
				} else if (index == 0x11) { // 1th-AC-table
					jdcfg_p->p_huff_ac1th = buf_p;
				}

				total = 0;
				for (i = 0; i < 16; i++) {
					total += GetCurByte(&buf_p);
				}
				buf_p  += total;
				ms_len -= (total + 17); // 1+16+total
				if (ms_len < 0) {
					// Prevent bad jpeg bring about system hang..
					return JPG_HEADER_ER_MARKER;
				}
			}
			break;

		case 0xC0:      // SOF marker for Baseline DCT
			if (GetCurByte(&buf_p) != 0x08) {
				return (JPG_HEADER_ER_SOF_P);
			}

			bSOFflag = TRUE;
			jdcfg_p->p_tag_sof = buf_p;
			jdcfg_p->imageheight = (GetCurByte(&buf_p) << 8);
			jdcfg_p->imageheight |= GetCurByte(&buf_p);
			jdcfg_p->ori_imageheight = jdcfg_p->imageheight;

			jdcfg_p->imagewidth  = (GetCurByte(&buf_p) << 8);
			jdcfg_p->imagewidth |= GetCurByte(&buf_p);
			jdcfg_p->ori_imagewidth = jdcfg_p->imagewidth;

			jdcfg_p->numcomp = GetCurByte(&buf_p);
			for (i = 0; i < jdcfg_p->numcomp; i++) {
				GetCurByte(&buf_p);
				temp = GetCurByte(&buf_p);
				jdcfg_p->scanfreq[i][1] = temp & 0x0F; // low  4-bit: vertical freq
				jdcfg_p->scanfreq[i][0] = temp >> 4;   // high 4-bit: horizontal freq
				jdcfg_p->qtype[i] = GetCurByte(&buf_p);// Q-tab index
			}
			headerlen += ms_len;

			{
				// Fill the fileformat member variables and check the format if supported..
				BOOL bCheckResult;

				bCheckResult = jpg_supported_format(jdcfg_p->numcomp, jdcfg_p->scanfreq, &jdcfg_p->fileformat);
				if (bCheckResult == FALSE) {
					DBG_ERR("Format doesn't support !\r\n");
					return JPG_HEADER_ER_SOF_SFY;
				}
			}
			break;

		case 0xDA:      // SOS marker, ending
			headerlen += ms_len;
			jdcfg_p->headerlen = headerlen;
//#NT#2008/12/03#Scottie -begin
//#NT#Use pucStarAddr instead of jdcfg_p->inbuf for new Screennail decoding mechanism
			if (jdcfg_p->speedup_sn) {
				buf_p = pucStarAddr + jdcfg_p->headerlen;
			} else {
//#NT#2008/12/03#Scottie -end
				buf_p = jdcfg_p->inbuf + jdcfg_p->headerlen;
			}

			if (dec_type == DEC_THUMBNAIL) {
				EXIFData.ThumbJPGHeaderSize = headerlen - EXIFData.ThumbExifHeaderSize;
			}

			if (!bSOFflag) {
				return JPG_HEADER_ER_SOF_NBL;
			}

			//check Qtable
			if ((!pQTabIndex0) && (!pQTabIndex1)) {
				return JPG_HEADER_ER_DQT_TYPE;
			} else {
				// according Q talbe type to assign Q table to jdcfg_p->p_q_tbl_y
				switch (jdcfg_p->qtype[0]) {
				case 0:
					jdcfg_p->p_q_tbl_y = pQTabIndex0;
					break;
				case 1:
					jdcfg_p->p_q_tbl_y = pQTabIndex1;
					break;
				default:
					jdcfg_p->p_q_tbl_y = NULL;
					break;
				}

				if (jdcfg_p->p_q_tbl_y == NULL) {
					return JPG_HEADER_ER_DQT_TYPE;
				}

				// according Q talbe type to assign Q table to jdcfg_p->p_q_tbl_uv
				switch (jdcfg_p->qtype[1]) {
				case 0:
					jdcfg_p->p_q_tbl_uv = pQTabIndex0;
					break;
				case 1:
					jdcfg_p->p_q_tbl_uv = pQTabIndex1;
					break;
				default:
					jdcfg_p->p_q_tbl_uv = NULL;
					break;
				}

				if (jdcfg_p->p_q_tbl_uv == NULL && jdcfg_p->numcomp > 1) {
					return JPG_HEADER_ER_DQT_TYPE;
				}

//#NT#2010/06/03#Scottie -begin
//#NT#Support Quantization table 2
				// check if there is Q talbe type 2
				if (jdcfg_p->qtype[2] == 2) {
					jdcfg_p->p_q_tbl_uv2 = pQTabIndex2;
				} else {
					jdcfg_p->p_q_tbl_uv2 = NULL;
				}

				if (jdcfg_p->p_q_tbl_uv2 != NULL) {
					DBG_MSG("There is Chrominance QTable 2!\r\n");
				}
//#NT#2010/06/03#Scottie -end
			}

			//if((dec_type != DEC_AVI)&&(!bDHTflag))
			if (!bDHTflag) {
				return JPG_HEADER_ER_DHT_TYPE;
			}

			return (JPG_HEADER_ER_OK);                     // normal JPG returns here!
			break;

		case 0xDD:      // DRI marker
			jdcfg_p->rstinterval = (GetCurByte(&buf_p) << 8);
			jdcfg_p->rstinterval |= GetCurByte(&buf_p);
			headerlen += ms_len;
			break;

		//case 0xE1:         // APP1 marker
		//  // Do Nothing
		//break;

		default:
			// skip bitstream as specified by ms_len
			headerlen += ms_len;
			break;
		}   // switch (cbyte)

//#NT#2008/12/03#Scottie -begin
//#NT#Use pucStarAddr instead of jdcfg_p->inbuf for new Screennail decoding mechanism
		if (jdcfg_p->speedup_sn) {
			buf_p = pucStarAddr + headerlen;
		} else {
//#NT#2008/12/03#Scottie -end
			buf_p = jdcfg_p->inbuf + headerlen;
		}
	}   // End of while(GetCurByte(&buf_p) == 0xFF)

	return (JPG_HEADER_ER_SOS_NF);         // missing SOS marker
}

#if defined __KERNEL__
EXPORT_SYMBOL(jpg_supported_format);
EXPORT_SYMBOL(jpg_parse_header);
#endif


//@}
