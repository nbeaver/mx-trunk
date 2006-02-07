/*
 * Name:     mx_plot.c
 *
 * Purpose:  Support for plotting a data file during a scan.
 *
 * Author:   William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2003, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_scan.h"
#include "mx_plot.h"
#include "mx_driver.h"
#include "p_none.h"
#include "p_child.h"
#include "p_gnuplot.h"

#include "mx_motor.h"
#include "p_gnuplot_xafs.h"

MX_PLOT_TYPE_ENTRY mx_plot_type_list[] = {
	{ MXP_NONE, "none", &mxp_none_function_list },
	{ MXP_CHILD, "child", &mxp_child_function_list },
	{ MXP_GNUPLOT, "gnuplot", &mxp_gnuplot_function_list },
	{ MXP_GNUXAFS, "gnuxafs", &mxp_gnuxafs_function_list },
	{ -1, "", NULL }
};

MX_EXPORT mx_status_type mx_get_plot_type_by_name( 
	MX_PLOT_TYPE_ENTRY *plot_type_list,
	char *name,
	MX_PLOT_TYPE_ENTRY **plot_type_entry )
{
	const char fname[] = "mx_get_plot_type_by_name()";

	char *list_name;
	int i;

	if ( plot_type_list == NULL ) {
		*plot_type_entry = NULL;

		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Plot type list passed was NULL." );
	}

	if ( name == NULL ) {
		*plot_type_entry = NULL;

		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Plot name pointer passed is NULL.");
	}

	for ( i=0; ; i++ ) {
		/* Check for the end of the list. */

		if ( plot_type_list[i].type < 0 ) {
			*plot_type_entry = NULL;

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Plot type '%s' was not found.", name );
		}

		list_name = plot_type_list[i].name;

		if ( list_name == NULL ) {
			*plot_type_entry = NULL;

			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
				"NULL name ptr for plot type %ld.",
				plot_type_list[i].type );
		}

		if ( strcmp( name, list_name ) == 0 ) {
			*plot_type_entry = &( plot_type_list[i] );

			MX_DEBUG( 8,
			("mx_get_plot_type_by_name: ptr = 0x%p, type = %ld",
				*plot_type_entry, (*plot_type_entry)->type));

			return MX_SUCCESSFUL_RESULT;
		}
	}
}

MX_EXPORT mx_status_type mx_get_plot_type_by_value( 
	MX_PLOT_TYPE_ENTRY *plot_type_list,
	long plot_type,
	MX_PLOT_TYPE_ENTRY **plot_type_entry )
{
	const char fname[] = "mx_get_plot_type_by_value()";

	int i;

	if ( plot_type_list == NULL ) {
		*plot_type_entry = NULL;

		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Plot type list passed was NULL." );
	}

	if ( plot_type <= 0 ) {
		*plot_type_entry = NULL;

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Plot type %ld is illegal.", plot_type );
	}

	for ( i=0; ; i++ ) {
		/* Check for the end of the list. */

		if ( plot_type_list[i].type < 0 ) {
			*plot_type_entry = NULL;

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Plot type %ld was not found.", plot_type );
		}

		if ( plot_type_list[i].type == plot_type ) {
			*plot_type_entry = &( plot_type_list[i] );

			MX_DEBUG( 8,
			("mx_get_plot_type_by_value: ptr = 0x%p, type = %ld",
				*plot_type_entry, (*plot_type_entry)->type));

			return MX_SUCCESSFUL_RESULT;
		}
	}
}

MX_EXPORT mx_status_type
mx_plot_open( MX_PLOT *plot )
{
	const char fname[] = "mx_plot_open()";

	MX_PLOT_TYPE_ENTRY *plot_type_entry;
	MX_PLOT_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_PLOT * );
	mx_status_type status;

	if ( plot == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PLOT pointer passed was NULL.");
	}

	status = mx_get_plot_type_by_value(
			mx_plot_type_list, plot->type, &plot_type_entry );

	if ( status.code != MXE_SUCCESS )
		return status;

	if ( plot_type_entry == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"mx_get_plot_type_by_value() returned a NULL plot_type_entry pointer." );
	}

	flist = plot_type_entry->plot_function_list;

	if ( flist == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PLOT_FUNCTION_LIST pointer for plot type '%s' is NULL",
			plot_type_entry->name );
	}

	/* Save a copy of the function list pointer. */

	plot->plot_function_list = flist;

	plot->plot_type_struct = NULL;

	/* Now invoke the open function. */

	fptr = flist->open;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"open function pointer for plot type '%s' is NULL",
			plot_type_entry->name );
	}

	status = (*fptr) ( plot );

	return status;
}

