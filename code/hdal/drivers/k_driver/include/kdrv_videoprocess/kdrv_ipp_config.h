/*
    IPL DAL configuration header file.

    IPL DAL configuration header file. Define semaphore ID, flag ID, etc.

    @file       dal_ipp_config.h
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

#ifndef _KDRV_IPP_CONFIG_H
#define _KDRV_IPP_CONFIG_H

// NOTE: XXXXX
#if defined __UITRON || defined __ECOS
#include "SysKer.h"
#include "dma.h"
#include "Type.h"
#else
#if defined(__FREERTOS)
#include "kwrap/type.h"
#endif
#include <kwrap/semaphore.h>
#include <kwrap/flag.h>
#endif

#define KDRV_IPP_MAX_CHANNEL_NUM  28  // maxima available channel number
#define KDRV_IPP_NEW_INIT_FLOW (ENABLE)

// ==================================================================
// Flag PTN
// ==================================================================
#define KDRV_IPP_RHE_LOCK        FLGPTN_BIT(0)  //(ofs bit << id)
#define KDRV_IPP_RHE_FMD         FLGPTN_BIT(28)
#define KDRV_IPP_RHE_TIMEOUT     FLGPTN_BIT(29)
#define KDRV_IPP_RHE_HDL_UNLOCK  FLGPTN_BIT(30)
#define KDRV_IPP_RHE_LOCK_BITS   0x0001FFFF
#define KDRV_IPP_RHE_INIT_BITS   (KDRV_IPP_RHE_LOCK_BITS | KDRV_IPP_RHE_HDL_UNLOCK)

extern UINT32 kdrv_ife_lock_chls;
#define KDRV_IPP_IFE_LOCK        FLGPTN_BIT(0)  //(ofs bit << id)
#define KDRV_IPP_IFE_FMD         FLGPTN_BIT(28)
#define KDRV_IPP_IFE_TIMEOUT     FLGPTN_BIT(29)
#define KDRV_IPP_IFE_HDL_UNLOCK  FLGPTN_BIT(30)
#define KDRV_IPP_IFE_LOCK_BITS   (kdrv_ife_lock_chls)//0x0001FFFF
#define KDRV_IPP_IFE_INIT_BITS   (KDRV_IPP_IFE_LOCK_BITS | KDRV_IPP_IFE_HDL_UNLOCK)


extern UINT32 kdrv_dce_lock_chls;
#define KDRV_IPP_DCE_LOCK        FLGPTN_BIT(0)  //(ofs bit << id)
#define KDRV_IPP_DCE_FMD         FLGPTN_BIT(28)
#define KDRV_IPP_DCE_TIMEOUT     FLGPTN_BIT(29)
#define KDRV_IPP_DCE_HDL_UNLOCK  FLGPTN_BIT(30)
#define KDRV_IPP_DCE_LOCK_BITS   (kdrv_dce_lock_chls)//0x0001FFFF
#define KDRV_IPP_DCE_INIT_BITS   (KDRV_IPP_DCE_LOCK_BITS | KDRV_IPP_DCE_HDL_UNLOCK)

extern UINT32 kdrv_ipe_lock_chls;
#define KDRV_IPP_IPE_LOCK        FLGPTN_BIT(0)  //(ofs bit << id)
#define KDRV_IPP_IPE_FMD         FLGPTN_BIT(28)
#define KDRV_IPP_IPE_TIMEOUT     FLGPTN_BIT(29)
#define KDRV_IPP_IPE_HDL_UNLOCK  FLGPTN_BIT(30)
#define KDRV_IPP_IPE_LOCK_BITS   (kdrv_ipe_lock_chls)//0x0001FFFF
#define KDRV_IPP_IPE_INIT_BITS   (KDRV_IPP_IPE_LOCK_BITS | KDRV_IPP_IPE_HDL_UNLOCK)

extern UINT32 kdrv_ime_lock_chls;
#define KDRV_IPP_IME_LOCK        FLGPTN_BIT(0)  //(ofs bit << id)
#define KDRV_IPP_IME_FMD         FLGPTN_BIT(28)
#define KDRV_IPP_IME_TIMEOUT     FLGPTN_BIT(29)
#define KDRV_IPP_IME_HDL_UNLOCK  FLGPTN_BIT(30)
#define KDRV_IPP_IME_LOCK_BITS   (kdrv_ime_lock_chls)//0x0001FFFF
#define KDRV_IPP_IME_INIT_BITS   (KDRV_IPP_IME_LOCK_BITS | KDRV_IPP_IME_HDL_UNLOCK)

extern UINT32 kdrv_ife2_lock_chls;
#define KDRV_IPP_IFE2_LOCK        FLGPTN_BIT(0)  //(ofs bit << id)
#define KDRV_IPP_IFE2_FMD         FLGPTN_BIT(28)
#define KDRV_IPP_IFE2_TIMEOUT     FLGPTN_BIT(29)
#define KDRV_IPP_IFE2_HDL_UNLOCK  FLGPTN_BIT(30)
#define KDRV_IPP_IFE2_LOCK_BITS   (kdrv_ife2_lock_chls)//0x0001FFFF
#define KDRV_IPP_IFE2_INIT_BITS   (KDRV_IPP_IFE2_LOCK_BITS | KDRV_IPP_IFE2_HDL_UNLOCK)


extern UINT32 kdrv_vpe_lock_chls;
#define KDRV_IPP_VPE_LOCK        FLGPTN_BIT(0)  //(ofs bit << id)
#define KDRV_IPP_VPE_FMD         FLGPTN_BIT(28)
#define KDRV_IPP_VPE_TVPEOUT     FLGPTN_BIT(29)
#define KDRV_IPP_VPE_HDL_UNLOCK  FLGPTN_BIT(30)
#define KDRV_IPP_VPE_LOCK_BITS   (kdrv_vpe_lock_chls)//0x0001FFFF
#define KDRV_IPP_VPE_INIT_BITS   (KDRV_IPP_VPE_LOCK_BITS | KDRV_IPP_VPE_HDL_UNLOCK)


// ==================================================================
// Flag ID
// ==================================================================
typedef enum {
	FLG_ID_KDRV_IFE,
	KDRV_IFE_FLAG_COUNT,
	ENUM_DUMMY4WORD(KDRV_IFE_FLAG)
} KDRV_IFE_FLAG;

typedef enum {
	FLG_ID_KDRV_DCE,
	KDRV_DCE_FLAG_COUNT,
	ENUM_DUMMY4WORD(KDRV_DCE_FLAG)
} KDRV_DCE_FLAG;

typedef enum {
	FLG_ID_KDRV_IPE,
	KDRV_IPE_FLAG_COUNT,
	ENUM_DUMMY4WORD(KDRV_IPE_FLAG)
} KDRV_IPE_FLAG;

typedef enum {
	FLG_ID_KDRV_RHE,
	KDRV_RHE_FLAG_COUNT,
	ENUM_DUMMY4WORD(KDRV_RHE_FLAG)
} KDRV_RHE_FLAG;

typedef enum {
	FLG_ID_KDRV_IME,
	KDRV_IME_FLAG_COUNT,
	ENUM_DUMMY4WORD(KDRV_IME_FLAG)
} KDRV_IME_FLAG;

typedef enum {
	FLG_ID_KDRV_IFE2,
	KDRV_IFE2_FLAG_COUNT,
	ENUM_DUMMY4WORD(KDRV_IFE2_FLAG)
} KDRV_IFE2_FLAG;

typedef enum {
	FLG_ID_KDRV_VPE,
	KDRV_VPE_FLAG_COUNT,
	ENUM_DUMMY4WORD(KDRV_VPE_FLAG)
} KDRV_VPE_FLAG;


// ==================================================================
// Semaphore ID
// ==================================================================
typedef enum {
	SEMID_KDRV_IFE,
	// Insert member before this line
	// Don't change the order of following two members
	KDRV_IFE_SEMAPHORE_COUNT,
	ENUM_DUMMY4WORD(KDRV_IFE_SEM)
} KDRV_IFE_SEM;

typedef enum {
	SEMID_KDRV_DCE,
	// Insert member before this line
	// Don't change the order of following two members
	KDRV_DCE_SEMAPHORE_COUNT,
	ENUM_DUMMY4WORD(KDRV_DCE_SEM)
} KDRV_DCE_SEM;

typedef enum {
	SEMID_KDRV_IPE,
	// Insert member before this line
	// Don't change the order of following two members
	KDRV_IPE_SEMAPHORE_COUNT,
	ENUM_DUMMY4WORD(KDRV_IPE_SEM)
} KDRV_IPE_SEM;

typedef enum {
	SEMID_KDRV_RHE,
	// Insert member before this line
	// Don't change the order of following two members
	KDRV_RHE_SEMAPHORE_COUNT,
	ENUM_DUMMY4WORD(KDRV_RHE_SEM)
} KDRV_RHE_SEM;

typedef enum {
	SEMID_KDRV_IME,
	// Insert member before this line
	// Don't change the order of following two members
	KDRV_IME_SEMAPHORE_COUNT,
	ENUM_DUMMY4WORD(KDRV_IME_SEM)
} KDRV_IME_SEM;

typedef enum {
	SEMID_KDRV_IFE2,
	// Insert member before this line
	// Don't change the order of following two members
	KDRV_IFE2_SEMAPHORE_COUNT,
	ENUM_DUMMY4WORD(KDRV_IFE2_SEM)
} KDRV_IFE2_SEM;

typedef enum {
	SEMID_KDRV_VPE,
	// Insert member before this line
	// Don't change the order of following two members
	KDRV_VPE_SEMAPHORE_COUNT,
	ENUM_DUMMY4WORD(KDRV_VPE_SEM)
} KDRV_VPE_SEM;


//@}


/**
    Semphore Configuration Infomation
*/
typedef struct _KDRV_IPP_SEM_INFO_ {
	SEM_HANDLE  semphore_id;                ///< Semaphore ID

#if defined __UITRON || defined __ECOS
	UINT32      attribute;
	UINT32      counter;
#endif

	UINT32      max_counter;
} KDRV_IPP_SEM_TABLE;

