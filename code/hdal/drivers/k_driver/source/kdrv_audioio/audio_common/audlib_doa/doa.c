/**
    Audio Direction-Of-Arrival (DOA) library and Voice-Active-Detection (VAD) library

	Audio DOA and VAD library. This is pure software audio library.

    @file       doa.c
    @ingroup    mIAudDOA
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef __KERNEL__
#include <string.h>
#include <stdlib.h>
#define abs(x)      (((x) < 0) ? -(x) : (x))
#define __MODULE__ rtos_doa
#define __DBGLVL__ 8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__ "*"
#include "kwrap/debug.h"
unsigned int rtos_doa_debug_level = NVT_DBG_WRN;
#else
#define abort(...)
#define dma_getCacheAddr(x) (x)
#define dma_flushWriteCache(parm,parm2)
#define dma_flushReadCache(parm,parm2)
#endif

#include "doa_int.h"
#include "vad_int.h"

#if !DOA_NORMALIZATION_FIXED_POINT
#include <math.h>
#endif


/**
    @addtogroup mIAudEC
*/
//@{

#ifndef __KERNEL__
//NVTVER_VERSION_ENTRY(ADOA, 1, 00, 000, 00)
#endif

static BOOL			doa_opened;

static UINT32		doa_total_buffer_address;
static UINT32		doa_total_buffer_size;
static UINT32		doa_input_buffer_address;

static INT16		*vad_buf1;
static int			*doa_sig_r,*doa_sig_i,*doa_ref_r,*doa_ref_i,*doa_fft_r,*doa_fft_i;

static UINT8		mic_location[4]={0,1,2,3};
static DOA_PARAM	doa_parameter = {
	AUDDOA_MODE_VAD_DOA,	// mode
	DISABLE,				// init
	0,						// tdoa
	0,						// maxshift
	0,						// doa_op_sz
	2048,					// doa_fft_op_sz
	16000,			// doa_sampletae_real
	16000,			// doa_sampletae_set
	0,						// doa_active_len
	3,						// doa_front_ext_len
	0,						// doa_back_ext_len
	1,						// doa_sample_avg
	0,						// vad_mic_select
	0,						// debug_print
	1,						// resample_en
	0,						// resample_hdl1
	0,						// resample_hdl2
	10,						// vad_sensitiveity
	2,						// channels
	ENABLE					// defaultcfg
};


static VadInstT vad_instance;


INT64 doa_sqrt64(const UINT64 number);
INT64 doa_sqrt64(const UINT64 number)
{
    INT32 interation_cnt = 0;
    UINT64 n = 1, n_tmp=0;//

    if (number <= 1) {
        return 1;
    }

    n_tmp = (1 + number) / 2;

    while ((((n_tmp >= n) ? (n_tmp-n) : (n-n_tmp)) > 1) && (++interation_cnt < 64)) {
        n = n_tmp;
		#ifndef __KERNEL__
        n_tmp = (n + (number / n)) / 2;
		#else
		{
		    UINT64 n2;

			n2 = number;
			do_div(n2, n);
			n_tmp = (n + n2) / 2;
		}
		#endif
    }

    while ((n_tmp * n_tmp) > number) {
        if (--n_tmp <= 1) {
            break;
        }
    }

    return (interation_cnt < 64) ? (INT64)n_tmp : 1;
}

#if 0
#define FLT_MIN 1.175494351e-38F
//#define FLT_MIN 10
#define ABS(x)      (((x) < 0) ? -(x) : (x))

double mySqrt_binarysearch(long long x);
double mySqrt_binarysearch(long long x)
{
	if(x <= 0) return 0;

	double begin = 1;
	double end = x/2+1;
	double mid, lastmid;

	mid = begin + (end-begin)/2;
	do{
		if(mid < x/mid) begin = mid;
		else end = mid;

		lastmid = mid;
		mid = begin + (end-begin)/2;
	}
	while(ABS(lastmid-mid) > FLT_MIN);

	return mid;
}


double mySqrt_newton(long long x);
double mySqrt_newton(long long x)
{
	if(x <= 0) return 0;

	double res, lastres;

	res = x; //初始值，可以?任意非0的值

	do{
		lastres = res;
		res = (res + x/res)/2;
	}
	while(ABS(lastres-res) > FLT_MIN);

	return res;
}
#endif

