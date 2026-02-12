#ifndef _UIAPPMOVIE_SAMPLECODE_HEIC_H_
#define _UIAPPMOVIE_SAMPLECODE_HEIC_H_

#if HEIC_FUNC == ENABLE


#define MOVIE_SAMPLE_HEIC_WITH_JPG          TRUE

extern ER MovieSample_HEIC_InstallID(void);
extern ER MovieSample_HEIC_UninstallID(void);

extern HD_RESULT MovieSample_HEIC_VEnc_Start(void);
extern HD_RESULT MovieSample_HEIC_VEnc_Stop(void);

extern ER MovieSample_HEIC_Trigger(MOVIE_CFG_REC_ID id);
#endif

#endif //_UIAPPMOVIE_SAMPLECODE_HEIC_H_