#if 0
// DCE
typedef struct _KDRV_DCE_SEM_INFO_ {
	SEM_HANDLE  semphore_id;                ///< Semaphore ID

#if defined __UITRON || defined __ECOS
	UINT32      attribute;
	UINT32      counter;
#endif

	UINT32      max_counter;
} KDRV_DCE_SEM_TABLE;

// IPE
typedef struct _KDRV_IPE_SEM_INFO_ {
	SEM_HANDLE  semphore_id;                ///< Semaphore ID

#if defined __UITRON || defined __ECOS
	UINT32      attribute;
	UINT32      counter;
#endif

	UINT32      max_counter;
} KDRV_IPE_SEM_TABLE;

// RHE
typedef struct _KDRV_RHE_SEM_INFO_ {
	SEM_HANDLE  semphore_id;                ///< Semaphore ID

#if defined __UITRON || defined __ECOS
	UINT32      attribute;
	UINT32      counter;
#endif

	UINT32      max_counter;
} KDRV_RHE_SEM_TABLE;

// IME
typedef struct _KDRV_IME_SEM_INFO_ {
	SEM_HANDLE  semphore_id;                ///< Semaphore ID

#if defined __UITRON || defined __ECOS
	UINT32      attribute;
	UINT32      counter;
#endif

	UINT32      max_counter;
} KDRV_IME_SEM_TABLE;

