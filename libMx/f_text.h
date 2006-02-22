/*
 * Name:    f_text.h
 *
 * Purpose: Include file for simple text data file type with no header.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __F_TEXT_H__
#define __F_TEXT_H__

typedef struct {
	FILE *file;
} MX_DATAFILE_TEXT;

MX_API mx_status_type mxdf_text_open( MX_DATAFILE *datafile );
MX_API mx_status_type mxdf_text_close( MX_DATAFILE *datafile );
MX_API mx_status_type mxdf_text_write_main_header( MX_DATAFILE *datafile );
MX_API mx_status_type mxdf_text_write_segment_header( MX_DATAFILE *datafile );
MX_API mx_status_type mxdf_text_write_trailer( MX_DATAFILE *datafile );
MX_API mx_status_type mxdf_text_add_measurement_to_datafile(
						MX_DATAFILE *datafile );
MX_API mx_status_type mxdf_text_add_array_to_datafile( MX_DATAFILE *datafile,
		long position_type, long num_positions, void *position_array,
		long data_type, long num_data_points, void *data_array );

extern MX_DATAFILE_FUNCTION_LIST mxdf_text_datafile_function_list;

#endif /* __F_TEXT_H__ */

