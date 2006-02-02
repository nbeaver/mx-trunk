/*
 * Name:    p_child.c
 *
 * Purpose: Plotting support for use when a child scan is using the plotting
 *          type specified in the parent.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_plot.h"
#include "mx_scan.h"
#include "p_child.h"

MX_PLOT_FUNCTION_LIST mxp_child_function_list = {
		mxp_child_open,
		mxp_child_close,
		mxp_child_add_measurement_to_plot_buffer,
		mxp_child_add_array_to_plot_buffer,
		mxp_child_display_plot,
		mxp_child_set_x_range,
		mxp_child_set_y_range,
		mxp_child_start_plot_section,
};

MX_EXPORT mx_status_type
mxp_child_open( MX_PLOT *plot )
{
	static const char fname[] = "mxp_child_open()";

	MX_SCAN *child_scan, *parent_scan;
	MX_RECORD *parent_scan_record_ptr;
	MX_PLOT *parent_plot;
	MX_PLOT_CHILD *child_file_struct;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( plot == (MX_PLOT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PLOT pointer passed is NULL." );
	}

	/* The data for this scan are to be written to a plot that
	 * was opened in the parent scan, so the primary thing to do
	 * in this function is set up pointers to the MX_PLOT structure
	 * in the parent scan.
	 */

	child_scan = (MX_SCAN *)(plot->scan);

	if ( child_scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_SCAN pointer for MX_PLOT is NULL." );
	}

	/* For a child scan, the "plot arguments" field contains the name
	 * of the parent scan that opened the plot.  This may not
	 * be true in some later version of the code, but for now
	 * we can use it as a consistency check.
	 */

	parent_scan_record_ptr = mx_get_record(
		child_scan->record->list_head, plot->plot_arguments );

	if ( parent_scan_record_ptr == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"Parent of child scan '%s' is supposed to be named '%s', "
		"but a record by that name does not exist.",
			child_scan->record->name, plot->plot_arguments );
	}

	if ( parent_scan_record_ptr->mx_superclass != MXR_SCAN ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Parent of child scan '%s' is supposed to be named '%s', "
		"but the record by that name is not a scan record.",
			child_scan->record->name, plot->plot_arguments );
	}

#if 0
	if ( parent_scan_record_ptr != child_scan->parent_scan_record ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"Pointers to parent scan of child scan '%s' differ.  "
	"parent_scan_record_ptr = %p, child_scan->parent_scan_record = %p.",
			plot->plot_arguments,
			parent_scan_record_ptr,
			child_scan->parent_scan_record );
	}
