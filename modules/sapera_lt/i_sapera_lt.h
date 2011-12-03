/*
 * Name:    i_sapera_lt.h
 *
 * Purpose: Header for DALSA's Sapera LT camera interface.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_SAPERA_LT_H__
#define __I_SAPERA_LT_H__

#ifdef __cplusplus

/* Vendor include file. */

#include "SapClassBasic.h"

/*----*/

typedef struct {
	MX_RECORD *record;

	long max_devices;
	MX_RECORD **device_record_array;

	char server_name[CORSERVER_MAX_STRLEN+1];

	long server_index;
	long num_frame_grabbers;
	long num_cameras;

} MX_SAPERA_LT;

#define MXI_SAPERA_LT_STANDARD_FIELDS \
  {-1, -1, "max_devices", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SAPERA_LT, max_devices), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "server_name", MXFT_STRING, NULL, 1, {CORSERVER_MAX_STRLEN}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SAPERA_LT, server_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \

#endif /* __cplusplus */

/* The following data structures must be exported as C symbols. */

#ifdef __cplusplus
extern "C" {
#endif

MX_API mx_status_type mxi_sapera_lt_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_sapera_lt_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_sapera_lt_open( MX_RECORD *record );
MX_API mx_status_type mxi_sapera_lt_close( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_sapera_lt_record_function_list;

extern long mxi_sapera_lt_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_sapera_lt_rfield_def_ptr;

#ifdef __cplusplus
}
#endif

#endif /* __I_SAPERA_LT_H__ */