#if 1
static INT32 doa_get_theta(UINT8 idx0, UINT8 idx1)
{
#define DOA_THETA_PERF 0

#if DOA_NORMALIZATION_FIXED_POINT
	INT64   tau;
	INT32   theta1;
#else
	FLOAT	theta1,tau;
	INT32	shift;
#endif
	UINT32   i;
	UINT32	dimen,nfft;
	INT16 	*input_audio,temp;
	UINT32  input_shift;
	UINT64  arg_max=0;
	UINT16	arg_maxidx=0;
	INT32   doa_tau_r[DOA_MAX_TAU_NO];
	INT32   doa_tau_i[DOA_MAX_TAU_NO];
	#if DOA_THETA_PERF
	UINT32  curr;
	curr = Perf_GetCurrent();
	#endif

	nfft = doa_parameter.doa_fft_op_sz;


	if (doa_parameter.channels == 4)
		input_shift = 2;
	else
		input_shift = 1;


    for(i=0;i<nfft;i++) {
		doa_sig_i[i]= 0;
	}
    for(i=0;i<nfft;i++) {
		doa_ref_i[i]= 0;
	}
    for (i=(nfft/2);i<nfft;i++) {
        doa_sig_r[i]= 0;
    }
    for (i=(nfft/2);i<nfft;i++) {
        doa_ref_r[i]= 0;
    }

	input_audio = (INT16 *)doa_input_buffer_address;

    for (i=0;i<(nfft/2);i++)
    {
        temp = input_audio[((i)<<input_shift)+idx0];
        doa_sig_r[i]= temp;
    }

    for (i=0;i<(nfft/2);i++) {
        temp = input_audio[(i<<input_shift)+idx1];
        doa_ref_r[i]= temp;
    }

	#if DOA_THETA_PERF
	DBG_DUMP("time1=%d\r\n",(int)(Perf_GetCurrent()-curr));
	curr = Perf_GetCurrent();
	#endif

	doa_fft(nfft, doa_sig_r, doa_sig_i, DOA_FFT, 0);
    doa_fft(nfft, doa_ref_r, doa_ref_i, DOA_FFT, 0);

	#if DOA_THETA_PERF
	DBG_DUMP("time2=%d\r\n",(int)(Perf_GetCurrent()-curr));
	curr = Perf_GetCurrent();
	#endif

	#if DOA_NORMALIZATION_FIXED_POINT
	// R = SIG * np.conj(REFSIG)
	for(i=0;i<nfft;i++) {
		INT64 tempR,tempI,absofixed;

		tempR= (INT64)((((INT64)doa_sig_r[i]*doa_ref_r[i]) - ((INT64)doa_sig_i[i]*(-doa_ref_i[i]))))>>15;
    	tempI= (INT64)((((INT64)doa_sig_r[i]*(-doa_ref_i[i])) + ((INT64)doa_sig_i[i]*doa_ref_r[i])))>>15;
		absofixed = doa_sqrt64((UINT64)(((tempR*tempR)+(tempI*tempI))));

		#ifndef __KERNEL__
		doa_sig_r[i] = (int)((tempR<<15)/absofixed);
		doa_sig_i[i] = (int)((tempI<<15)/absofixed);
		#else
		tempR = (tempR<<15);
		if (tempR < 0) {
			tempR = -tempR;
			do_div(tempR,absofixed);
			tempR = -tempR;
		} else {
			do_div(tempR,absofixed);
		}
		doa_sig_r[i] = tempR;

		tempI = (tempI<<15);

		if (tempI < 0) {
			tempI = -tempI;
			do_div(tempI,absofixed);
			tempI = -tempI;
		} else {
			do_div(tempI,absofixed);
		}
		doa_sig_i[i] = tempI;
		#endif
	}
	#else
	// R = SIG * np.conj(REFSIG)
	for(i=0;i<nfft;i++) {

		float abso,tempR,tempI;
		tempR= (float)((((INT64)doa_sig_r[i]*doa_ref_r[i]) - ((INT64)doa_sig_i[i]*(-doa_ref_i[i])))>>15);
    	tempI= (float)((((INT64)doa_sig_r[i]*(-doa_ref_i[i])) + ((INT64)doa_sig_i[i]*doa_ref_r[i]))>>15);
		// R / np.abs(R)
		abso = sqrtf((tempR*tempR)+(tempI*tempI));
		doa_sig_r[i] = (int)((float)(tempR/abso)*32768.0);//524288.0
		doa_sig_i[i] = (int)((float)(tempI/abso)*32768.0);//524288.0
	}
	#endif

	#if DOA_THETA_PERF
	DBG_DUMP("time3=%d\r\n",(int)(Perf_GetCurrent()-curr));
	curr = Perf_GetCurrent();
	#endif

	doa_fft(nfft, doa_sig_r, doa_sig_i, DOA_IFFT,doa_parameter.maxshift);

	#if DOA_THETA_PERF
	DBG_DUMP("time4=%d\r\n",(int)(Perf_GetCurrent()-curr));
	curr = Perf_GetCurrent();
	#endif

	dimen = 0;
	for(i=0; i < doa_parameter.maxshift;i++) {
		doa_tau_r[dimen]   = doa_sig_r[nfft-doa_parameter.maxshift+i];
		doa_tau_i[dimen++] = doa_sig_i[nfft-doa_parameter.maxshift+i];
	}
	for(i=0; i < (doa_parameter.maxshift+1);i++) {
		doa_tau_r[dimen]   = doa_sig_r[i];
		doa_tau_i[dimen++] = doa_sig_i[i];
	}

	arg_max=0;
	arg_maxidx=0;
    for(i=0;i<dimen;i++)
    {
		if(((UINT64)doa_square((doa_tau_r[i]))+(UINT64)doa_square((doa_tau_i[i]))) > arg_max) {
			arg_max = ((UINT64)doa_square((doa_tau_r[i]))+(UINT64)doa_square((doa_tau_i[i])));
			arg_maxidx = i;
		}
	}

	#if DOA_THETA_PERF
	DBG_DUMP("time5=%d\r\n",(int)(Perf_GetCurrent()-curr));
	curr = Perf_GetCurrent();
	#endif

#if DOA_NORMALIZATION_FIXED_POINT
	#ifndef __KERNEL__
	tau    = (INT64)((INT64)arg_maxidx - (INT64)doa_parameter.maxshift)*AUDUOA_PRECISION/(INT64)doa_parameter.doa_sampletae_real;
	tau    = ((tau *AUDUOA_PRECISION)/doa_parameter.tdoa);
	#else
	tau    = (INT64)((INT64)arg_maxidx - (INT64)doa_parameter.maxshift)*AUDUOA_PRECISION;
	if (tau<0) {
		tau = -tau;
		do_div(tau, doa_parameter.doa_sampletae_real);
		tau= -tau;
	} else {
		do_div(tau, doa_parameter.doa_sampletae_real);
	}
	tau    = tau *AUDUOA_PRECISION;

	if (tau < 0) {
		tau = -tau;
		do_div(tau, doa_parameter.tdoa);
		tau = -tau;
	} else {
		do_div(tau, doa_parameter.tdoa);
	}
	#endif

	if(tau<-AUDUOA_PRECISION)
		tau = -AUDUOA_PRECISION;
	else if(tau>AUDUOA_PRECISION)
		tau=AUDUOA_PRECISION;


	#ifndef __KERNEL__
	for(i=0;i<(DOA_FFT_MAX_SZ>>2);i+=8){
		if(doa_sin_tab[i] >= abs(tau*32765/AUDUOA_PRECISION))
			break;
	}
	#else
	for(i=0;i<(DOA_FFT_MAX_SZ>>2);i+=8){
		INT64 temp0;

		temp0 = tau*32765;
		if (temp0 < 0) {
			temp0 = -temp0;
			do_div(temp0,AUDUOA_PRECISION);
			temp0 = -temp0;
		} else {
			do_div(temp0,AUDUOA_PRECISION);
		}
		if(doa_sin_tab[i] >= abs(temp0))
			break;
	}
	#endif

	theta1 = i*90/(DOA_FFT_MAX_SZ>>2);

	if(tau < 0)
		theta1 = -theta1;

	return theta1;

#else

	shift  = arg_maxidx - doa_parameter.maxshift;
	tau    = (float)shift / (float)doa_parameter.doa_sampletae_real;
	tau    = tau *AUDUOA_PRECISION/doa_parameter.tdoa;

	if(tau<-1)
		tau = -1;
	else if(tau>1)
		tau=1;

	theta1 = asin(tau)*180/M_PI;

	return (INT32)theta1;

#endif
}


