#ifndef _NVTPACKINFO_H
#define _NVTPACKINFO_H

#define HEADER_CHKSUM_VERSION 0x16040719U

#if !defined(_WIN32)
typedef struct _GUID {
	unsigned int   Data1;
	unsigned short Data2;
	unsigned short Data3;
	unsigned char  Data4[8];
} GUID;
#endif

typedef struct _HEADER_RES{
    GUID         guid;
    unsigned int TotalSize;
    unsigned int CheckSum;
    unsigned int ContentSize;
    char         SrcName[16];  ///< original packed bin file name
    char         FileName[16]; ///< target file name
}HEADER_RES;

typedef struct _HEADER_FW{
    GUID         guid;
    unsigned int TotalSize;
    unsigned int CheckSum;
    unsigned int TotalRes;
}HEADER_FW;

typedef struct _HEADER_PARTITION{
	unsigned int Offset;
	unsigned int Size;
	unsigned int PartitionID;
}HEADER_PARTITION;

typedef struct _HEADER_CHKSUM {
	unsigned int uiFourCC ;	///< 'C','K','S','M'
	unsigned int uiVersion;	///< HEADER_CHKSUM header version
	unsigned int uiChkMethod;	///< 0: CheckSum
	unsigned int uiChkValue;	///< check sum value or crc value within HEADER_CHKSUM
	unsigned int uiDataOffset;///< real data starting offset without HEADER_CHKSUM
	unsigned int uiDataSize;  ///< real data size without HEADER_CHKSUM
	unsigned int uiPaddingSize; ///< padding bytes for check sum
	unsigned int uiEmbType;   ///< relate to EMBTYPE_?????
	unsigned int uiReversed[8]; ///< align to 64 bytes
} HEADER_CHKSUM, *PHEADER_CHKSUM;

#ifndef IsEqualGUID
#define IsEqualGUID(rguid1, rguid2) (!memcmp(rguid1, rguid2, sizeof(GUID)))
#endif

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3) ((UINT32)(UINT8)(ch0) | ((UINT32)(UINT8)(ch1) << 8) | ((UINT32)(UINT8)(ch2) << 16) | ((UINT32)(UINT8)(ch3) << 24 ))
#endif
#endif