/*
 * Name:    p_none.h
 *
 * Purpose: Header file for use when plotting is disabled.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __P_NONE_H__
#define __P_NONE_H__

MX_API mx_status_type mxp_none_open( MX_PLOT *plot );
MX_API mx_status_type mxp_none_close( MX_PLOT *plot );
MX_API mx_status_type mxp_none_add_measurement_to_plot_buffer( MX_PLOT *plot );
MX_API mx_status_type mxp_none_add_array_to_plot_buffer( MX_PLOT *plot,
		long position_type, long num_positions, void *position_array,
		long data_type, long num_data_points, void *data_array );
MX_API mx_status_type mxp_none_display_plot( MX_PLOT *plot );
MX_API mx_status_type mxp_none_set_x_range( MX_PLOT *plot,
					double x_min, double x_max );
MX_API mx_status_type mxp_none_set_y_range( MX_PLOT *plot,
					double y_min, double y_max );
MX_API mx_status_type mxp_none_start_plot_section( MX_PLOT *plot );

extern MX_PLOT_FUNCTION_LIST mxp_none_function_list;

#endif /* __P_NONE_H__ */