#endif
#if 1
/**
    Open the Direction-Of-Arrival (DOA) library.

    This API must be called to initialize the Audio Direction-Of-Arrival (DOA) basic settings.

    @return
     - @b E_OK:   Open Done and success.
*/
ER audlib_doa_open(void)
{
	if (doa_opened) {
		return E_OK;
	}

	doa_opened = TRUE;
	doa_parameter.init = FALSE;

	return E_OK;
}

/**
    Check if the Audio Direction-Of-Arrival (DOA) library is opened or not.

    Check if the Audio Direction-Of-Arrival (DOA) library is opened or not.

    @return
     - @b TRUE:  Already Opened.
     - @b FALSE: Have not opened.
*/
BOOL     audlib_doa_is_opened(void)
{
	return doa_opened;
}

/**
    Close the Audio Direction-Of-Arrival (DOA) library.

    Close the Audio Audio Direction-Of-Arrival (DOA) library.
    After closing the Audio Audio Direction-Of-Arrival (DOA) library,
    any accessing for the Audio Audio Direction-Of-Arrival (DOA) APIs are forbidden except audlib_doa_open().

    @return
     - @b E_OK:  Close Done and success.
*/
ER audlib_doa_close(void)
{
	if (!doa_opened) {
		return E_OK;
	}

	#if ASRC_READY
	if(doa_parameter.resample_hdl1) {
		audlib_src_destroy(doa_parameter.resample_hdl1);
		doa_parameter.resample_hdl1 = 0;
	}
	if(doa_parameter.resample_hdl2) {
		audlib_src_destroy(doa_parameter.resample_hdl2);
		doa_parameter.resample_hdl2 = 0;
	}
	#endif

	doa_parameter.init = FALSE;
	doa_opened = FALSE;
	return E_OK;
}

