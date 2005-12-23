/*
 * Name:    p_none.c
 *
 * Purpose: Null plotting support for use when plotting is disabled.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_plot.h"
#include "mx_scan.h"
#include "p_none.h"

MX_PLOT_FUNCTION_LIST mxp_none_function_list = {
		mxp_none_open,
		mxp_none_close,
		mxp_none_add_measurement_to_plot_buffer,
		mxp_none_add_array_to_plot_buffer,
		mxp_none_display_plot,
		mxp_none_set_x_range,
		mxp_none_set_y_range,
		mxp_none_start_plot_section,
};

MX_EXPORT mx_status_type
mxp_none_open( MX_PLOT *plot )
{
	const char fname[] = "mxp_none_open()";

	MX_DEBUG( 2,("%s invoked.", fname));

	plot->plot_type_struct = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxp_none_close( MX_PLOT *plot )
{
	const char fname[] = "mxp_none_close()";

	MX_DEBUG( 2,("%s invoked.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxp_none_add_measurement_to_plot_buffer( MX_PLOT *plot )
{
	const char fname[] = "mxp_none_add_measurement_to_plot_buffer()";

	MX_DEBUG( 2,("%s invoked.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxp_none_add_array_to_plot_buffer( MX_PLOT *plot,
		long position_type, long num_positions, void *position_array,
		long data_type, long num_data_points, void *data_array )
{
	const char fname[] = "mxp_none_add_array_to_plot_buffer()";

	MX_DEBUG( 2,("%s invoked.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxp_none_display_plot( MX_PLOT *plot )
{
	const char fname[] = "mxp_none_display_plot()";

	MX_DEBUG( 2,("%s invoked.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxp_none_set_x_range( MX_PLOT *plot, double x_min, double x_max )
{
	const char fname[] = "mxp_none_set_x_range()";

	MX_DEBUG( 2,("%s invoked.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxp_none_set_y_range( MX_PLOT *plot, double y_min, double y_max )
{
	const char fname[] = "mxp_none_set_y_range()";

	MX_DEBUG( 2,("%s invoked.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxp_none_start_plot_section( MX_PLOT *plot )
{
	const char fname[] = "mxp_none_start_plot_section()";

	MX_DEBUG( 2,("%s invoked.", fname));

	return MX_SUCCESSFUL_RESULT;
}

