/**
    Copyright   Novatek Microelectronics Corp. 2004.  All rights reserved.

    @file       Exif.h
    @ingroup    mIAVJPEG

    @brief      JPEG EXIF(DCF2.2) header file.

*/

#ifndef _EXIF_H
#define _EXIF_TEMP_H

#include "kwrap/type.h"
//#include "BinaryFormat.h"
//#NT#2011/03/03#Niven Cho -begin
//#NT#Retire EXIFEncParam
//#include "JpgEnc.h"
//#NT#2011/03/03#Niven Cho -end

/** \addtogroup mIAVJPEG
*/
//@{

//#define _Support_QV5_AVI_   1           //If you want to support AVI format for Apple QuicklyView 5.0, set this 1
/*
#define    HiWord(data)     ((((int)data) & 0xffff0000)>>16)              //Bug inside, (UINT16)HiWord(-20) is not 0xFFFF
#define    LoWord(data)     (((int)data) & 0x0000ffff)
#define    HiByte(data)     ((((int)data) & 0x0000ff00) >> 8)
#define    LoByte(data)     (((int)data) & 0x000000ff)
*/
//#define    EXIF_DATE_STR    0x01
//#define    DPOF_DATE_STR    0x02


//#define ByteOrderLittleEndian    0x4949
//#define ByteOrderBigEndian       0x4D4D
//#define TIFFIdentify             42
//-----------------------------------------
//      TIFF IFD Type
//-----------------------------------------
#define TypeBYTE                 1
#define TypeASCII                2
#define TypeSHORT                3
#define TypeLONG                 4
#define TypeRATIONAL             5
#define TypeUNDEFINED            7
#define TypeSLONG                9
#define TypeSRATIONAL            10

//-----------------------------------------
//      TIFF IFD Tag Numbers
//-----------------------------------------
//#NT#2009/03/18#Photon Lin -begin
//#GPS feature
#define _Support_GPSInfo_Tag    0
//#NT#2009/03/18#Photon Lin -end
//#NT#2011/06/01#Ben Wang -begin
//#NT#Don't use "#ifdef"
#define _Support_Brightness_Tag    0
//#NT#2011/06/01#Ben Wang -end


//-----------------------------------------
//      TIFF IFD Tag ID
//-----------------------------------------
//----- 0th IFD -----------------------
#define TagImageDescription       0x010E  //ASCII
#define TagMake                   0x010F  //ASCII
#define TagModel                  0x0110  //ASCII
#define TagOrientation            0x0112  //SHORT
#define TagXResolution            0x011A  //RATIONAL
#define TagYResolution            0x011B  //RATIONAL
#define TagResolutionUnit         0x0128  //SHORT
#define TagSoftVer                0x0131  //ASCII
#define TagDataTime               0x0132  //ASCII
#define TagYCbCrPositioning       0x0213  //SHORT
//#NT#2009/06/03#Scottie -begin
//#NT#Add PrintIM tag
#define TagPrintIM                0xC4A5
//#NT#2009/06/03#Scottie -end

//#NT#2008/03/28#Jades Shih - Begin
// Add complete TIFF tags
#define TagImageWidth                  0x0100
#define TagImageLength                 0x0101
#define TagBitsPerSample               0x0102
#define TagPhotometricInterpretation   0x0106
#define TagStripOffsets                0x0111
#define TagSamplesPerPixel             0x0115
#define TagRowsPerStrip                0x0116
#define TagStripByteCounts             0x0117
#define TagPlanarConfiguration         0x011C
#define TagTransferFunction            0x012D
#define TagSoftware                    0x0131
#define TagArtist                      0x013B
#define TagWhitePoint                  0x013E
#define TagPrimaryChromaticities       0x013F
#define TagYCbCrCoefficients           0x0211
#define TagYCbCrSubSampling            0x0212
#define TagReferenceBlackWhite         0x0214
#define TagCopyright                   0x8298
//#NT#2008/03/28#Jades Shih - End

#define TagGPSIFDPtr              0x8825
#define TagExifIFDPtr             0x8769