/**
    Set specified Audio Direction-Of-Arrival (DOA) configurations.

    This API is used to set the specified Audio Direction-Of-Arrival (DOA) configurations.

    @param[in] doa_config_id      Select which of the DOA options is selected for configuring.
    @param[in] doa_config         The configurations of the specified DOA option.

    @return void
*/
void audlib_doa_set_config(AUDDOA_CONFIG_ID doa_config_id, INT32 doa_config)
{
	switch (doa_config_id) {
	case AUDDOA_CONFIG_ID_MODE: {
		doa_parameter.mode = doa_config;
	} break;

	case AUDDOA_CONFIG_ID_SAMPLERATE: {
		doa_parameter.doa_sampletae_set		= doa_config;
		doa_parameter.doa_sampletae_real	= doa_config;
	} break;

	case AUDDOA_CONFIG_ID_CHANNEL_NO: {
		if ((doa_config==1) ||(doa_config==2) || (doa_config==4))
			doa_parameter.channels = doa_config;
	} break;

	case AUDDOA_CONFIG_ID_BUFFER_ADDR: {
		doa_total_buffer_address = (UINT32)doa_config;
	} break;
	case AUDDOA_CONFIG_ID_BUFFER_SIZE: {
		doa_total_buffer_size = doa_config;
	} break;

	case AUDDOA_CONFIG_ID_MIC_DISTANCE: {
		doa_parameter.tdoa = doa_config;
	} break;
	case AUDDOA_CONFIG_ID_OPERATION_SIZE: {
		doa_parameter.doa_op_sz = doa_config;
	} break;
	case AUDDOA_CONFIG_ID_FRAME_SIZE: {
		if((doa_config == 512)||(doa_config == 1024)||(doa_config == 2048)||(doa_config == 4096))
			doa_parameter.doa_fft_op_sz = doa_config<<1;
	} break;
	case AUDDOA_CONFIG_ID_AVERGARE: {
		if(doa_config)
			doa_parameter.doa_sample_avg = doa_config;
	} break;
	case AUDDOA_CONFIG_ID_VAD_FRONT_EXT: {
		doa_parameter.doa_front_ext_len = doa_config;
	} break;
	case AUDDOA_CONFIG_ID_VAD_BACK_EXT: {
		doa_parameter.doa_back_ext_len  = doa_config;
	} break;
	case AUDDOA_CONFIG_ID_VAD_SENSITIVITY: {
		doa_parameter.vad_sensitiveity = doa_config;
	}break;
	case AUDDOA_CONFIG_ID_VAD_MIC_IDX: {
		doa_parameter.vad_mic_select = doa_config&0x7;
	} break;



	case AUDDOA_CONFIG_ID_INDEX_MIC0: {
		mic_location[0] = doa_config;
	} break;
	case AUDDOA_CONFIG_ID_INDEX_MIC1: {
		mic_location[1] = doa_config;
	} break;
	case AUDDOA_CONFIG_ID_INDEX_MIC2: {
		mic_location[2] = doa_config;
	} break;
	case AUDDOA_CONFIG_ID_INDEX_MIC3: {
		mic_location[3] = doa_config;
	} break;

	case AUDDOA_CONFIG_ID_DBG_MSG: {
		doa_parameter.debug_print= doa_config;
	} break;

	default:
		break;
	}

}

