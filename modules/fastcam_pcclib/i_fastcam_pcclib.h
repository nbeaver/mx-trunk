/*
 * Name:    i_fastcam_pcclib.h
 *
 * Purpose: Header for Photron's FASTCAM PccLib library.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_FASTCAM_PCCLIB_H__
#define __I_FASTCAM_PCCLIB_H__

#ifdef __cplusplus

/* Vendor include file. */

#include "PccLib.h"

/*----*/

typedef struct {
	MX_RECORD *record;

	long max_cameras;

	long num_cameras;
	MX_RECORD **camera_record_array;

} MX_FASTCAM_PCCLIB;

#define MXI_FASTCAM_PCCLIB_STANDARD_FIELDS \
  {-1, -1, "max_cameras", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FASTCAM_PCCLIB, max_cameras), \
	{0}, NULL, MXFF_IN_DESCRIPTION }

#endif /* __cplusplus */

/* The following data structures must be exported as C symbols. */

#ifdef __cplusplus
extern "C" {
#endif

MX_API mx_status_type mxi_fastcam_pcclib_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_fastcam_pcclib_open( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_fastcam_pcclib_record_function_list;

extern long mxi_fastcam_pcclib_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_fastcam_pcclib_rfield_def_ptr;

#ifdef __cplusplus
}
#endif

#endif /* __I_FASTCAM_PCCLIB_H__ */
