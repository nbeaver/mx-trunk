/*
 * Name:    f_sff.h
 *
 * Purpose: Include file for simple file format data file type with no header.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __F_SFF_H__
#define __F_SFF_H__

typedef struct {
	FILE *file;
} MX_DATAFILE_SFF;

#define MX_SFF_MAIN_HEADER	1
#define MX_SFF_SEGMENT_HEADER	2
#define MX_SFF_TRAILER		3
#define MX_SFF_MCA_HEADER	4

MX_API mx_status_type mxdf_sff_open( MX_DATAFILE *datafile );
MX_API mx_status_type mxdf_sff_close( MX_DATAFILE *datafile );
MX_API mx_status_type mxdf_sff_write_main_header( MX_DATAFILE *datafile );
MX_API mx_status_type mxdf_sff_write_segment_header( MX_DATAFILE *datafile );
MX_API mx_status_type mxdf_sff_write_trailer( MX_DATAFILE *datafile );
MX_API mx_status_type mxdf_sff_add_measurement_to_datafile(
						MX_DATAFILE *datafile );
MX_API mx_status_type mxdf_sff_add_array_to_datafile( MX_DATAFILE *datafile,
		long position_type, long num_positions, void *position_array,
		long data_type, long num_data_points, void *data_array );

MX_API mx_status_type mxdf_sff_write_header( MX_DATAFILE *datafile,
						FILE *output_file,
						int header_type );

extern MX_DATAFILE_FUNCTION_LIST mxdf_sff_datafile_function_list;

#endif /* __F_SFF_H__ */