/**
    Get specified Audio Direction-Of-Arrival (DOA) configurations.

    This API is used to get the specified Audio Direction-Of-Arrival (DOA) configurations.

    @param[in] doa_config_id      Select which of the DOA options is selected for configuring.

    @return The configuration value of the specified DOA doa_config_id.
*/
INT32 audlib_doa_get_config(AUDDOA_CONFIG_ID doa_config_id)
{
	INT32 ret = 0;

	switch (doa_config_id) {
	case AUDDOA_CONFIG_ID_SAMPLERATE: {
		ret = doa_parameter.doa_sampletae_set;
	} break;

	case AUDDOA_CONFIG_ID_CHANNEL_NO: {
		ret = doa_parameter.channels;
	} break;

	case AUDDOA_CONFIG_ID_BUFFER_ADDR: {
		ret = doa_total_buffer_address;
	} break;
	case AUDDOA_CONFIG_ID_BUFFER_SIZE: {
		ret = (doa_parameter.doa_fft_op_sz*24)+64;
	} break;

	case AUDDOA_CONFIG_ID_MIC_DISTANCE: {
		ret = doa_parameter.tdoa;
	} break;
	case AUDDOA_CONFIG_ID_OPERATION_SIZE: {
		ret = doa_parameter.doa_op_sz;
	} break;
	case AUDDOA_CONFIG_ID_FRAME_SIZE: {
		ret = doa_parameter.doa_fft_op_sz>>1;
	} break;
	case AUDDOA_CONFIG_ID_VAD_FRONT_EXT: {
		ret = doa_parameter.doa_front_ext_len;
	} break;
	case AUDDOA_CONFIG_ID_VAD_BACK_EXT: {
		ret = doa_parameter.doa_back_ext_len;
	} break;

	case AUDDOA_CONFIG_ID_INDEX_MIC0: {
		ret = mic_location[0];
	} break;
	case AUDDOA_CONFIG_ID_INDEX_MIC1: {
		ret = mic_location[1];
	} break;
	case AUDDOA_CONFIG_ID_INDEX_MIC2: {
		ret = mic_location[2];
	} break;
	case AUDDOA_CONFIG_ID_INDEX_MIC3: {
		ret = mic_location[3];
	} break;

	default:
		break;
	}

	return ret;
}

