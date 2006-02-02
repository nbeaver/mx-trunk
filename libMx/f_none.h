/*
 * Name:    f_none.h
 *
 * Purpose: Include file for dummy datafile support.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __F_NONE_H__
#define __F_NONE_H__

typedef struct {
	int placeholder;
} MX_DATAFILE_NONE;

MX_API mx_status_type mxdf_none_open( MX_DATAFILE *datafile );
MX_API mx_status_type mxdf_none_close( MX_DATAFILE *datafile );
MX_API mx_status_type mxdf_none_write_main_header( MX_DATAFILE *datafile );
MX_API mx_status_type mxdf_none_write_segment_header( MX_DATAFILE *datafile );
MX_API mx_status_type mxdf_none_write_trailer( MX_DATAFILE *datafile );
MX_API mx_status_type mxdf_none_add_measurement_to_datafile(
						MX_DATAFILE *datafile );
MX_API mx_status_type mxdf_none_add_array_to_datafile( MX_DATAFILE *datafile,
	long position_type, mx_length_type num_positions, void *position_array,
	long data_type, mx_length_type num_data_points, void *data_array );

extern MX_DATAFILE_FUNCTION_LIST mxdf_none_datafile_function_list;

#endif /* __F_NONE_H__ */

