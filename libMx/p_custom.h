/*
 * Name:    p_custom.h
 *
 * Purpose: Header file for use with custom plotting routines.
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

#ifndef __P_CUSTOM_H__
#define __P_CUSTOM_H__

typedef struct {
	void *custom_args;
	MX_PLOT_FUNCTION_LIST *custom_function_list;
} MX_PLOT_CUSTOM;

MX_API mx_status_type mxp_custom_open( MX_PLOT *plot );
MX_API mx_status_type mxp_custom_close( MX_PLOT *plot );
MX_API mx_status_type mxp_custom_add_measurement_to_plot_buffer( MX_PLOT *plot);
MX_API mx_status_type mxp_custom_add_array_to_plot_buffer( MX_PLOT *plot,
		long position_type, long num_positions, void *position_array,
		long data_type, long num_data_points, void *data_array );
MX_API mx_status_type mxp_custom_display_plot( MX_PLOT *plot );
MX_API mx_status_type mxp_custom_set_x_range( MX_PLOT *plot,
					double x_min, double x_max );
MX_API mx_status_type mxp_custom_set_y_range( MX_PLOT *plot,
					double y_min, double y_max );
MX_API mx_status_type mxp_custom_start_plot_section( MX_PLOT *plot );

extern MX_PLOT_FUNCTION_LIST mxp_custom_function_list;

#endif /* __P_CUSTOM_H__ */