/**
    Initialize the Audio Direction-Of-Arrival (DOA) before recording start

    This API would reset the DOA internal temp buffer and variables.
    The user must call this before starting of any new BitStream.

    @return
     - @b TRUE:  Done and success.
     - @b FALSE: Operation fail.
*/
BOOL audlib_doa_init(void)
{
	UINT32 tmp,tmp_op_sz;
  	VadInstT* self = (VadInstT*) &vad_instance;

	if (!doa_opened) {
		DBG_ERR("No Opened.\r\n");
		return FALSE;
	}

	if(((doa_parameter.mode == AUDDOA_MODE_VAD_DOA)&&(doa_total_buffer_size < (UINT32)((doa_parameter.doa_fft_op_sz*24)+6144))) ||
		((doa_parameter.mode == AUDDOA_MODE_VAD_ONLY)&&(doa_total_buffer_size < 7168))){
		DBG_ERR("BUFSZ ERR!%d %d\r\n",(int)doa_total_buffer_size,(int)doa_parameter.mode);
		return FALSE;
	}

	if((doa_parameter.mode == AUDDOA_MODE_VAD_DOA)&&(doa_parameter.doa_sampletae_set < 16000)) {
		DBG_ERR("DOA minimum supported samplerate is 16KHz\r\n");
		abort();
		//doa_parameter.resample_en = DISABLE;
	}

	// Make buffer 64B align -> cache line aligned.
	tmp = (doa_total_buffer_address+0x3F)&0xFFFFFFC0;
	tmp = dma_getCacheAddr(tmp);
	vad_buf1  = (INT16 *)tmp;
	tmp_op_sz = doa_parameter.doa_op_sz;

	if((doa_parameter.mode == AUDDOA_MODE_VAD_ONLY) && (doa_parameter.doa_sampletae_set == 8000)) {
		doa_parameter.resample_en = DISABLE;
	}

	if (doa_parameter.resample_en) {
		doa_parameter.doa_sampletae_real = DOA_RESAMPLE_TARGET;
		tmp_op_sz = tmp_op_sz * doa_parameter.doa_sampletae_real/doa_parameter.doa_sampletae_set;
	}

	doa_parameter.doa_active_len = (((doa_parameter.doa_fft_op_sz>>1)*doa_parameter.doa_sample_avg)/(doa_parameter.doa_sampletae_real/100));

	// default config:
	if(doa_parameter.defaultcfg) {
		doa_parameter.doa_front_ext_len = ((tmp_op_sz/(doa_parameter.doa_sampletae_real/100)) - doa_parameter.doa_active_len)/2;
		doa_parameter.doa_back_ext_len = doa_parameter.doa_front_ext_len;
	}

	if((UINT32)(doa_parameter.doa_active_len+doa_parameter.doa_back_ext_len+doa_parameter.doa_front_ext_len) > (UINT32)(tmp_op_sz/(doa_parameter.doa_sampletae_real/100))) {
		DBG_ERR("VAD detect len exceeed OPERATION_SIZE\r\n");
		return FALSE;
	}

	doa_parameter.maxshift = (UINT32)(((doa_parameter.tdoa*doa_parameter.doa_sampletae_real)+DOA_DIST_ROUND)/AUDUOA_PRECISION);
	if(((doa_parameter.maxshift<<1)+1) > DOA_MAX_TAU_NO) {
		DBG_ERR("DOA_MAX_TAU_NO too small! %d %d %d\r\n",(int)doa_parameter.maxshift,(int)doa_parameter.tdoa,(int)DOA_MAX_TAU_NO);
		abort();
	}

	// Init VAD core
	memset(self, 0x00, sizeof(VadInstT));
	WebRtcVad_InitCore(self);

	if(doa_parameter.mode == AUDDOA_MODE_VAD_DOA) {
		doa_sig_r = (int *)tmp;
		tmp += (doa_parameter.doa_fft_op_sz<<2);
		doa_sig_i = (int *)tmp;
		tmp += (doa_parameter.doa_fft_op_sz<<2);
		doa_ref_r = (int *)tmp;
		tmp += (doa_parameter.doa_fft_op_sz<<2);
		doa_ref_i = (int *)tmp;
		tmp += (doa_parameter.doa_fft_op_sz<<2);
		doa_fft_r = (int *)tmp;
		tmp += (doa_parameter.doa_fft_op_sz<<2);
		doa_fft_i = (int *)tmp;
		doa_assign_fft_buffer(doa_fft_r, doa_fft_i);

		tmp += (doa_parameter.doa_fft_op_sz<<2);
	} else {
		tmp += 1024;
	}

	#if ASRC_READY
	if ((doa_parameter.resample_en) && (doa_parameter.doa_sampletae_set != DOA_RESAMPLE_TARGET)) {
		if(doa_parameter.resample_hdl1) {
			audlib_src_destroy(doa_parameter.resample_hdl1);
			doa_parameter.resample_hdl1 = 0;
		}
		if(doa_parameter.resample_hdl2) {
			audlib_src_destroy(doa_parameter.resample_hdl2);
			doa_parameter.resample_hdl2 = 0;
		}
		if(audlib_src_pre_init(2, doa_parameter.doa_sampletae_set/100, DOA_RESAMPLE_TARGET/100, FALSE) > 3072) {
			DBG_ERR("ASRC Buf Err\r\n");
			abort();
		}

		audlib_src_init(&doa_parameter.resample_hdl1 , 2 , doa_parameter.doa_sampletae_set/100, DOA_RESAMPLE_TARGET/100, FALSE, (short *)tmp);

		if(doa_parameter.channels == 4) {
			tmp+=3072;
			audlib_src_init(&doa_parameter.resample_hdl2 , 2 , doa_parameter.doa_sampletae_set/100, DOA_RESAMPLE_TARGET/100, FALSE, (short *)tmp);
		}

	}
	#endif

	if(doa_parameter.debug_print) {
		DBG_DUMP("mode=%d SRS=%d SRR=%d ch=%d max_shift=%d tdoa=%d op-sz=%d nfft=%d\r\n",(int)doa_parameter.mode,(int)doa_parameter.doa_sampletae_set,(int)doa_parameter.doa_sampletae_real,(int)doa_parameter.channels,(int)doa_parameter.maxshift,(int)doa_parameter.tdoa,(int)doa_parameter.doa_op_sz,(int)doa_parameter.doa_fft_op_sz);
		DBG_DUMP("param act=%d frt=%d back=%d avg=%d vmic=%d sensitive=%d\r\n",(int)doa_parameter.doa_active_len,(int)doa_parameter.doa_front_ext_len,(int)doa_parameter.doa_back_ext_len,(int)doa_parameter.doa_sample_avg,(int)doa_parameter.vad_mic_select,(int)doa_parameter.vad_sensitiveity);
		DBG_DUMP("resample=%d hdl1=%d hdl2=%d\r\n",(int)doa_parameter.resample_en, (int)doa_parameter.resample_hdl1,(int)doa_parameter.resample_hdl2);
		DBG_DUMP("Buf addr=0x%08X size=0x%08X\r\n",(int)doa_total_buffer_address,(int)doa_total_buffer_size);
	}
	doa_parameter.init= TRUE;
	return TRUE;
}

