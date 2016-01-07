/*
 * Name:    i_dalsa_gev.h
 *
 * Purpose: Header for the DALSA Gev API camera interface for
 *          DALSA GigE-Vision cameras.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_DALSA_GEV_H__
#define __I_DALSA_GEV_H__

/* Vendor include file. */

#include "gevapi.h"

/* Flag bits for the 'dalsa_gev_flags' field. */

#define MXF_DALSA_GEV_SHOW_CAMERA_LIST	0x1

/*----*/

typedef struct {
	MX_RECORD *record;


	long num_cameras;
	GEV_CAMERA_INFO *camera_array;

	MX_RECORD **device_record_array;

	unsigned long dalsa_gev_flags;
} MX_DALSA_GEV;

#define MXI_DALSA_GEV_STANDARD_FIELDS \
  {-1, -1, "num_cameras", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DALSA_GEV, num_cameras), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_READ_ONLY) }, \
  \
  {-1, -1, "dalsa_gev_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DALSA_GEV, dalsa_gev_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }

/* The following data structures must be exported as C symbols. */

MX_API mx_status_type mxi_dalsa_gev_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_dalsa_gev_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_dalsa_gev_open( MX_RECORD *record );
MX_API mx_status_type mxi_dalsa_gev_close( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_dalsa_gev_record_function_list;

extern long mxi_dalsa_gev_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_dalsa_gev_rfield_def_ptr;

#endif /* __I_DALSA_GEV_H__ */
