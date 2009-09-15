/*
 * Name:    f_xafs.h
 *
 * Purpose: Include file for MRCAT XAFS data file type.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __F_XAFS_H__
#define __F_XAFS_H__

#include "mx_motor.h"

typedef struct {
	FILE     *file;
	MX_RECORD *energy_motor_record;
} MX_DATAFILE_XAFS;

MX_API mx_status_type mxdf_xafs_open( MX_DATAFILE *datafile );
MX_API mx_status_type mxdf_xafs_close( MX_DATAFILE *datafile );
MX_API mx_status_type mxdf_xafs_write_main_header( MX_DATAFILE *datafile );
MX_API mx_status_type mxdf_xafs_write_segment_header( MX_DATAFILE *datafile );
MX_API mx_status_type mxdf_xafs_write_trailer( MX_DATAFILE *datafile );
MX_API mx_status_type mxdf_xafs_add_measurement_to_datafile(
						MX_DATAFILE *datafile );
MX_API mx_status_type mxdf_xafs_add_array_to_datafile( MX_DATAFILE *datafile,
		long position_type, long num_positions, void *position_array,
		long data_type, long num_data_points, void *data_array );

MX_API mx_status_type mxdf_xafs_write_header( MX_DATAFILE *datafile,
						FILE *output_file,
						mx_bool_type is_mca_file );

extern MX_DATAFILE_FUNCTION_LIST mxdf_xafs_datafile_function_list;

#endif /* __F_XAFS_H__ */

