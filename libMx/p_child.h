/*
 * Name:    p_child.h
 *
 * Purpose: Header file for use when a child scan is using the plotting
 *          type specified in the parent.
 *
 * Author:  William Lavender
 *
 *-----------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __P_CHILD_H__
#define __P_CHILD_H__

typedef struct {
	MX_PLOT *parent_plot;

	/* Note that the following is _not_ a pointer to a structure,
	 * but the structure itself.
	 */
	MX_PLOT local_parent_copy;
} MX_PLOT_CHILD;

MX_API mx_status_type mxp_child_open( MX_PLOT *plot );

MX_API mx_status_type mxp_child_close( MX_PLOT *plot );

MX_API mx_status_type mxp_child_add_measurement_to_plot_buffer( MX_PLOT *plot );

MX_API mx_status_type mxp_child_add_array_to_plot_buffer( MX_PLOT *plot,
	long position_type, mx_length_type num_positions, void *position_array,
	long data_type, mx_length_type num_data_points, void *data_array );

MX_API mx_status_type mxp_child_display_plot( MX_PLOT *plot );

MX_API mx_status_type mxp_child_set_x_range( MX_PLOT *plot,
					double x_min, double x_max );

MX_API mx_status_type mxp_child_set_y_range( MX_PLOT *plot,
					double y_min, double y_max );

MX_API mx_status_type mxp_child_start_plot_section( MX_PLOT *plot );

extern MX_PLOT_FUNCTION_LIST mxp_child_function_list;

#endif /* __P_CHILD_H__ */

