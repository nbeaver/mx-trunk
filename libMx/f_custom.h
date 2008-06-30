/*
 * Name:    f_custom.h
 *
 * Purpose: Header file for use with custom datafile routines.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __F_CUSTOM_H__
#define __F_CUSTOM_H__

typedef struct {
	void *custom_args;
	MX_DATAFILE_FUNCTION_LIST *custom_function_list;
} MX_DATAFILE_CUSTOM;

MX_API mx_status_type mxdf_custom_open( MX_DATAFILE *datafile );
MX_API mx_status_type mxdf_custom_close( MX_DATAFILE *datafile );
MX_API mx_status_type mxdf_custom_write_main_header( MX_DATAFILE *datafile );
MX_API mx_status_type mxdf_custom_write_segment_header( MX_DATAFILE *datafile );
MX_API mx_status_type mxdf_custom_write_trailer( MX_DATAFILE *datafile );
MX_API mx_status_type mxdf_custom_add_measurement_to_datafile(
						MX_DATAFILE *datafile );
MX_API mx_status_type mxdf_custom_add_array_to_datafile( MX_DATAFILE *datafile,
		long position_type, long num_positions, void *position_array,
		long data_type, long num_data_points, void *data_array );

extern MX_DATAFILE_FUNCTION_LIST mxdf_custom_datafile_function_list;

#endif /* __F_CUSTOM_H__ */

