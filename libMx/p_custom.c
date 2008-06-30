/*
 * Name:    p_custom.c
 *
 * Purpose: Null plotting support for use with custom plot routines.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXP_CUSTOM_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_plot.h"
#include "mx_scan.h"
#include "p_custom.h"

MX_PLOT_FUNCTION_LIST mxp_custom_function_list = {
		mxp_custom_open,
		mxp_custom_close,
		mxp_custom_add_measurement_to_plot_buffer,
		mxp_custom_add_array_to_plot_buffer,
		mxp_custom_display_plot,
		mxp_custom_set_x_range,
		mxp_custom_set_y_range,
		mxp_custom_start_plot_section,
};

static mx_status_type
mxp_custom_get_pointers( MX_PLOT *plot,
			MX_PLOT_CUSTOM **custom_plot,
			MX_PLOT_FUNCTION_LIST **flist,
			const char *calling_fname )
{
	MX_PLOT_CUSTOM *custom_plot_ptr;
	MX_SCAN *scan;

	if ( plot == (MX_PLOT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, calling_fname,
		"The MX_PLOT pointer passed was NULL." );
	}

	scan = plot->scan;

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, calling_fname,
		"The MX_SCAN pointer is NULL for MX_PLOT %p.", plot );
	}

	custom_plot_ptr = plot->plot_type_struct;

	if ( custom_plot_ptr == (MX_PLOT_CUSTOM *) NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, calling_fname,
		"No custom plot handler has been installed for scan '%s'.",
			scan->record->name );
	}

	if ( custom_plot != (MX_PLOT_CUSTOM **) NULL ) {
		*custom_plot = custom_plot_ptr;
	}

	if ( flist != (MX_PLOT_FUNCTION_LIST **) NULL ) {
		*flist = custom_plot_ptr->custom_function_list;

		if ( (*flist) == (MX_PLOT_FUNCTION_LIST *) NULL ) {
			return mx_error( MXE_INITIALIZATION_ERROR,calling_fname,
			"No custom plot function list has been installed "
			"for scan '%s'.", scan->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxp_custom_open( MX_PLOT *plot )
{
	static const char fname[] = "mxp_custom_open()";

	MX_PLOT_FUNCTION_LIST *flist;
	mx_status_type (*open_fn)( MX_PLOT * );
	mx_status_type mx_status;

#if MXP_CUSTOM_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	/* For custom plots, we do not allocate any data structures here
	 * since they should already have been created by a call to the
	 * mx_scan_set_custom_plot_handler() function.
	 */

	mx_status = mxp_custom_get_pointers( plot, NULL, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	open_fn = flist->open;

	if ( open_fn != NULL ) {
		mx_status = (*open_fn)( plot );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxp_custom_close( MX_PLOT *plot )
{
	static const char fname[] = "mxp_custom_close()";

	MX_PLOT_FUNCTION_LIST *flist;
	mx_status_type (*close_fn)( MX_PLOT * );
	mx_status_type mx_status;

#if MXP_CUSTOM_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	/* We do _not_ free any data structures here, since that is handled
	 * for us by mx_scan_set_custom_plot_handler().
	 */

	mx_status = mxp_custom_get_pointers( plot, NULL, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	close_fn = flist->close;

	if ( close_fn != NULL ) {
		mx_status = (*close_fn)( plot );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxp_custom_add_measurement_to_plot_buffer( MX_PLOT *plot )
{
	static const char fname[] =
		"mxp_custom_add_measurement_to_plot_buffer()";

	MX_PLOT_FUNCTION_LIST *flist;
	mx_status_type (*add_measurement_to_plot_buffer_fn)( MX_PLOT * );
	mx_status_type mx_status;

#if MXP_CUSTOM_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mxp_custom_get_pointers( plot, NULL, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	add_measurement_to_plot_buffer_fn =
		flist->add_measurement_to_plot_buffer;

	if ( add_measurement_to_plot_buffer_fn != NULL ) {
		mx_status = (*add_measurement_to_plot_buffer_fn)( plot );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxp_custom_add_array_to_plot_buffer( MX_PLOT *plot,
		long position_type, long num_positions, void *position_array,
		long data_type, long num_data_points, void *data_array )
{
	static const char fname[] = "mxp_custom_add_array_to_plot_buffer()";

	MX_PLOT_FUNCTION_LIST *flist;
	mx_status_type (*add_array_to_plot_buffer_fn)( MX_PLOT *,
						long, long, void *,
						long, long, void * );
	mx_status_type mx_status;

#if MXP_CUSTOM_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mxp_custom_get_pointers( plot, NULL, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	add_array_to_plot_buffer_fn = flist->add_array_to_plot_buffer;

	if ( add_array_to_plot_buffer_fn != NULL ) {
		mx_status = (*add_array_to_plot_buffer_fn)( plot,
				position_type, num_positions, position_array,
				data_type, num_data_points, data_array );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxp_custom_display_plot( MX_PLOT *plot )
{
	static const char fname[] = "mxp_custom_display_plot()";

	MX_PLOT_FUNCTION_LIST *flist;
	mx_status_type (*display_plot_fn)( MX_PLOT * );
	mx_status_type mx_status;

#if MXP_CUSTOM_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mxp_custom_get_pointers( plot, NULL, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	display_plot_fn = flist->display_plot;

	if ( display_plot_fn != NULL ) {
		mx_status = (*display_plot_fn)( plot );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxp_custom_set_x_range( MX_PLOT *plot, double x_min, double x_max )
{
	static const char fname[] = "mxp_custom_set_x_range()";

	MX_PLOT_FUNCTION_LIST *flist;
	mx_status_type (*set_x_range_fn)( MX_PLOT *, double, double );
	mx_status_type mx_status;

#if MXP_CUSTOM_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mxp_custom_get_pointers( plot, NULL, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_x_range_fn = flist->set_x_range;

	if ( set_x_range_fn != NULL ) {
		mx_status = (*set_x_range_fn)( plot, x_min, x_max );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxp_custom_set_y_range( MX_PLOT *plot, double y_min, double y_max )
{
	static const char fname[] = "mxp_custom_set_y_range()";

	MX_PLOT_FUNCTION_LIST *flist;
	mx_status_type (*set_y_range_fn)( MX_PLOT *, double, double );
	mx_status_type mx_status;

#if MXP_CUSTOM_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mxp_custom_get_pointers( plot, NULL, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_y_range_fn = flist->set_y_range;

	if ( set_y_range_fn != NULL ) {
		mx_status = (*set_y_range_fn)( plot, y_min, y_max );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxp_custom_start_plot_section( MX_PLOT *plot )
{
	static const char fname[] = "mxp_custom_start_plot_section()";

	MX_PLOT_FUNCTION_LIST *flist;
	mx_status_type (*start_plot_section_fn)( MX_PLOT * );
	mx_status_type mx_status;

#if MXP_CUSTOM_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mxp_custom_get_pointers( plot, NULL, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	start_plot_section_fn = flist->start_plot_section;

	if ( start_plot_section_fn != NULL ) {
		mx_status = (*start_plot_section_fn)( plot );
	}

	return mx_status;
}

