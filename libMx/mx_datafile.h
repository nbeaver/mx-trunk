/*
 * Name:     mx_datafile.h
 *
 * Purpose:  Header file to describe output data file support.
 *
 * Author:   William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003, 2007-2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_DATAFILE_H__
#define __MX_DATAFILE_H__

#include "mxconfig.h"
#include "mx_util.h"

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#define MXU_DATAFILE_TYPE_NAME_LENGTH		20

#define MXU_DATAFILE_OPTIONS_LENGTH		80

#define MXU_DATAFILE_DESCRIPTION_LENGTH \
    ( MXU_DATAFILE_TYPE_NAME_LENGTH + MXU_DATAFILE_OPTIONS_LENGTH + 1 )

/* List of datafile types. */

#define MXDF_NONE	1
#define MXDF_CHILD	2
#define MXDF_TEXT	3
#define MXDF_SFF	4
#define MXDF_XAFS 	5
#define MXDF_CUSTOM	6

/* List of datafile_open_flag values. */

#define MXDF_OPEN_OVERWRITE	1
#define MXDF_OPEN_APPEND	2

typedef struct {
	/* Reference to the MX_SCAN that creates this datafile. */
	void *scan;

	long type;
	char mx_typename[MXU_DATAFILE_TYPE_NAME_LENGTH + 1];
	char options[MXU_DATAFILE_OPTIONS_LENGTH + 1];
	char *filename;

	long num_x_motors;
	MX_RECORD **x_motor_array;
	double **x_position_array;

	int normalize_data;

	void *datafile_type_struct;
	void *datafile_function_list;
} MX_DATAFILE;

typedef struct {
	mx_status_type ( *open ) ( MX_DATAFILE *datafile );
	mx_status_type ( *close ) ( MX_DATAFILE *datafile );
	mx_status_type ( *write_main_header ) ( MX_DATAFILE *datafile );
	mx_status_type ( *write_segment_header ) ( MX_DATAFILE *datafile );
	mx_status_type ( *write_trailer ) ( MX_DATAFILE *datafile );
	mx_status_type ( *add_measurement_to_datafile )(MX_DATAFILE *datafile);
	mx_status_type ( *add_array_to_datafile )(MX_DATAFILE *datafile,
		long position_type, long num_positions, void *position_array,
		long data_type, long num_data_points, void *data_array );
} MX_DATAFILE_FUNCTION_LIST;

typedef struct {
	long type;
	char name[MXU_DATAFILE_TYPE_NAME_LENGTH + 1];
	MX_DATAFILE_FUNCTION_LIST *datafile_function_list;
} MX_DATAFILE_TYPE_ENTRY;

MX_API mx_status_type mx_get_datafile_type_by_name(
	MX_DATAFILE_TYPE_ENTRY *datafile_type_list,
	char *name,
	MX_DATAFILE_TYPE_ENTRY **datafile_type_entry );

MX_API mx_status_type mx_get_datafile_type_by_value(
	MX_DATAFILE_TYPE_ENTRY *datafile_type_list,
	long datafile_type,
	MX_DATAFILE_TYPE_ENTRY **datafile_type_entry );

MX_API mx_status_type mx_datafile_open( MX_DATAFILE *datafile );
MX_API mx_status_type mx_datafile_close( MX_DATAFILE *datafile );
MX_API mx_status_type mx_datafile_write_main_header( MX_DATAFILE *datafile );
MX_API mx_status_type mx_datafile_write_segment_header(MX_DATAFILE *datafile);
MX_API mx_status_type mx_datafile_write_trailer( MX_DATAFILE *datafile );
MX_API mx_status_type mx_add_measurement_to_datafile( MX_DATAFILE *datafile );
MX_API mx_status_type mx_add_array_to_datafile( MX_DATAFILE *datafile,
		long position_type, long num_positions, void *position_array,
		long data_type, long num_data_points, void *data_array );

MX_API mx_status_type mx_parse_datafile_name( 
			char *datafile_name, char **version_number_pointer,
			long *version_number, long *version_number_length );

MX_API long mx_construct_datafile_version_number( long version_number_to_test,
						long version_number_length );

MX_API mx_status_type mx_update_datafile_name( MX_DATAFILE *datafile );

MX_API mx_status_type mx_datafile_parse_options( MX_DATAFILE *datafile );

/* One global variable. */

extern MX_DATAFILE_TYPE_ENTRY mx_datafile_type_list[];

#ifdef __cplusplus
}
#endif

#endif /* __MX_DATAFILE_H__ */