MX_EXPORT mx_status_type
mx_plot_close( MX_PLOT *plot )
{
	const char fname[] = "mx_plot_close()";

	MX_PLOT_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_PLOT * );
	mx_status_type status;

	if ( plot == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PLOT pointer passed was NULL.");
	}

	flist = (MX_PLOT_FUNCTION_LIST *) (plot->plot_function_list);

	if ( flist == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PLOT_FUNCTION_LIST pointer for plot is NULL");
	}

	/* Now invoke the close function. */

	fptr = flist->close;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"close function pointer for plot type '%s' is NULL.",
			plot->typename );
	}

	status = (*fptr) ( plot );

	return status;
}

MX_EXPORT mx_status_type
mx_add_measurement_to_plot_buffer( MX_PLOT *plot )
{
	const char fname[] = "mx_add_measurement_to_plot_buffer()";

	MX_PLOT_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_PLOT * );
	mx_status_type status;

	MX_DEBUG(8,("%s invoked.", fname));

	if ( plot == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PLOT pointer passed was NULL.");
	}

	flist = (MX_PLOT_FUNCTION_LIST *) (plot->plot_function_list);

	if ( flist == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PLOT_FUNCTION_LIST pointer for plot is NULL");
	}

	/* Now invoke the add_measurement_to_plot_buffer function. */

	fptr = flist->add_measurement_to_plot_buffer;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"add_measurement_to_plot_buffer function pointer for plot is NULL." );
	}

	status = (*fptr) ( plot );

	return status;
}

MX_EXPORT mx_status_type
mx_add_array_to_plot_buffer( MX_PLOT *plot,
	long position_type, mx_length_type num_positions, void *position_array,
	long data_type, mx_length_type num_data_points, void *data_array )
{
	const char fname[] = "mx_add_array_to_plot_buffer()";

	MX_PLOT_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_PLOT *, long, mx_length_type, void *,
						long, mx_length_type, void * );
	mx_status_type status;

	MX_DEBUG(8,("%s invoked.", fname));

	if ( plot == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PLOT pointer passed was NULL.");
	}

	flist = (MX_PLOT_FUNCTION_LIST *) (plot->plot_function_list);

	if ( flist == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PLOT_FUNCTION_LIST pointer for plot is NULL");
	}

	/* Now invoke the add_array_to_plot_buffer function. */

	fptr = flist->add_array_to_plot_buffer;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"add_array_to_plot_buffer function pointer for plot is NULL." );
	}

	status = (*fptr) ( plot, position_type, num_positions, position_array,
				data_type, num_data_points, data_array );

	return status;
}

MX_EXPORT mx_status_type
mx_display_plot( MX_PLOT *plot )
{
	const char fname[] = "mx_display_plot()";

	MX_PLOT_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_PLOT * );
	mx_status_type status;

	MX_DEBUG(8,("%s invoked.", fname));

	if ( plot == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PLOT pointer passed was NULL.");
	}

	flist = (MX_PLOT_FUNCTION_LIST *) (plot->plot_function_list);

	if ( flist == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PLOT_FUNCTION_LIST pointer for plot is NULL");
	}

	/* Now invoke the display_plot function. */

	fptr = flist->display_plot;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"display_plot function pointer for plot is NULL." );
	}

	status = (*fptr) ( plot );

	return status;
}

MX_EXPORT mx_status_type
mx_plot_set_x_range( MX_PLOT *plot, double x_min, double x_max )
{
	const char fname[] = "mx_plot_set_x_range()";

	MX_PLOT_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_PLOT *, double, double );
	mx_status_type status;

	if ( plot == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PLOT pointer passed was NULL.");
	}

	flist = (MX_PLOT_FUNCTION_LIST *) (plot->plot_function_list);

	if ( flist == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PLOT_FUNCTION_LIST pointer for plot is NULL");
	}

	/* Now invoke the set_x_range function. */

	fptr = flist->set_x_range;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"set_x_range function pointer for plot is NULL." );
	}

	status = (*fptr) ( plot, x_min, x_max );

	return status;
}

