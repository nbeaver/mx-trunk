/*
 * Name:     mx_plot.h
 *
 * Purpose:  Header file to define functions to plot data during a scan.
 *
 * Author:   William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003, 2006-2008, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_PLOT_H__
#define __MX_PLOT_H__

#include "mxconfig.h"
#include "mx_util.h"

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

/* List of plot types. */

#define MXP_NONE	1
#define MXP_CHILD	2
#define MXP_GNUPLOT	3
#define MXP_GNUXAFS	4
#define MXP_CUSTOM	5

#define MXU_PLOT_TYPE_NAME_LENGTH	20

#define MXU_PLOT_OPTIONS_LENGTH		80

#define MXU_PLOT_DESCRIPTION_LENGTH \
	( MXU_PLOT_TYPE_NAME_LENGTH + MXU_PLOT_OPTIONS_LENGTH + 1 )

#define MXU_PLOT_ARGUMENTS_LENGTH	250

/* Default plotting command. */

#ifndef MXP_PLOTGNU_COMMAND
#define  MXP_PLOTGNU_COMMAND	"perl -S -- plotgnu.pl"
#endif

/* Plot enable flags */

#define MXPF_PLOT_OFF		0
#define MXPF_PLOT_ON		1
#define MXPF_PLOT_NOWAIT	2
#define MXPF_PLOT_END		3

typedef struct {
	/* Reference to the MX_SCAN that invokes this plot. */
	void *scan;

	long type;
	char mx_typename[MXU_PLOT_TYPE_NAME_LENGTH + 1];
	char options[MXU_PLOT_OPTIONS_LENGTH + 1];
	char *plot_arguments;

	long num_x_motors;
	MX_RECORD **x_motor_array;
	double **x_position_array;

	int continuous_plot;
	int normalize_data;

	void *plot_type_struct;
	void *plot_function_list;
	unsigned long section_number;
} MX_PLOT;

typedef struct {
	mx_status_type ( *open ) ( MX_PLOT *plot );
	mx_status_type ( *close ) ( MX_PLOT *plot );
	mx_status_type ( *add_measurement_to_plot_buffer ) ( MX_PLOT *plot );
	mx_status_type ( *add_array_to_plot_buffer ) ( MX_PLOT *plot,
		long position_type, long num_positions, void *position_array,
		long data_type, long num_data_points, void *data_array );
	mx_status_type ( *display_plot ) ( MX_PLOT *plot );
	mx_status_type ( *set_x_range ) ( MX_PLOT *plot,
					double x_min, double x_max );
	mx_status_type ( *set_y_range ) ( MX_PLOT *plot,
					double y_min, double y_max );
	mx_status_type ( *start_plot_section ) ( MX_PLOT *plot );
} MX_PLOT_FUNCTION_LIST;

typedef struct {
	long type;
	char name[MXU_PLOT_TYPE_NAME_LENGTH + 1];
	MX_PLOT_FUNCTION_LIST *plot_function_list;
} MX_PLOT_TYPE_ENTRY;

MX_API mx_status_type mx_get_plot_type_by_name(
	MX_PLOT_TYPE_ENTRY *plot_type_list,
	char *name,
	MX_PLOT_TYPE_ENTRY **plot_type_entry );

MX_API mx_status_type mx_get_plot_type_by_value(
	MX_PLOT_TYPE_ENTRY *plot_type_list,
	long plot_type,
	MX_PLOT_TYPE_ENTRY **plot_type_entry );

MX_API mx_status_type mx_plot_open( MX_PLOT *plot );
MX_API mx_status_type mx_plot_close( MX_PLOT *plot );
MX_API mx_status_type mx_add_measurement_to_plot_buffer( MX_PLOT *plot );
MX_API mx_status_type mx_add_array_to_plot_buffer( MX_PLOT *plot,
		long position_type, long num_positions, void *position_array,
		long data_type, long num_data_points, void *data_array );
MX_API mx_status_type mx_display_plot( MX_PLOT * );
MX_API mx_status_type mx_plot_set_x_range( MX_PLOT *plot,
				double x_min, double x_max );
MX_API mx_status_type mx_plot_set_y_range( MX_PLOT *plot,
				double y_min, double y_max );
MX_API mx_status_type mx_plot_start_plot_section( MX_PLOT *plot );

MX_API int mx_plotting_is_enabled( MX_RECORD *record );
MX_API void mx_set_plot_enable( MX_RECORD *record, int enable );

MX_API MX_PLOT_TYPE_ENTRY *mx_plot_get_type_list( void );

MX_API mx_status_type mx_plot_parse_options( MX_PLOT *plot );

/* One global variable. */

extern MX_PLOT_TYPE_ENTRY mx_plot_type_list[];

#ifdef __cplusplus
}
#endif

#endif /* __MX_PLOT_H__ */

