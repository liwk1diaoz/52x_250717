/**
	@brief Header file of isf debug log.

	@file isf_debug_log.h

	@ingroup isf_debug

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2021.  All rights reserved.
*/
#ifndef _ISF_DEBUG_LOG_H_
#define _ISF_DEBUG_LOG_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/

/********************************************************************
	TYPE DEFINITION
********************************************************************/

/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
extern int isf_debug_log_open(int buffer_size);
extern int isf_debug_log_close(void);
extern int isf_debug_log_print(const char *fmt, ...);
extern int isf_debug_log_dump(void *s);

#endif  /* _ISF_DEBUG_CORE_H_ */