MX_EXPORT mx_status_type
mx_plot_set_y_range( MX_PLOT *plot, double y_min, double y_max )
{
	const char fname[] = "mx_plot_set_y_range()";

	MX_PLOT_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_PLOT *, double, double );
	mx_status_type status;

	if ( plot == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PLOT pointer passed was NULL.");
	}

	flist = (MX_PLOT_FUNCTION_LIST *) (plot->plot_function_list);

	if ( flist == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PLOT_FUNCTION_LIST pointer for plot is NULL");
	}

	/* Now invoke the set_y_range function. */

	fptr = flist->set_y_range;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"set_y_range function pointer for plot is NULL." );
	}

	status = (*fptr) ( plot, y_min, y_max );

	return status;
}

MX_EXPORT mx_status_type
mx_plot_start_plot_section( MX_PLOT *plot )
{
	const char fname[] = "mx_plot_start_plot_section()";

	MX_PLOT_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_PLOT * );
	mx_status_type status;

	MX_DEBUG( 8,("%s invoked.",fname));

	if ( plot == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PLOT pointer passed was NULL.");
	}

	flist = (MX_PLOT_FUNCTION_LIST *) (plot->plot_function_list);

	if ( flist == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PLOT_FUNCTION_LIST pointer for plot is NULL");
	}

	/* Now invoke the start_plot_section function. */

	fptr = flist->start_plot_section;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"start_plot_section function pointer for plot is NULL" );
	}

	status = (*fptr) ( plot );

	return status;
}

MX_EXPORT mx_bool_type
mx_plotting_is_enabled( MX_RECORD *record )
{
	MX_RECORD *list_head_record;
	MX_LIST_HEAD *list_head_struct;
	mx_bool_type enabled;

	if ( record == (MX_RECORD *) NULL )
		return FALSE;

	list_head_record = (MX_RECORD *)(record->list_head);

	list_head_struct = (MX_LIST_HEAD *)
				(list_head_record->record_superclass_struct);

	enabled = list_head_struct->plotting_enabled;

	return enabled;
}

MX_EXPORT void
mx_set_plot_enable( MX_RECORD *record, mx_bool_type enabled )
{
	MX_RECORD *list_head_record;
	MX_LIST_HEAD *list_head_struct;

	if ( record == (MX_RECORD *) NULL )
		return;

	list_head_record = (MX_RECORD *)(record->list_head);

	list_head_struct = (MX_LIST_HEAD *)
				(list_head_record->record_superclass_struct);

	list_head_struct->plotting_enabled = enabled;

	return;
}

MX_EXPORT MX_PLOT_TYPE_ENTRY *
mx_plot_get_type_list( void )
{
	return mx_plot_type_list;
}

static mx_status_type
mx_plot_do_x_command( MX_PLOT *plot, char *command_arguments )
{
	static const char fname[] = "mx_plot_do_x_command()";

	MX_RECORD *motor_record;
	MX_SCAN *scan;
	char record_name[ MXU_RECORD_NAME_LENGTH + 1 ];
	long i;
	int valid_type;
	size_t length;
	char *start_ptr, *end_ptr;

	scan = (MX_SCAN *) plot->scan;

	/* The motor names are separated by comma (,) characters.
	 * By counting the number of commas, we can figure out how
	 * many motors there are.
	 */

	scan->plot.num_x_motors = 1;

	start_ptr = command_arguments;

	while (1) {
		end_ptr = strchr( start_ptr, ',' );

		if ( end_ptr == NULL ) {
			break;			/* Exit the while loop. */
		}

		(scan->plot.num_x_motors)++;

		start_ptr = (++end_ptr);
	}

	scan->plot.x_motor_array = (MX_RECORD **)
		malloc( scan->plot.num_x_motors * sizeof(MX_RECORD *) );

	if ( scan->plot.x_motor_array == (MX_RECORD **) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Unable to allocate a %ld element array of MX_RECORD pointers "
	"for the plot x_motor_array used by scan '%s'.",
			(long) scan->plot.num_x_motors, scan->record->name );
	}

	start_ptr = command_arguments;

	for ( i = 0; i < scan->plot.num_x_motors; i++ ) {
		end_ptr = strchr( start_ptr, ',' );

		if ( end_ptr == NULL ) {
			strlcpy( record_name, start_ptr,
					MXU_RECORD_NAME_LENGTH );
		} else {
			length = end_ptr - start_ptr + 1;

			if ( length > MXU_RECORD_NAME_LENGTH ) {
				length = MXU_RECORD_NAME_LENGTH;
			}

			strlcpy( record_name, start_ptr, length );
		}

		motor_record = mx_get_record( scan->record, record_name );

		if ( motor_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_NOT_FOUND, fname,
		"The motor record '%s' specified by the plot 'x=' command "
		"for scan '%s' does not exist.",
				record_name, scan->record->name );
		}

		valid_type = mx_verify_driver_type( motor_record,
				MXR_DEVICE, MXC_MOTOR, MXT_ANY );

		if ( valid_type == FALSE ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
		"The record '%s' specified by the plot 'x=' command "
		"for scan '%s' is not a motor record.",
				motor_record->name, scan->record->name );
		}

		scan->plot.x_motor_array[i] = motor_record;

		if ( end_ptr != NULL ) {
			start_ptr = (++end_ptr);
		}
	}

