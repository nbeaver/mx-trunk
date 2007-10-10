/*
 * Name:    mx_camac.h
 *
 * Purpose: General header file for CAMAC crate controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2004, 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_CAMAC_H__
#define __MX_CAMAC_H__

#include "mx_record.h"

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#define MX_CAMAC_Q	1
#define MX_CAMAC_X	2

#define MX_CAMAC_Z	1
#define MX_CAMAC_C	2
#define MX_CAMAC_I	4

/* ======= Macros to convert to and from 24-bit signed integers. ======= */

/* Note:  8388608 = 2**23, 16777216 = 2**24.  */

#if 1
#define MX_FROM_24_TO_32(x) ( (x) > 8388608 ? ((x) - 16777216) : (x) )
#define MX_FROM_32_TO_24(x)   ( (x) < 0 ? ((x) + 16777216) : (x) )
#else
/* Kludge! Don't use this for normal running. */
#define MX_FROM_24_TO_32(x) (x)
#define MX_FROM_32_TO_24(x) (x)
#endif

/* ======  Generic CAMAC crate controller data structure ====== */

typedef struct {
	MX_RECORD *record;
	long branch_number;
	long crate_number;
} MX_CAMAC;

/* MX_CAMAC_STANDARD_FIELDS are the record_fields that should be present
 * in all CAMAC records.  They are located in the MX_RECORD_FIELD array
 * after MX_RECORD_STANDARD_FIELDS.
 */

#define MX_CAMAC_STANDARD_FIELDS \
  {-1, -1, "branch_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_CAMAC, branch_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  {-1, -1, "crate_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_CAMAC, crate_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/*
 * The structure type MX_CAMAC_FUNCTION_LIST contains a list of
 * pointers to functions that vary from controller type to 
 * controller type.
 */

typedef struct {
	mx_status_type ( *get_lam_status ) ( MX_CAMAC *crate, long *lam_n );
	mx_status_type ( *controller_command ) ( MX_CAMAC *crate, long command);
	mx_status_type ( *camac ) ( MX_CAMAC *crate, long slot, long subaddress,
		long function_code, int32_t *data, int *Q, int *X );
} MX_CAMAC_FUNCTION_LIST;

/* ============== Interface function prototypes. ============== */

MX_API mx_status_type mx_camac_get_lam_status( MX_RECORD *camac_record,
						long *lam_status );

MX_API mx_status_type mx_camac_controller_command( MX_RECORD *camac_record,
						long command );

MX_API mx_status_type mx_camac( MX_RECORD *camac_record,
				long slot, long subaddress, long function_code,
				int32_t *data, int *Q, int *X );

MX_API void mx_camac_qwait( MX_RECORD *camac_record,
				long slot, long subaddress, long function_code,
				int32_t *data, int *X );

extern MX_RECORD_FUNCTION_LIST mx_camac_record_function_list;

#ifdef __cplusplus
}
#endif

#endif /* __MX_CAMAC_H__ */
