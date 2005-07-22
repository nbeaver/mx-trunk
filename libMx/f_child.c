/*
 * Name:    f_child.c
 *
 * Purpose: Datafile driver for child datafile support.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003 Illinois Institute of Technology
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
#include "mx_driver.h"
#include "mx_scan.h"
#include "mx_datafile.h"
#include "f_child.h"

MX_DATAFILE_FUNCTION_LIST mxdf_child_datafile_function_list = {
	mxdf_child_open,
	mxdf_child_close,
	mxdf_child_write_main_header,
	mxdf_child_write_segment_header,
	mxdf_child_write_trailer,
	mxdf_child_add_measurement_to_datafile,
	mxdf_child_add_array_to_datafile
};

MX_EXPORT mx_status_type
mxdf_child_open( MX_DATAFILE *datafile )
{
	const char fname[] = "mxdf_child_open()";

	MX_SCAN *child_scan, *parent_scan;
	MX_RECORD *parent_scan_record_ptr;
	MX_DATAFILE *parent_datafile;
	MX_DATAFILE_CHILD *child_file_struct;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( datafile == (MX_DATAFILE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DATAFILE pointer passed is NULL." );
	}

	/* The data for this scan are to be written to a datafile that
	 * was opened in the parent scan, so the primary thing to do
	 * in this function is set up pointers to the MX_DATAFILE structure
	 * in the parent scan.
	 */

	child_scan = (MX_SCAN *)(datafile->scan);

	if ( child_scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_SCAN pointer for MX_DATAFILE is NULL." );
	}

	/* For a child scan, the "filename" field contains the name
	 * of the parent scan that opened the datafile.
	 */

	parent_scan_record_ptr = mx_get_record(
		child_scan->record->list_head, datafile->filename );

	if ( parent_scan_record_ptr == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"Parent of child scan '%s' is supposed to be named '%s', "
		"but a record by that name does not exist.",
			child_scan->record->name, datafile->filename );
	}

	if ( parent_scan_record_ptr->mx_superclass != MXR_SCAN ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Parent of child scan '%s' is supposed to be named '%s', "
		"but the record by that name is not a scan record.",
			child_scan->record->name, datafile->filename );
	}

	/* Now setup the datafile_type_struct for this datafile. */

	child_file_struct
		= (MX_DATAFILE_CHILD *) malloc( sizeof(MX_DATAFILE_CHILD) );

	if ( child_file_struct == (MX_DATAFILE_CHILD *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate MX_DATAFILE_CHILD structure for datafile '%s'",
			datafile->filename );
	}

	datafile->datafile_type_struct = child_file_struct;

	parent_scan = (MX_SCAN *)
			(parent_scan_record_ptr->record_superclass_struct);

	if ( parent_scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_SCAN pointer for parent '%s' of child '%s' is NULL.",
			datafile->filename, child_scan->record->name );
	}

	parent_datafile = &(parent_scan->datafile);

	child_file_struct->parent_datafile = parent_datafile;

	/* If we directly pass the parent_datafile structure to the 
	 * functions in the parent's MX_DATAFILE_FUNCTION_LIST, those
	 * functions will attempt to find things out about motors,
	 * scalers, etc. from the parent's MX_SCAN structure.  We want
	 * those functions to look up motors and so forth in the _child_'s
	 * MX_SCAN structure.  Thus, we need to make a local version
	 * of the parent's MX_DATAFILE structure which has been modified
	 * to have datafile->scan point to the child's MX_SCAN structure.
	 */

	child_file_struct->local_parent_copy.type = parent_datafile->type;

	mx_strncpy( child_file_struct->local_parent_copy.typename,
				parent_datafile->typename,
				MXU_DATAFILE_TYPE_NAME_LENGTH );
	
	mx_strncpy( child_file_struct->local_parent_copy.options,
				parent_datafile->options,
				MXU_DATAFILE_OPTIONS_LENGTH );

	child_file_struct->local_parent_copy.filename
				= parent_datafile->filename;
	child_file_struct->local_parent_copy.datafile_type_struct
				= parent_datafile->datafile_type_struct;
	child_file_struct->local_parent_copy.datafile_function_list
				= parent_datafile->datafile_function_list;

	/* And the following is the step that is different. */

	child_file_struct->local_parent_copy.scan = child_scan;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxdf_child_close( MX_DATAFILE *datafile )
{
	const char fname[] = "mxdf_child_close()";

	MX_DATAFILE_CHILD *child_file_struct;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( datafile == (MX_DATAFILE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DATAFILE pointer passed is NULL." );
	}

	child_file_struct
		= (MX_DATAFILE_CHILD *)(datafile->datafile_type_struct);

	if ( child_file_struct == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_DATAFILE_CHILD pointer for datafile '%s' is NULL.",
			datafile->filename );
	}

	free( child_file_struct );

	datafile->datafile_type_struct = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxdf_child_write_main_header( MX_DATAFILE *datafile )
{
	/* A child scan does not write a main header to the datafile. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxdf_child_write_segment_header( MX_DATAFILE *datafile )
{
	const char fname[] = "mxdf_child_write_segment_header()";

	MX_DATAFILE_CHILD *child_file_struct;
	MX_DATAFILE_FUNCTION_LIST *parent_flist;
	mx_status_type( *fptr ) ( MX_DATAFILE * );
	mx_status_type status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( datafile == (MX_DATAFILE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DATAFILE pointer passed is NULL." );
	}

	child_file_struct
		= (MX_DATAFILE_CHILD *)(datafile->datafile_type_struct);

	if ( child_file_struct == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_DATAFILE_CHILD pointer for datafile '%s' is NULL.",
			datafile->filename );
	}

	parent_flist = (MX_DATAFILE_FUNCTION_LIST *)
		child_file_struct->parent_datafile->datafile_function_list;

	if ( parent_flist == (MX_DATAFILE_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_DATAFILE_FUNCTION_LIST pointer for parent scan '%s' is NULL.",
			datafile->filename );
	}

	fptr = parent_flist->write_segment_header;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"write_segment_header function pointer for parent scan '%s' is NULL.",
			datafile->filename );
	}

	status = (*fptr) ( &(child_file_struct->local_parent_copy) );

	return status;
}

MX_EXPORT mx_status_type
mxdf_child_write_trailer( MX_DATAFILE *datafile )
{
	/* A child scan does not write a trailer to the datafile. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxdf_child_add_measurement_to_datafile( MX_DATAFILE *datafile )
{
	const char fname[] = "mxdf_child_add_measurement_to_datafile()";

	MX_DATAFILE_CHILD *child_file_struct;
	MX_DATAFILE_FUNCTION_LIST *parent_flist;
	mx_status_type( *fptr ) ( MX_DATAFILE * );
	mx_status_type status;

	if ( datafile == (MX_DATAFILE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DATAFILE pointer passed is NULL." );
	}

	child_file_struct
		= (MX_DATAFILE_CHILD *)(datafile->datafile_type_struct);

	if ( child_file_struct == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_DATAFILE_CHILD pointer for datafile '%s' is NULL.",
			datafile->filename );
	}

	parent_flist = (MX_DATAFILE_FUNCTION_LIST *)
		child_file_struct->parent_datafile->datafile_function_list;

	if ( parent_flist == (MX_DATAFILE_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_DATAFILE_FUNCTION_LIST pointer for parent scan '%s' is NULL.",
			datafile->filename );
	}

	fptr = parent_flist->add_measurement_to_datafile;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"add_measurement_to_datafile function pointer for parent scan '%s' is NULL.",
			datafile->filename );
	}

	status = (*fptr) ( &(child_file_struct->local_parent_copy) );

	return status;
}

MX_EXPORT mx_status_type
mxdf_child_add_array_to_datafile( MX_DATAFILE *datafile,
		long position_type, long num_positions, void *position_array,
		long data_type, long num_data_points, void *data_array )
{
	const char fname[] = "mxdf_child_add_array_to_datafile()";

	MX_DATAFILE_CHILD *child_file_struct;
	MX_DATAFILE_FUNCTION_LIST *parent_flist;
	mx_status_type( *fptr ) ( MX_DATAFILE *, long, long, void *,
						long, long, void * );
	mx_status_type status;

	if ( datafile == (MX_DATAFILE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DATAFILE pointer passed is NULL." );
	}

	child_file_struct
		= (MX_DATAFILE_CHILD *)(datafile->datafile_type_struct);

	if ( child_file_struct == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_DATAFILE_CHILD pointer for datafile '%s' is NULL.",
			datafile->filename );
	}

	parent_flist = (MX_DATAFILE_FUNCTION_LIST *)
		child_file_struct->parent_datafile->datafile_function_list;

	if ( parent_flist == (MX_DATAFILE_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_DATAFILE_FUNCTION_LIST pointer for parent scan '%s' is NULL.",
			datafile->filename );
	}

	fptr = parent_flist->add_array_to_datafile;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"add_array_to_datafile function pointer for parent scan '%s' is NULL.",
			datafile->filename );
	}

	status = (*fptr) ( &(child_file_struct->local_parent_copy),
				position_type, num_positions, position_array,
				data_type, num_data_points, data_array );

	return status;
}