#if 0
	for ( i = 0; i < scan->plot.num_x_motors; i++ ) {
		MX_DEBUG(-2,("%s: scan '%s', motor[%ld] = '%s'",
			fname, scan->record->name, i,
			scan->plot.x_motor_array[i]->name));
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_plot_parse_options( MX_PLOT *plot )
{
	static const char fname[] = "mx_plot_parse_options()";

	MX_SCAN *scan;
	char *options, *start_ptr, *end_ptr;
	char *command_arguments;
	char command_name[80];
	char command_buffer[120];
	int last_command;
	size_t length;
	mx_status_type mx_status;

	if ( plot == (MX_PLOT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PLOT pointer passed was NULL." );
	}

	scan = (MX_SCAN *) plot->scan;

	options = plot->options;

	/* The commands in the type arguments are separated by 
	 * semicolon (;) characters.
	 */

	last_command = FALSE;

	start_ptr = options;

	while ( last_command == FALSE ) {

		end_ptr = strchr( start_ptr, ';' );

		if ( end_ptr == NULL ) {
			strlcpy( command_buffer, start_ptr,
					sizeof(command_buffer) );

			last_command = TRUE;
		} else {
			length = end_ptr - start_ptr + 1;

			if ( length > sizeof(command_buffer) - 1 ) {
				length = sizeof(command_buffer) - 1;
			}

			strlcpy( command_buffer, start_ptr, length );

			start_ptr = (++end_ptr);
		}

		/* If the command has arguments, they are separated from
		 * the command name by an equals sign '=' character.
		 */

		end_ptr = strchr( command_buffer, '=' );

		if ( end_ptr == NULL ) {
			strlcpy( command_name, command_buffer,
					sizeof(command_name) );

			command_arguments = NULL;
		} else {
			length = end_ptr - command_buffer + 1;

			if ( length > ( sizeof(command_name) - 1 ) ) {
				length = sizeof(command_name) - 1;
			}

			strlcpy( command_name, command_buffer, length );

			command_arguments = (++end_ptr);
		}

		/* Figure out which command this is and invoke it. */

		length = strlen( command_name );

		if ( strcmp( command_name, "x" ) == 0 ) {
			mx_status = mx_plot_do_x_command( plot,
						command_arguments );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

		} else if ( strncmp( command_name, "continuous_plot",
					length ) == 0 )
		{
			plot->continuous_plot = TRUE;
		} else if ( strncmp( command_name, "normalize_data",
					length ) == 0 )
		{
			plot->normalize_data = TRUE;
		} else if ( strncmp( command_name, "xafs", length ) == 0 ) {

			/* 'xafs' is a special option that turns on
			 * normalization, continuous plot, and sets
			 * the X axis to 'energy'.
			 */

			plot->continuous_plot = TRUE;
			plot->normalize_data = TRUE;

			mx_status = mx_plot_do_x_command( plot, "energy" );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

		} else {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal command '%s' passed in type arguments for scan '%s'.",
				command_name, scan->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