#endif

	/* Now that all of the consistency checking is out of the way,
	 * we may now setup the plot_type_struct for this plot.
	 */

	child_file_struct
		= (MX_PLOT_CHILD *) malloc( sizeof(MX_PLOT_CHILD) );

	if ( child_file_struct == (MX_PLOT_CHILD *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate MX_PLOT_CHILD structure for plot '%s'",
			plot->plot_arguments );
	}

	plot->plot_type_struct = child_file_struct;

	parent_scan = (MX_SCAN *)
			(parent_scan_record_ptr->record_superclass_struct);

	if ( parent_scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_SCAN pointer for parent '%s' of child '%s' is NULL.",
			plot->plot_arguments, child_scan->record->name );
	}

	parent_plot = &(parent_scan->plot);

	child_file_struct->parent_plot = parent_plot;

	/* If we directly pass the parent_plot structure to the 
	 * functions in the parent's MX_PLOT_FUNCTION_LIST, those
	 * functions will attempt to find things out about motors,
	 * scalers, etc. from the parent's MX_SCAN structure.  We want
	 * those functions to look up motors and so forth in the _child_'s
	 * MX_SCAN structure.  Thus, we need to make a local version
	 * of the parent's MX_PLOT structure which has been modified
	 * to have plot->scan point to the child's MX_SCAN structure.
	 */

	child_file_struct->local_parent_copy.type = parent_plot->type;

	strlcpy( child_file_struct->local_parent_copy.typename,
				parent_plot->typename,
				MXU_PLOT_TYPE_NAME_LENGTH );

	strlcpy( child_file_struct->local_parent_copy.options,
				parent_plot->options,
				MXU_PLOT_OPTIONS_LENGTH );

	child_file_struct->local_parent_copy.plot_arguments
				= parent_plot->plot_arguments;
	child_file_struct->local_parent_copy.plot_type_struct
				= parent_plot->plot_type_struct;
	child_file_struct->local_parent_copy.plot_function_list
				= parent_plot->plot_function_list;

	/* And the following is the step that is different. */

	child_file_struct->local_parent_copy.scan = child_scan;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxp_child_close( MX_PLOT *plot )
{
	static const char fname[] = "mxp_child_close()";

	MX_PLOT_CHILD *child_file_struct;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( plot == (MX_PLOT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PLOT pointer passed is NULL." );
	}

	child_file_struct
		= (MX_PLOT_CHILD *)(plot->plot_type_struct);

	if ( child_file_struct == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PLOT_CHILD pointer for plot '%s' is NULL.",
			plot->plot_arguments );
	}

	free( child_file_struct );

	plot->plot_type_struct = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxp_child_add_measurement_to_plot_buffer( MX_PLOT *plot )
{
	static const char fname[] = "mxp_child_add_measurement_to_plot_buffer()";

	MX_PLOT_CHILD *child_file_struct;
	MX_PLOT_FUNCTION_LIST *parent_flist;
	mx_status_type( *fptr ) ( MX_PLOT * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( plot == (MX_PLOT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PLOT pointer passed is NULL." );
	}

	child_file_struct
		= (MX_PLOT_CHILD *)(plot->plot_type_struct);

	if ( child_file_struct == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PLOT_CHILD pointer for plot '%s' is NULL.",
			plot->plot_arguments );
	}

	parent_flist = (MX_PLOT_FUNCTION_LIST *)
		child_file_struct->parent_plot->plot_function_list;

	if ( parent_flist == (MX_PLOT_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_PLOT_FUNCTION_LIST pointer for parent scan '%s' is NULL.",
			plot->plot_arguments );
	}

	fptr = parent_flist->add_measurement_to_plot_buffer;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"add_measurement_to_plot_buffer function pointer for parent scan '%s' is NULL.",
			plot->plot_arguments );
	}

	mx_status = (*fptr) ( &(child_file_struct->local_parent_copy) );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxp_child_add_array_to_plot_buffer( MX_PLOT *plot,
	long position_type, mx_length_type num_positions, void *position_array,
	long data_type, mx_length_type num_data_points, void *data_array )
{
	static const char fname[] = "mxp_child_add_array_to_plot_buffer()";

	MX_PLOT_CHILD *child_file_struct;
	MX_PLOT_FUNCTION_LIST *parent_flist;
	mx_status_type( *fptr ) ( MX_PLOT *, long, mx_length_type, void *,
					long, mx_length_type, void * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( plot == (MX_PLOT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PLOT pointer passed is NULL." );
	}

	child_file_struct
		= (MX_PLOT_CHILD *)(plot->plot_type_struct);

	if ( child_file_struct == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PLOT_CHILD pointer for plot '%s' is NULL.",
			plot->plot_arguments );
	}

	parent_flist = (MX_PLOT_FUNCTION_LIST *)
		child_file_struct->parent_plot->plot_function_list;

	if ( parent_flist == (MX_PLOT_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_PLOT_FUNCTION_LIST pointer for parent scan '%s' is NULL.",
			plot->plot_arguments );
	}

	fptr = parent_flist->add_array_to_plot_buffer;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"add_array_to_plot_buffer function pointer for parent scan '%s' is NULL.",
			plot->plot_arguments );
	}

	mx_status = (*fptr) ( &(child_file_struct->local_parent_copy),
				position_type, num_positions, position_array,
				data_type, num_data_points, data_array );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxp_child_display_plot( MX_PLOT *plot )
{
	static const char fname[] = "mxp_child_display_plot()";

	MX_PLOT_CHILD *child_file_struct;
	MX_PLOT_FUNCTION_LIST *parent_flist;
	mx_status_type( *fptr ) ( MX_PLOT * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( plot == (MX_PLOT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PLOT pointer passed is NULL." );
	}

	child_file_struct
		= (MX_PLOT_CHILD *)(plot->plot_type_struct);

	if ( child_file_struct == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PLOT_CHILD pointer for plot '%s' is NULL.",
			plot->plot_arguments );
	}

	parent_flist = (MX_PLOT_FUNCTION_LIST *)
		child_file_struct->parent_plot->plot_function_list;

	if ( parent_flist == (MX_PLOT_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_PLOT_FUNCTION_LIST pointer for parent scan '%s' is NULL.",
			plot->plot_arguments );
	}

	fptr = parent_flist->display_plot;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"display_plot function pointer for parent scan '%s' is NULL.",
			plot->plot_arguments );
	}

	mx_status = (*fptr) ( &(child_file_struct->local_parent_copy) );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxp_child_set_x_range( MX_PLOT *plot, double x_min, double x_max )
{
	static const char fname[] = "mxp_child_set_x_range()";

	MX_PLOT_CHILD *child_file_struct;
	MX_PLOT_FUNCTION_LIST *parent_flist;
	mx_status_type( *fptr ) ( MX_PLOT *, double, double );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( plot == (MX_PLOT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PLOT pointer passed is NULL." );
	}

	child_file_struct
		= (MX_PLOT_CHILD *)(plot->plot_type_struct);

	if ( child_file_struct == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PLOT_CHILD pointer for plot '%s' is NULL.",
			plot->plot_arguments );
	}

	parent_flist = (MX_PLOT_FUNCTION_LIST *)
		child_file_struct->parent_plot->plot_function_list;

	if ( parent_flist == (MX_PLOT_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_PLOT_FUNCTION_LIST pointer for parent scan '%s' is NULL.",
			plot->plot_arguments );
	}

	fptr = parent_flist->set_x_range;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"set_x_range function pointer for parent scan '%s' is NULL.",
			plot->plot_arguments );
	}

	mx_status = (*fptr) ( &(child_file_struct->local_parent_copy),
						x_min, x_max );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxp_child_set_y_range( MX_PLOT *plot, double y_min, double y_max )
{
	static const char fname[] = "mxp_child_set_y_range()";

	MX_PLOT_CHILD *child_file_struct;
	MX_PLOT_FUNCTION_LIST *parent_flist;
	mx_status_type( *fptr ) ( MX_PLOT *, double, double );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( plot == (MX_PLOT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PLOT pointer passed is NULL." );
	}

	child_file_struct
		= (MX_PLOT_CHILD *)(plot->plot_type_struct);

	if ( child_file_struct == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PLOT_CHILD pointer for plot '%s' is NULL.",
			plot->plot_arguments );
	}

	parent_flist = (MX_PLOT_FUNCTION_LIST *)
		child_file_struct->parent_plot->plot_function_list;

	if ( parent_flist == (MX_PLOT_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_PLOT_FUNCTION_LIST pointer for parent scan '%s' is NULL.",
			plot->plot_arguments );
	}

	fptr = parent_flist->set_y_range;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"set_y_range function pointer for parent scan '%s' is NULL.",
			plot->plot_arguments );
	}

	mx_status = (*fptr) ( &(child_file_struct->local_parent_copy),
						y_min, y_max );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxp_child_start_plot_section( MX_PLOT *plot )
{
	static const char fname[] = "mxp_child_start_plot_section()";

	MX_PLOT *parent_plot;
	MX_PLOT_CHILD *child_file_struct;
	MX_PLOT_FUNCTION_LIST *parent_flist;
	mx_status_type( *fptr ) ( MX_PLOT * );
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( plot == (MX_PLOT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PLOT pointer passed is NULL." );
	}

	child_file_struct
		= (MX_PLOT_CHILD *)(plot->plot_type_struct);

	if ( child_file_struct == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PLOT_CHILD pointer for plot '%s' is NULL.",
			plot->plot_arguments );
	}

	parent_plot = child_file_struct->parent_plot;

	if ( parent_plot == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"parent_plot pointer for plot '%s' is NULL.",
			plot->plot_arguments );
	}

	/* If the parent scan has asked for continuous plotting with
	 * all of the child scans on the same plot, then we must not 
	 * start a new plot section here.
	 */

	if ( parent_plot->continuous_plot ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Otherwise, tell the parent plot to start a new plot section. */

	parent_flist = (MX_PLOT_FUNCTION_LIST *)
				parent_plot->plot_function_list;

	if ( parent_flist == (MX_PLOT_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_PLOT_FUNCTION_LIST pointer for parent scan '%s' is NULL.",
			plot->plot_arguments );
	}

	fptr = parent_flist->start_plot_section;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"start_plot_section function pointer for parent scan '%s' is NULL.",
			plot->plot_arguments );
	}

	mx_status = (*fptr) ( &(child_file_struct->local_parent_copy) );

	return MX_SUCCESSFUL_RESULT;
}

