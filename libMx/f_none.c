/*
 * Name:    f_none.c
 *
 * Purpose: Datafile driver for dummy datafile support.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_scan.h"
#include "mx_datafile.h"
#include "f_none.h"

MX_DATAFILE_FUNCTION_LIST mxdf_none_datafile_function_list = {
	mxdf_none_open,
	mxdf_none_close,
	mxdf_none_write_main_header,
	mxdf_none_write_segment_header,
	mxdf_none_write_trailer,
	mxdf_none_add_measurement_to_datafile,
	mxdf_none_add_array_to_datafile
};

MX_EXPORT mx_status_type
mxdf_none_open( MX_DATAFILE *datafile )
{
	datafile->datafile_type_struct = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxdf_none_close( MX_DATAFILE *datafile )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxdf_none_write_main_header( MX_DATAFILE *datafile )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxdf_none_write_segment_header( MX_DATAFILE *datafile )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxdf_none_write_trailer( MX_DATAFILE *datafile )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxdf_none_add_measurement_to_datafile( MX_DATAFILE *datafile )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxdf_none_add_array_to_datafile( MX_DATAFILE *datafile,
	long position_type, mx_length_type num_positions, void *position_array,
	long data_type, mx_length_type num_data_points, void *data_array )
{
	return MX_SUCCESSFUL_RESULT;
}