/**
    Apply Audio Direction-Of-Arrival (DOA)'s Voice Active Detect(VAD) to audio stream

    Apply Audio Direction-Of-Arrival (DOA)'s Voice Active Detect(VAD) to audio stream.
    The return value would be the VAD detection probability. If the DOA would be performed after VAD,
    we suggest that only the probability value 100's audio frame is adopted for DOA prediction.
    Smaller probability value would cause DOA prediction less accurately.

    @param[in] doa_input_addr    Input audio buffer address in AUDDOA_CONFIG_ID_OPERATION_SIZE size..

    @return 0~100 percentage of the VAD detected probability.
*/
UINT32 audlib_doa_run_vad(UINT32 doa_input_addr)
{
	#define DOA_8KHZ_SUPPORT DISABLE

	INT16 *pBuffer;
	UINT32 len;
	UINT32 threshold=0,CurrMax=0;
	UINT32 i,proc_size,slice,slice_resample,doa_valid_len;
	UINT32 ret=0;
	INT16 *pVadBuffer,*pResampleBuffer;
	INT32  shft=0;
	UINT32 *aud2chin, *aud2chout,*audsrc;
	#if DOA_8KHZ_SUPPORT
	UINT32 *upsample_buf;
	#endif

	if (!doa_opened) {
		DBG_ERR("No Opened.\r\n");
		return ret;
	}

	if(!doa_parameter.init) {
		DBG_ERR("No init\r\n");
		return ret;
	}

	pBuffer = (INT16 *) dma_getCacheAddr(doa_input_addr);
	len = (doa_parameter.doa_op_sz<<1)*doa_parameter.channels;

	dma_flushReadCache((UINT32)pBuffer,  len);
	dma_flushWriteCache((UINT32)pBuffer, len);

	/* 10ms per VAD slice */
	slice = doa_parameter.doa_sampletae_real/100;
	slice_resample = doa_parameter.doa_sampletae_set/100;
	threshold		= 0;
	pVadBuffer		= pBuffer;
	pResampleBuffer = pBuffer;

	proc_size  = doa_parameter.doa_op_sz*slice/slice_resample;

	doa_valid_len	= doa_parameter.doa_active_len + doa_parameter.doa_front_ext_len
					+ doa_parameter.doa_back_ext_len;

	if(doa_valid_len == 0)
		return ret;

	shft = ((doa_parameter.vad_sensitiveity - 10) > 0) ? (doa_parameter.vad_sensitiveity - 10) : -(doa_parameter.vad_sensitiveity - 10);

	aud2chin		= (UINT32 *)(vad_buf1);
	aud2chout		= (UINT32 *)(vad_buf1+(doa_parameter.doa_sampletae_set/50));

	#if DOA_8KHZ_SUPPORT
	upsample_buf	= (UINT32 *)(vad_buf1+(doa_parameter.doa_sampletae_set/50)+(doa_parameter.doa_sampletae_real/50));

	if ((doa_parameter.resample_en) && (doa_parameter.doa_sampletae_set < doa_parameter.doa_sampletae_real)) {
		// Upsample: For example 8 ->16KHz, we need to prevent buf over-written
		memcpy((void *) upsample_buf, (void *) pBuffer, len);
		pResampleBuffer = (INT16 *)upsample_buf;
	}
	#endif

	//DBG_DUMP("0x%08X 0x%08X 0x%08X\r\n",(UINT32)aud2ch0,(UINT32)aud2ch1,(UINT32)audsrc);

	while(proc_size > slice) {

		#if ASRC_READY
		/* reample add here! */
		if ((doa_parameter.resample_en) && (doa_parameter.doa_sampletae_set != DOA_RESAMPLE_TARGET)) {

			if(doa_parameter.channels == 4) {

				/*
					Handle 2 ch resample
				*/
				audsrc  = (UINT32 *) pResampleBuffer;
				for(i=0;i<(slice_resample<<1);i++) {
					if(i&0x1) {
						aud2chin[i>>1] = audsrc[i];
					}
				}

				/* Resample */
				audlib_src_run(doa_parameter.resample_hdl1, (void *) aud2chin, (void *) aud2chout);

				audsrc  = (UINT32 *) pVadBuffer;
				for(i=0;i<(slice<<1);i++) {
					if(i&0x1) {
						audsrc[i] = aud2chout[i>>1];
					}
				}

				/*
					Handle 2 ch resample
				*/
				audsrc  = (UINT32 *) pResampleBuffer;
				for(i=0;i<(slice_resample<<1);i++) {
					if(!(i&0x1)) {
						aud2chin[i>>1] = audsrc[i];
					}
				}

				/* Resample */
				audlib_src_run(doa_parameter.resample_hdl2, (void *) aud2chin, (void *) aud2chout);

				audsrc  = (UINT32 *) pVadBuffer;
				for(i=0;i<(slice<<1);i++) {
					if(!(i&0x1)) {
						audsrc[i] = aud2chout[i>>1];
					}
				}

			}else{

				/* Resample */
				audlib_src_run(doa_parameter.resample_hdl1, (void *) pResampleBuffer, (void *) pVadBuffer);
			}

		}
		#endif

		if(doa_parameter.vad_sensitiveity > 10) {
			for(i=0;i<slice;i++) {
				vad_buf1[i] = (pVadBuffer[(i*doa_parameter.channels)+doa_parameter.vad_mic_select])<<shft;
			}
		} else {
			for(i=0;i<slice;i++) {
				vad_buf1[i] = (pVadBuffer[(i*doa_parameter.channels)+doa_parameter.vad_mic_select])>>shft;
			}
		}

		if(WebRtcVad_Process((VadInst*)&vad_instance, doa_parameter.doa_sampletae_real, (int16_t *)vad_buf1, slice)) {
			threshold++;
		} else {
			threshold=0;
		}

		pVadBuffer      = pVadBuffer+(slice*doa_parameter.channels);
		pResampleBuffer = pResampleBuffer+(slice_resample*doa_parameter.channels);
		proc_size-=slice;

		if(threshold > CurrMax) {
			CurrMax = threshold;
			doa_input_buffer_address = (UINT32)(pVadBuffer-(((slice*doa_parameter.channels)*CurrMax)));
		}

		if(threshold == doa_valid_len) {
			doa_input_buffer_address = (UINT32)(pVadBuffer-(((slice*doa_parameter.channels)*(doa_parameter.doa_active_len+doa_parameter.doa_back_ext_len))));
			break;
		}

	}

	ret = CurrMax*100/doa_valid_len;

	if(doa_parameter.debug_print) {
		DBG_DUMP("*%d ",(int)ret);
	}

	return ret;
}

