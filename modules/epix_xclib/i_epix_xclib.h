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
 * Copyright 2006-2007, 2011, 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_EPIX_XCLIB_H__
#define __I_EPIX_XCLIB_H__

#define MXU_EPIX_DRIVER_PARMS_LENGTH	80
#define MXU_EPIX_FORMAT_NAME_LENGTH	80

/* An error message buffer length of 1024 is recommended by the XCLIB manual. */

#define MXI_EPIX_ERROR_MESSAGE_LENGTH	1024

/* On x86, the PIXCI unitmap is a 32-bit integer, which limits us
 * to 32 video input boards.  I haven't looked at 64-bit versions
 * of the code yet.
 */

#define MXI_EPIX_MAXIMUM_VIDEO_INPUTS	32

/* Flag values for the 'epix_xclib_flags' field. */

#define MXF_EPIX_USE_CLCCSE_REGISTER    0x1
#define MXF_EPIX_BYTESWAP               0x2
#define MXF_EPIX_SET_AFFINITY           0x4

typedef struct {
	MX_RECORD *record;

	char driver_parms[MXU_EPIX_DRIVER_PARMS_LENGTH+1];
	char format_name[MXU_EPIX_FORMAT_NAME_LENGTH+1];
	char format_file[MXU_FILENAME_LENGTH+1];
	unsigned long epix_xclib_flags;

	mx_bool_type use_high_resolution_time_stamps;

	struct timespec system_boot_timespec;	/* since Jan 1, 1970 */

	unsigned long tick_frequency;		/* ticks per second */

	struct timespec sequence_start_timespec;  /* since Jan 1, 1970 */

	MX_RECORD *video_input_record_array[MXI_EPIX_MAXIMUM_VIDEO_INPUTS];
} MX_EPIX_XCLIB;

#define MXI_EPIX_XCLIB_STANDARD_FIELDS \
  {-1, -1, "driver_parms", MXFT_STRING, NULL, \
					1, {MXU_EPIX_DRIVER_PARMS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPIX_XCLIB, driver_parms), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "format_name", MXFT_STRING, NULL, 1, {MXU_EPIX_FORMAT_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPIX_XCLIB, format_name), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "format_file", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPIX_XCLIB, format_file), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "epix_xclib_flags", MXFT_HEX, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_EPIX_XCLIB, epix_xclib_flags), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxi_epix_xclib_create_record_structures(
						MX_RECORD *record );

MX_API mx_status_type mxi_epix_xclib_open( MX_RECORD *record );

MX_API mx_status_type mxi_epix_xclib_close( MX_RECORD *record );

MX_API mx_status_type mxi_epix_xclib_resynchronize( MX_RECORD *record );

MX_API char *mxi_epix_xclib_error_message( int unitmap,
					int epix_status,
					char *buffer,
					size_t buffer_length );

MX_API struct timespec mxi_epix_xclib_get_buffer_timestamp(
					MX_EPIX_XCLIB *epix_xclib,
					long unitmap,
					long buffer_number );

MX_API mx_status_type mxi_epix_xclib_get_pxvidstatus(
					MX_EPIX_XCLIB *epix_xclib,
					long unitmap,
					struct pxvidstatus *pxstatus,
					int selection_mode );

MX_API mx_status_type mxi_epix_xclib_get_pxbufstatus(
					MX_EPIX_XCLIB *epix_xclib,
					long unitmap,
					long buffer_number,
					struct pxbufstatus *pxstatus );

extern MX_RECORD_FUNCTION_LIST mxi_epix_xclib_record_function_list;

extern long mxi_epix_xclib_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_epix_xclib_rfield_def_ptr;

#endif /* __I_EPIX_XCLIB_H__ */