//----- Exif IFD -----------------------
// If you want to to add/delete tags into ExifSubIFD,
// You should also modify TagNumsOfExifIFD value.
#define TagExposureTime           0x829A  //RATIONAL
#define TagFNumber                0x829D  //RATIONAL
#define TagExposureProgram        0x8822  //SHORT
#define TagISOSpeedRatings        0x8827  //SHORT
#define TagExifVersion            0x9000  //UNDEFINED
#define TagDateTimeOriginal       0x9003  //ASCII
#define TagDateTimeDigitized      0x9004  //ASCII
#define TagComponentsCfg          0x9101  //UNDEFINED
#define TagShutterSpeed           0x9201  //SRATIONAL
#define TagAperture               0x9202  //RATIONAL
#define TagBrightness             0x9203  //SRATIONAL
#define TagExposureBias           0x9204  //SRATIONAL
#define TagMaxAperture            0x9205  //RATIONAL
#define TagMeteringMode           0x9207  //SHORT
#define TagLightSource            0x9208  //SHORT
#define TagStrobeFlash            0x9209  //SHORT
#define TagFocalLength            0x920A  //RATIONAL
#define TagMakerNote              0x927C  //UNDEFINED
#define TagUserComment            0x9286  //UNDEFINED
#define TagFlashPixVersion        0xA000  //UNDEFINED
#define TagColorspace             0xA001  //SHORT
#define TagPixelXDimension        0xA002  //LONG
#define TagPixelYDimension        0xA003  //LONG
#define TagInterOperateIFDPtr     0xA005
#define TagSensingMethod          0xA217  //SHORT
#define TagFileSource             0xA300  //UNDEFINED
#define TagCustomRendered         0xA401  //SHORT
#define TagExposureMode           0xA402  //SHORT
#define TagWhiteBalance           0xA403  //SHORT
#define TagSceneCaptureType       0xA406  //SHORT
#define TagDigitalZoomRatio       0xA404
#define TagGainControl            0xA407
#define TagContrast               0xA408
#define TagSaturation             0xA409
#define TagSharpness              0xA40A  //SHORT
#define TagSpectralSensitivity      0x8824
#define TagOECF                     0x8828
#define TagCompressedBitsPerPixel   0x9102
#define TagSubjectDistance          0x9206
#define TagSubjectArea              0x9214
#define TagSubSecTime               0x9290
#define TagSubSecTimeOriginal       0x9291
#define TagSubSecTimeDigitized      0x9292
#define TagRelatedSoundFile         0xA004
#define TagFlashEnergy              0xA20B
#define TagSpatialFrequencyResponse 0xA20C
#define TagFocalPlaneXResolution    0xA20E
#define TagFocalPlaneYResolution    0xA20F
#define TagFocalPlaneResolutionUnit 0xA210
#define TagSubjectLocation          0xA214
#define TagExposureIndex            0xA215
#define TagSensingMethod            0xA217
#define TagSceneType                0xA301
#define TagCFAPattern               0xA302
#define TagFocalLengthIn35mmFilm    0xA405
#define TagSceneCaptureType         0xA406
#define TagDeviceSettingDescription 0xA40B
#define TagSubjectDistanceRange     0xA40C
#define TagImageUniqueID            0xA420


