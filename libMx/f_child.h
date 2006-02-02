/*
 * Name:    f_child.h
 *
 * Purpose: Include file for child datafile support.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __F_CHILD_H__
#define __F_CHILD_H__

typedef struct {
	MX_DATAFILE *parent_datafile;

	/* Note that the following is _not_ a pointer to a structure,
	 * but the structure itself.
	 */
	MX_DATAFILE local_parent_copy;
} MX_DATAFILE_CHILD;

MX_API mx_status_type mxdf_child_open( MX_DATAFILE *datafile );
MX_API mx_status_type mxdf_child_close( MX_DATAFILE *datafile );
MX_API mx_status_type mxdf_child_write_main_header( MX_DATAFILE *datafile );
MX_API mx_status_type mxdf_child_write_segment_header( MX_DATAFILE *datafile );
MX_API mx_status_type mxdf_child_write_trailer( MX_DATAFILE *datafile );
MX_API mx_status_type mxdf_child_add_measurement_to_datafile(
						MX_DATAFILE *datafile );
MX_API mx_status_type mxdf_child_add_array_to_datafile( MX_DATAFILE *datafile,
	long position_type, mx_length_type num_positions, void *position_array,
	long data_type, mx_length_type num_data_points, void *data_array );

extern MX_DATAFILE_FUNCTION_LIST mxdf_child_datafile_function_list;

#endif /* __F_CHILD_H__ */

