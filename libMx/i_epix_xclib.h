/*
 * Name:    i_epix_xclib.h
 *
 * Purpose: Header file for cameras controlled through the EPIX, Inc.
 *          EPIX_XCLIB library.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_EPIX_XCLIB_H__
#define __I_EPIX_XCLIB_H__


typedef struct {
	MX_RECORD *record;

	char format_file[MXU_FILENAME_LENGTH+1];
} MX_EPIX_XCLIB;

#define MXI_EPIX_XCLIB_STANDARD_FIELDS \
  {-1, -1, "format_file", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPIX_XCLIB, format_file), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \

MX_API mx_status_type mxi_epix_xclib_create_record_structures(
						MX_RECORD *record );

MX_API mx_status_type mxi_epix_xclib_open( MX_RECORD *record );

MX_API char *mxi_epix_xclib_error_message( int unitmap,
					int epix_status,
					char *buffer,
					size_t buffer_length );

extern MX_RECORD_FUNCTION_LIST mxi_epix_xclib_record_function_list;

extern long mxi_epix_xclib_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_epix_xclib_rfield_def_ptr;

#endif /* __I_EPIX_XCLIB_H__ */