//----- GPS IFD  -----
// Until 2006/12, We still do NOT have GPS module. This information is defined for future used.
// If you required more information, please refer to EXIF Spec.
#define TagGPSVersionID           0x0000  //BYTE
#define TagGPSLatitudeRef         0x0001  //ASCII
#define TagGPSLatitude            0x0002  //RATIONAL
#define TagGPSLongitudeRef        0x0003  //ASCII
#define TagGPSLongitude           0x0004  //RATIONAL
#define TagGPSAltitudeRef         0x0005  //BYTE
#define TagGPSAltitude            0x0006  //RATIONAL
#define TagGPSTimeStamp           0x0007  //RATIONAL
#define TagGPSSatellites          0x0008
#define TagGPSStatus              0x0009
#define TagGPSMeasureMode         0x000A
#define TagGPSDOP                 0x000B
#define TagGPSSpeedRef            0x000C
#define TagGPSSpeed               0x000D
#define TagGPSTrackRef            0x000E
#define TagGPSTrack               0x000F
#define TagGPSImgDirectionRef     0x0010
#define TagGPSImgDirection        0x0011
#define TagGPSMapDatum            0x0012
#define TagGPSDestLatitudeRef     0x0013
#define TagGPSDestLatitude        0x0014
#define TagGPSDestLongitudeRef    0x0015
#define TagGPSDestLongitude       0x0016
#define TagGPSDestBearingRef      0x0017
#define TagGPSDestBearing         0x0018
#define TagGPSDestDistanceRef     0x0019
#define TagGPSDestDistance        0x001A
#define TagGPSProcessingMethod    0x001B
#define TagGPSAreaInformation     0x001C
#define TagGPSDateStamp           0x001D  //ASCII
#define TagGPSDifferential        0x001E

//----- InterOperability IFD ----------
#define TagInterOpIndex            0x0001
#define TagExifR98Ver              0x0002
//----- 1th IFD -------------------------------
#define TagCompression                     0x0103
#define TagJPEGInterchangeFormat           0x0201
#define TagJPEGInterchangeFormatLength     0x0202


//-----------------------------------------
//      TIFF IFD Tag Length
//-----------------------------------------
//----- 0th IFD -----------------------
#define TagImgDescLen             16
#define TagMakeLen                24
#define TagModelLen               16
#define TagOrientLen              1
#define TagXResolLen              1
#define TagYResolLen              1
#define TagResolUnitLen           1
#define TagSoftVerLen             32
#define TagDataTimeLen            20
#define TagYCbCrPosLen            1
#define TagGPSPtrLen              1
#define TagExifPtrLen             1
//----- Exif IFD -----------------------
#define TagExpTimeLen             1
#define TagFNumberLen             1
#define TagExpProgLen             1
#define TagISOSpeedLen            1
#define TagExifVerLen             4
#define TagDatTimOriLen           20
#define TagDatTimDigLen           20
#define TagCompCfgLen             4
#define TagShutterLen             1
#define TagApertureLen            1
#define TagBrightnessLen          1
#define TagExpoBiasLen            1
#define TagMaxApertureLen         1
#define TagMeteringLen            1
#define TagLightSrcLen            1
#define TagStrobeLen              1
#define TagFocalLenLen            1

// Until 2006/12, We still do NOT have GPS module. This information is defined for future used.
#define TagGPSVersionLen          4
#define TagGPSLatitudeRefLen      2
#define TagGPSLatitudeLen         3
#define TagGPSLongitudeRefLen     TagGPSLatitudeRefLen
#define TagGPSLongitudeLen        TagGPSLatitudeLen
//#NT#2011/06/08#Meg Lin -begin
#define TagGPSAltitudeRefLen      1
#define TagGPSAltitudeLen         1
#define TagGPSTimeStampLen        3
#define TagGPSDateStampLen        11

//#Add User Comment tag
#define TagUserCommentLen         32
#define TagUserCommentSNIDLen     7     // length of Social Network identifier in User Comment (7 bytes)
//#NT#2011/06/08#Meg Lin -end

///////////////////////////////////////////////////////////////////////////
//we reserve 128 bytes for normal data (only 64 bytes in real),
//4096 for debug message.
#define MakeNoteLen_DEBUGBUFSIZE  1024
#define MakeNoteLen_Real          63
#define EXIFW_MAKENOTELEN_REAL    64//2011/07/12 Meg Lin adds
//#NT#2009/03/18#Photon Lin -begin
//#Do not use #ifdef

//#define TagMakeNoteLen            MakeNoteLen_Real+MakeNoteLen_UnInit+MakeNoteLen_DEBUGBUFSIZE