/**
	Get the audio direction of arrival result in 0-359 degrees

	Get the audio direction of arrival result in 0-359 degrees.
	The direction result would be 0~359 if 4 microphones(channel) used.
	The direction result would be 0~179 if 2 microphones(channel) used.

    @return 0~359 degrees of the audio direction
*/
INT32 audlib_doa_get_direction(void)
{
	INT32	theta1=0,theta2=0;
	INT32   best_guess,i;

	if (doa_parameter.mode == AUDDOA_MODE_VAD_ONLY) {
		return 0;
	}

	if (!doa_opened) {
		DBG_ERR("No Opened.\r\n");
		return 0;
	}

	if(!doa_parameter.init) {
		DBG_ERR("No init\r\n");
		return 0;
	}

	if(doa_parameter.channels == 4) {

		for(i=0;i<doa_parameter.doa_sample_avg;i++) {
			theta1 += doa_get_theta(mic_location[3], mic_location[1]);
			theta2 += doa_get_theta(mic_location[2], mic_location[0]);
			doa_input_buffer_address = doa_input_buffer_address+(doa_parameter.doa_fft_op_sz*doa_parameter.channels);
		}
		theta1 = theta1/doa_parameter.doa_sample_avg;
		theta2 = theta2/doa_parameter.doa_sample_avg;

		if(abs(theta1) < abs(theta2)) {

			if(theta2 > 0) {
				best_guess = (theta1+360)%360;
			} else {
				best_guess = 180-theta1;
			}
		} else {
			if(theta1 < 0) {
				best_guess = (theta2+360)%360;
			} else {
				best_guess = 180-theta2;
			}

			best_guess = (best_guess + 270)%360;
		}

		best_guess = (-best_guess + 120);
		if(best_guess < 0)
			best_guess+=360;

		best_guess = best_guess%360;

		if(doa_parameter.debug_print) {
			DBG_DUMP("#%d ",(int)best_guess);
		}

		return best_guess;

	} else if (doa_parameter.channels == 2) {
		for(i=0;i<doa_parameter.doa_sample_avg;i++) {
			theta1 += doa_get_theta(mic_location[1], mic_location[0]);
			doa_input_buffer_address = doa_input_buffer_address+(doa_parameter.doa_fft_op_sz*doa_parameter.channels);
		}
		theta1 = theta1/doa_parameter.doa_sample_avg;
		best_guess = theta1+90;
		//best_guess = theta1+160;

		if(doa_parameter.debug_print) {
			DBG_DUMP("#%d ",(int)best_guess);
		}

		return best_guess;
	} else {
		return 0;
	}

}

//@}

#endif

#ifdef __KERNEL__
EXPORT_SYMBOL(audlib_doa_open);
EXPORT_SYMBOL(audlib_doa_is_opened);
EXPORT_SYMBOL(audlib_doa_close);
EXPORT_SYMBOL(audlib_doa_set_config);
EXPORT_SYMBOL(audlib_doa_get_config);
EXPORT_SYMBOL(audlib_doa_init);
EXPORT_SYMBOL(audlib_doa_run_vad);
EXPORT_SYMBOL(audlib_doa_get_direction);
#endif