// IFE2
typedef struct _KDRV_IFE2_SEM_INFO_ {
	SEM_HANDLE  semphore_id;                ///< Semaphore ID

#if defined __UITRON || defined __ECOS
	UINT32      attribute;
	UINT32      counter;
#endif

	UINT32      max_counter;
} KDRV_IFE2_SEM_TABLE;
#endif

#if defined __UITRON || defined __ECOS
extern UINT32 _SECTION(".kercfg_data") kdrv_ife_flag_id[KDRV_IPP_FLAG_COUNT];
extern UINT32 _SECTION(".kercfg_data") kdrv_ime_flag_id[KDRV_IME_FLAG_COUNT];
extern UINT32 _SECTION(".kercfg_data") kdrv_ife2_flag_id[KDRV_IFE2_FLAG_COUNT];

extern KDRV_IPP_SEM_TABLE _SECTION(".kercfg_data") kdrv_ipp_semtbl[KDRV_IPP_SEMAPHORE_COUNT];
void kdrv_ipp_install_id(void) _SECTION(".kercfg_text");
#else
extern ID kdrv_ife_flag_id[KDRV_IFE_FLAG_COUNT];
//extern KDRV_IPP_SEM_TABLE kdrv_ife_semtbl[KDRV_IFE_SEMAPHORE_COUNT];
void kdrv_ife_install_id(void);
void kdrv_ife_uninstall_id(void);

