/*
 * Name:    f_custom.c
 *
 * Purpose: Datafile driver for use with custom datafile routines.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXDF_CUSTOM_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_scan.h"
#include "mx_datafile.h"
#include "f_custom.h"

MX_DATAFILE_FUNCTION_LIST mxdf_custom_datafile_function_list = {
	mxdf_custom_open,
	mxdf_custom_close,
	mxdf_custom_write_main_header,
	mxdf_custom_write_segment_header,
	mxdf_custom_write_trailer,
	mxdf_custom_add_measurement_to_datafile,
	mxdf_custom_add_array_to_datafile
};

static mx_status_type
mxdf_custom_get_pointers( MX_DATAFILE *datafile,
			MX_DATAFILE_CUSTOM **custom_datafile,
			MX_DATAFILE_FUNCTION_LIST **flist,
			const char *calling_fname )
{
	MX_DATAFILE_CUSTOM *custom_datafile_ptr;
	MX_SCAN *scan;

	if ( datafile == (MX_DATAFILE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, calling_fname,
		"The MX_DATAFILE pointer passed was NULL." );
	}

	scan = datafile->scan;

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, calling_fname,
		"The MX_SCAN pointer is NULL for MX_DATAFILE %p.", datafile );
	}

	custom_datafile_ptr = datafile->datafile_type_struct;

	if ( custom_datafile_ptr == (MX_DATAFILE_CUSTOM *) NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, calling_fname,
		"No custom datafile handler has been installed for scan '%s'.",
			scan->record->name );
	}

	if ( custom_datafile != (MX_DATAFILE_CUSTOM **) NULL ) {
		*custom_datafile = custom_datafile_ptr;
	}

	if ( flist != (MX_DATAFILE_FUNCTION_LIST **) NULL ) {
		*flist = custom_datafile_ptr->custom_function_list;

		if ( (*flist) == (MX_DATAFILE_FUNCTION_LIST *) NULL ) {
			return mx_error( MXE_INITIALIZATION_ERROR,calling_fname,
			"No custom datafile function list has been installed "
			"for scan '%s'.", scan->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxdf_custom_open( MX_DATAFILE *datafile )
{
	static const char fname[] = "mxdf_custom_open()";

	MX_DATAFILE_FUNCTION_LIST *flist = NULL;
	mx_status_type (*open_fn)( MX_DATAFILE * );
	mx_status_type mx_status;

#if MXDF_CUSTOM_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	/* For custom datafiles, we do not allocate any data structures
	 * here since they should already have been created by a call to
	 * the mx_scan_set_custom_datafile_handler() function.
	 */

	mx_status = mxdf_custom_get_pointers( datafile, NULL, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	open_fn = flist->open;

	if ( open_fn != NULL ) {
		mx_status = (*open_fn)( datafile );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxdf_custom_close( MX_DATAFILE *datafile )
{
	static const char fname[] = "mxdf_custom_close()";

	MX_DATAFILE_FUNCTION_LIST *flist = NULL;
	mx_status_type (*close_fn)( MX_DATAFILE * );
	mx_status_type mx_status;

#if MXDF_CUSTOM_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	/* We do _not_ free any data structures here, since that is handled
	 * for us by mx_scan_set_custom_datafile_handler().
	 */

	mx_status = mxdf_custom_get_pointers( datafile, NULL, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	close_fn = flist->close;

	if ( close_fn != NULL ) {
		mx_status = (*close_fn)( datafile );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxdf_custom_write_main_header( MX_DATAFILE *datafile )
{
	static const char fname[] = "mxdf_custom_write_main_header()";

	MX_DATAFILE_FUNCTION_LIST *flist = NULL;
	mx_status_type (*write_main_header_fn)( MX_DATAFILE * );
	mx_status_type mx_status;

#if MXDF_CUSTOM_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mxdf_custom_get_pointers( datafile, NULL, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	write_main_header_fn = flist->write_main_header;

	if ( write_main_header_fn != NULL ) {
		mx_status = (*write_main_header_fn)( datafile );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxdf_custom_write_segment_header( MX_DATAFILE *datafile )
{
	static const char fname[] = "mxdf_custom_write_segment_header()";

	MX_DATAFILE_FUNCTION_LIST *flist = NULL;
	mx_status_type (*write_segment_header_fn)( MX_DATAFILE * );
	mx_status_type mx_status;

#if MXDF_CUSTOM_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mxdf_custom_get_pointers( datafile, NULL, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	write_segment_header_fn = flist->write_segment_header;

	if ( write_segment_header_fn != NULL ) {
		mx_status = (*write_segment_header_fn)( datafile );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxdf_custom_write_trailer( MX_DATAFILE *datafile )
{
	static const char fname[] = "mxdf_custom_write_trailer()";

	MX_DATAFILE_FUNCTION_LIST *flist = NULL;
	mx_status_type (*write_trailer_fn)( MX_DATAFILE * );
	mx_status_type mx_status;

#if MXDF_CUSTOM_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mxdf_custom_get_pointers( datafile, NULL, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	write_trailer_fn = flist->write_trailer;

	if ( write_trailer_fn != NULL ) {
		mx_status = (*write_trailer_fn)( datafile );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxdf_custom_add_measurement_to_datafile( MX_DATAFILE *datafile )
{
	static const char fname[] = "mxdf_custom_add_measurement_to_datafile()";

	MX_DATAFILE_FUNCTION_LIST *flist = NULL;
	mx_status_type (*add_measurement_to_datafile_fn)( MX_DATAFILE * );
	mx_status_type mx_status;

#if MXDF_CUSTOM_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mxdf_custom_get_pointers( datafile, NULL, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	add_measurement_to_datafile_fn = flist->add_measurement_to_datafile;

	if ( add_measurement_to_datafile_fn != NULL ) {
		mx_status = (*add_measurement_to_datafile_fn)( datafile );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxdf_custom_add_array_to_datafile( MX_DATAFILE *datafile,
		long position_type, long num_positions, void *position_array,
		long data_type, long num_data_points, void *data_array )
{
	static const char fname[] = "mxdf_custom_add_array_to_datafile()";

	MX_DATAFILE_FUNCTION_LIST *flist = NULL;
	mx_status_type (*add_array_to_datafile_fn)( MX_DATAFILE *,
							long, long, void *,
							long, long, void * );
	mx_status_type mx_status;

#if MXDF_CUSTOM_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mxdf_custom_get_pointers( datafile, NULL, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	add_array_to_datafile_fn = flist->add_array_to_datafile;

	if ( add_array_to_datafile_fn != NULL ) {
		mx_status = (*add_array_to_datafile_fn)( datafile,
				position_type, num_positions, position_array,
				data_type, num_data_points, data_array );
	}

	return mx_status;
}