#define TagFlashVerLen            4
#define TagColorSpcLen            1
#define TagPXDimenLen             1
#define TagPYDimenLen             1
#define TagOpIFDPtrLen            1
#define TagSensingLen             1
#define TagFileSrcLen             1
#define TagCMRenderLen            1
#define TagExpModeLen             1
#define TagWhiteBNLen             1
#define TagSNECaptLen             1
#define TagDZRatioLen             1
#define TagGainCtlLen             1
#define TagContrastLen            1
#define TagSaturateLen            1
#define TagSharpLen               1

//----- MakerNote IFD ------------------
#define TagBodyFWVer            0x0104
#define TagSpecialFunc          0x0200
#define TagMacroMode            0x0202
#define TagColorMode            0x0203
#define TagDigitalZoom          0x0204
#define TagAngleOfView          0x0205
#define TagAberration           0x0206
#define TagCameraType           0x0207
#define TagCameraID             0x0209
#define TagEditType             0x020A
#define TagJPEGCSRate           0x020B
#define TagAFLockData           0x0401
#define TagAFMode               0x0402
#define TagScenePrg             0x0403
#define TagWBMode               0x0500
//#NT#2011/10/20#Lincy Lin -begin
//#NT#650GxImage
#define TagTrimMode             0x0600//?? This tag isn't described in A735/A835 spec
//#NT#2011/10/20#Lincy Lin -end
#define TagCameraMode           0x2020
//#NT#2008/10/30#Scottie -begin
//#Modify for the modificaion of MakerNote v3.0 spec
#define TagHH_Local             0x5000//0x4010
//#NT#2008/10/30#Scottie -end

#define TagNumsOfMakerNoteIFD   16

//----- CameraMode IFD -----------------
#define TagCMVersion            0x0000
#define TagPlayPicFormat        0x0100
#define TagPlayPicOffset        0x0101
#define TagPlayPicLength        0x0102
//#NT#2009/04/14#Scottie -begin
//#NT#Add for OLY 2009Autumn project
#define TagJPEGCSRate_CM        0x050D

//#define TagNumsOfCameraModeIFD  4
#define TagNumsOfCameraModeIFD  5
//#NT#2009/04/14#Scottie -end

//----- InterOperability IFD ----------
#define TagInterOpIndexLen        4
#define TagExifR98VerLen          4
//----- 1th IFD -----------------------
#define TagCompressionLen         1
#define TagJPEGInterLen           1
#define TagJPEGLengthLen          1


/**
@name JPEG compression quality setting.
    These values are predefined meaning with specific range.
*/
//@{
#define  Quality_Undefine           0       ///< use default quality value
#define  Quality_SuperHigh          1       ///< uiQuality as 0, 97~95
#define  Quality_High               2       ///< uiQuality as 94~90
#define  Quality_Economy            3       ///< uiQuality as 89~80
//@}
//#NT#2011/10/19#Lincy Lin -begin
//#NT#
//-----------------------------------------------------------------------------------------------
// Enumerations
//-----------------------------------------------------------------------------------------------


//-----------------------------------------
// Exif file header data structures
//-----------------------------------------