extern ID kdrv_dce_flag_id[KDRV_DCE_FLAG_COUNT];
//extern KDRV_IPP_SEM_TABLE kdrv_dce_semtbl[KDRV_DCE_SEMAPHORE_COUNT];
void kdrv_dce_install_id(void);
void kdrv_dce_uninstall_id(void);

extern ID kdrv_ipe_flag_id[KDRV_IPE_FLAG_COUNT];
//extern KDRV_IPP_SEM_TABLE kdrv_ipe_semtbl[KDRV_IPE_SEMAPHORE_COUNT];
void kdrv_ipe_install_id(void);
void kdrv_ipe_uninstall_id(void);

//extern UINT32 kdrv_rhe_flag_id[KDRV_RHE_FLAG_COUNT];
//extern KDRV_IPP_SEM_TABLE kdrv_ipp_semtbl[KDRV_RHE_SEMAPHORE_COUNT];
//void kdrv_rhe_install_id(void);
//void kdrv_rhe_uninstall_id(void);

extern ID kdrv_ime_flag_id[KDRV_IME_FLAG_COUNT];
//extern KDRV_IPP_SEM_TABLE kdrv_ime_semtbl[KDRV_IME_SEMAPHORE_COUNT];
extern void kdrv_ime_install_id(void);
extern void kdrv_ime_uninstall_id(void);

extern ID kdrv_ife2_flag_id[KDRV_IFE2_FLAG_COUNT];
//extern KDRV_IPP_SEM_TABLE kdrv_ife2_semtbl[KDRV_IFE2_SEMAPHORE_COUNT];
extern void kdrv_ife2_install_id(void);
extern void kdrv_ife2_uninstall_id(void);

extern ID kdrv_vpe_flag_id[KDRV_VPE_FLAG_COUNT];
//extern KDRV_IPP_SEM_TABLE kdrv_ime_semtbl[KDRV_IME_SEMAPHORE_COUNT];
extern void kdrv_vpe_install_id(void);
extern void kdrv_vpe_uninstall_id(void);

#endif

void kdrv_ife_flow_init(void);
SEM_HANDLE *kdrv_ife_get_sem_id(KDRV_IFE_SEM idx);
ID kdrv_ife_get_flag_id(KDRV_IFE_FLAG idx);

void kdrv_dce_flow_init(void);
SEM_HANDLE *kdrv_dce_get_sem_id(KDRV_DCE_SEM idx);
ID kdrv_dce_get_flag_id(KDRV_DCE_FLAG idx);

void kdrv_ipe_flow_init(void);
SEM_HANDLE *kdrv_ipe_get_sem_id(KDRV_IPE_SEM idx);
ID kdrv_ipe_get_flag_id(KDRV_IPE_FLAG idx);

//void kdrv_rhe_flow_init(void);
//SEM_HANDLE *kdrv_rhe_get_sem_id(KDRV_RHE_SEM idx);
//ID kdrv_rhe_get_flag_id(KDRV_RHE_FLAG idx);

extern void kdrv_ime_flow_init(void);
SEM_HANDLE *kdrv_ime_get_sem_id(KDRV_IME_SEM idx);
ID kdrv_ime_get_flag_id(KDRV_IME_FLAG idx);

extern void kdrv_ife2_flow_init(void);
SEM_HANDLE *kdrv_ife2_get_sem_id(KDRV_IFE2_SEM idx);
ID kdrv_ife2_get_flag_id(KDRV_IFE2_FLAG idx);

extern void kdrv_vpe_flow_init(void);
SEM_HANDLE *kdrv_vpe_get_sem_id(KDRV_VPE_SEM idx);
ID kdrv_vpe_get_flag_id(KDRV_VPE_FLAG idx);

#endif