//----- Exposure Program ---------------------------
#define ExpPrgNoDef            0
#define ExpPrgManual           1
#define ExpPrgNormal           2
#define ExpPrgAperture         3    // Aperture Priority
#define ExpPrgShutter          4    // Shutter Priority
#define ExpPrgCreative         5
#define ExpPrgAction           6
#define ExpPrgPortrait         7
#define ExpPrgLandscape        8
//----- ISO Speed -----------------------------------
#define ISOSpeed50             50
#define ISOSpeed100            100
#define ISOSpeed200            200
#define ISOSpeed400            400
//----- Metering Mode -------------------------------
#define MeterUnknown           0
#define MeterAverage           1
#define MeterCenterWeightedAvg 2
#define MeterSpot              3
#define MeterMultiSpot         4
#define MeterPattern           5
#define MeterPartial           6
//----- Light Source -------------------------------
//#NT#2011/06/14#Hideo Lin -begin
//#NT#Modify EXIF tag value according to EXIF 2.2 spec.
#define LightUnknown                0
#define LightDaylight               1
#define LightFluorescent            2
#define LightTungsten               3
#define LightFlash                  4
#define LightFineWeather            9
#define LightCloudy                 10
#define LightShade                  11
#define LightStandardA              17
#define LightStandardB              18
#define LightStandardC              19
//----- Strobe Flash -------------------------------
#define StrobeNoFired               0
#define StrobeFired                 1
#define StrobeNoDetectFunction      (0 << 1) // bit[2:1] = 0
#define StrobeReturnNotDetected     (2 << 1) // bit[2:1] = 2
#define StrobeReturnDetected        (3 << 1) // bit[2:1] = 3
#define StrobeModeUnknown           (0 << 3) // bit[3:4] = 0
#define StrobeForceOn               (1 << 3) // bit[3:4] = 1
#define StrobeForceOff              (2 << 3) // bit[3:4] = 2
#define StrobeModeAuto              (3 << 3) // bit[3:4] = 3
#define StrobeFunctionPresent       (0 << 5) // bit[5] = 0
#define StrobeNoFunction            (1 << 5) // bit[5] = 1
#define StrobeRedEyeOff             (0 << 6) // bit[6] = 0
#define StrobeRedEyeOn              (1 << 6) // bit[6] = 1
#define StrobeFlashAuto             ((1<<4)+(1<<3))
#define StrobeFlashForceOFF         ((1<<4))
#define StrobeFlashForceON          ((1<<3))
#define StrobeFlashAutoRedEye       ((1<<6)+(1<<4)+(1<<3))
#define StrobeFlashForceONRedEye    ((1<<6)+(1<<3))
#define StrobeFlashNightStrobe      ((1<<6)+(1<<4)+(1<<3))
//#NT#2011/06/14#Hideo Lin -end
//----- Color Space --------------------------------
#define ColorSpaceSRGB         1
//----- Sensing Mode -------------------------------
#define SensingNoDef           1
#define SensingOneChip         2    // OneChipColorArea sensor
#define SensingTwoChip         3
#define SensingThreeChip       4
#define SensingColorSeqArea    5
#define SensingTriLinear       7
#define SensingColorSeqLinear  8
//----- CustomRendered ----------------------------
#define NormalRendered         0
#define CustomRendered         1
//-----Exposure Mode-------------------------------
#define AutoExposure           0
#define ManualExposure         1
#define AutoBracket            2
//-----White Balance ------------------------------
#define AutoWhite              0
#define ManualWhite            1

//-----SceneCapture Type --------------------------
#define StandardScene          0
#define LandScapeScene         1
#define PortraitScene          2
#define NightScene             3

//-----Gain Control--------------------------------
#define NONE_GAIN              0
#define LOW_GAIN_UP            1
#define HIGH_GAIN_UP           2
#define LOW_GAIN_DOWN          3
#define HIGH_GAIN_DOWN         4

//-----Contrast------------------------------------
#define NORMAL_CONTRAST        0
#define SOFT_CONTRAST          1
#define HARD_CONTRAST          2

//-----Saturation----------------------------------
#define NORMAL_SATURATION      0
#define LOW_SATURATION         1
#define HIGH_SATURATION        2

//-----Sharpness-----------------------------------
#define NORMAL_SHARPNESS       0
#define SOFT_SHARPNESS         1
#define HARD_SHARPNESS         2

//----- Compression -------------------------------
#define UnCompressed           1
#define JPEGCompression        6

//--------------------------------------------------------------
//#define Commentdatalength        17
//#NT#2011/06/08#Meg Lin -begin
//----- GPS IFD Value --------------------------------------------
#define TagGPSVersionVal        0x00000202                  //2.2.0.0
#define GPSLatitudeRefVal       0x0000004E                  //Default "N"
#define GPSLongitudeRefVal      0x00000045                  //Default "E"
#define GPSAltitudeRefVal       0x00000000                  //Default 0, 0->Above sea level, 1->Below sea level
//#NT#2011/06/08#Meg Lin -end


//@}

#endif    // _EXIF_TEMP_H