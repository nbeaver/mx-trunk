/*
 * Name:    p_gnuplot.c
 *
 * Purpose: "gnuplot" plotting support.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2003 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_driver.h"
#include "mx_plot.h"
#include "mx_scan.h"
#include "mx_scan_linear.h"
#include "p_gnuplot.h"

#ifdef OS_WIN32
#define popen _popen
#define pclose _pclose
#endif

MX_PLOT_FUNCTION_LIST mxp_gnuplot_function_list = {
		mxp_gnuplot_open,
		mxp_gnuplot_close,
		mxp_gnuplot_add_measurement_to_plot_buffer,
		mxp_gnuplot_add_array_to_plot_buffer,
		mxp_gnuplot_display_plot,
		mxp_gnuplot_set_x_range,
		mxp_gnuplot_set_y_range,
		mxp_gnuplot_start_plot_section,
};

#define CHECK_PLOTGNU_STATUS \
	if ( status == EOF ) { \
		saved_errno = errno; \
		return mx_error( MXE_IPC_IO_ERROR, fname, \
		"Error writing data to 'plotgnu'.  Reason = '%s'", \
			strerror( saved_errno ) ); \
	}

MX_EXPORT mx_status_type
mxp_gnuplot_open( MX_PLOT *plot )
{
	const char fname[] = "mxp_gnuplot_open()";

	MX_SCAN *scan;
	MX_PLOT_GNUPLOT *gnuplot_data;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( plot == (MX_PLOT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_PLOT pointer passed was NULL." );
	}

	scan = (MX_SCAN *)( plot->scan );

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_SCAN pointer for this plot is NULL." );
	}

	gnuplot_data = (MX_PLOT_GNUPLOT *) malloc( sizeof(MX_PLOT_GNUPLOT) );

	if ( gnuplot_data == (MX_PLOT_GNUPLOT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Out of memory trying to allocate gnuplot data structure.");
	}

	plot->plot_type_struct = gnuplot_data;

	gnuplot_data->plotfile_step_count = 0;

	/* Try to open the pipe to 'plotgnu' */

#if defined(OS_VXWORKS)
	return mx_error( MXE_UNSUPPORTED, fname,
	  "Plotting with Gnuplot is not supported for this operating system." );
#else
	gnuplot_data->pipe = popen( "perl -S -- plotgnu.pl", "w" );
#endif

	if ( gnuplot_data->pipe == NULL ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"Unable to start the 'plotgnu' program." );
	}

	/*** Set stream buffering to line buffered. ***/

	setvbuf( gnuplot_data->pipe, (char *)NULL, _IOLBF, 0);

	/* Suppose this plot is to be used by a parent scan that has
	 * multiple child scans such as an XAFS scan which has a separate
	 * child scan for each region.  If so, then we may want all of
	 * the scan regions to appear on the same plot rather than on
	 * separate plots for each region.  To do this, we must turn on
	 * the 'continuous_plot' option in the parent scan.  If the
	 * 'continuous_plot' option is set, then the child scans will
	 * not attempt to start new plot sections.  However, that means
	 * that the parent scan must start a plot section of its own
	 * right now before any of the child scans are run.
	 */

	if ( plot->continuous_plot ) {
		mx_status = mxp_gnuplot_start_plot_section( plot );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxp_gnuplot_close( MX_PLOT *plot )
{
	const char fname[] = "mxp_gnuplot_close()";

	MX_PLOT_GNUPLOT *gnuplot_data;
	int status;

	MX_DEBUG( 2,("%s invoked.", fname));

	gnuplot_data = (MX_PLOT_GNUPLOT *) (plot->plot_type_struct);

	if ( gnuplot_data == NULL ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"A connection to 'plotgnu' is not currently active.");
	}

	if ( gnuplot_data->pipe == NULL ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"The most recent attempt to connect to 'gnuplot' failed.");
	}

	if ( fprintf( gnuplot_data->pipe, "exit\n" ) < 0 ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"An attempt to send the 'exit' command to 'plotgnu' failed." );
	}

#if defined(OS_VXWORKS)
	status = EOF;
#else
	status = pclose( gnuplot_data->pipe );
#endif

	if ( status == EOF ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
"An error occurred while trying to close the connection to 'plotgnu'");
	}

	gnuplot_data->pipe = NULL;

	free( gnuplot_data );

	plot->plot_type_struct = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxp_gnuplot_add_measurement_to_plot_buffer( MX_PLOT *plot )
{
	const char fname[] = "mxp_gnuplot_add_measurement_to_plot_buffer()";

	MX_SCAN *scan;
	MX_PLOT_GNUPLOT *gnuplot_data;
	MX_RECORD **input_device_array;
	MX_RECORD *input_device;
	MX_RECORD *x_motor_record;
	double *motor_position;
	double normalization;
	char buffer[80];
	long i;
	int status, saved_errno;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	/* ---- Get and check a zillion pointers. ---- */

	if ( plot == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PLOT pointer passed was NULL." );
	}

	scan = (MX_SCAN *) (plot->scan);

	if ( scan == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Scan pointer for MX_PLOT pointer = %p was NULL.",
			plot );
	}

	if ( scan->num_motors <= 0 ) {
		motor_position = NULL;
	} else {
		motor_position = scan->motor_position;

		if ( motor_position == NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The motor current position array pointer is NULL." );
		}
	}

	input_device_array = scan->input_device_array;

	if ( input_device_array == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'input_device_array' pointer is NULL." );
	}

	gnuplot_data = (MX_PLOT_GNUPLOT *) (plot->plot_type_struct);

	if ( gnuplot_data == NULL ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"A connection to 'plotgnu' is not currently active.");
	}

	if ( gnuplot_data->pipe == NULL ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"The most recent attempt to connect to 'plotgnu' failed.");
	}

	/* ---- Write the most recent measurement out to plotgnu. ---- */

	status = fprintf( gnuplot_data->pipe, "data" );

	CHECK_PLOTGNU_STATUS;

	/* Send the current motor positions (if any ). */

	if ( scan->plot.num_x_motors == 0 ) {

		/* By default, we use the axes being scanned. */

		if ( scan->num_motors == 0 ) {
			status = fprintf( gnuplot_data->pipe,
				" %3ld", gnuplot_data->plotfile_step_count );

			CHECK_PLOTGNU_STATUS;

			(gnuplot_data->plotfile_step_count)++;
		} else {
			for ( i = 0; i < scan->num_motors; i++ ) {
				if ( (scan->motor_is_independent_variable)[i] )
				{
					status = fprintf( gnuplot_data->pipe,
						" %g", motor_position[i] );

					CHECK_PLOTGNU_STATUS;
				}
			}
		}
	} else {
		/* However, if alternate X axis motors have been specified,
		 * we display them instead.
		 */

		if ( scan->plot.x_position_array == (double **) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The alternate x_position_array pointer for scan '%s' is NULL.",
				scan->record->name );
		}

		for ( i = 0; i < scan->plot.num_x_motors; i++ ) {
			x_motor_record = scan->plot.x_motor_array[i];

			status = fprintf( gnuplot_data->pipe,
				" %g", scan->plot.x_position_array[i][0] );

			CHECK_PLOTGNU_STATUS;
		}
	}

	/* If we were requested to normalize the data, the 'normalization'
	 * variable is set to the scan measurement time.  Otherwise,
	 * 'normalization' is set to -1.0.
	 */

	if ( scan->plot.normalize_data ) {
		normalization = mx_scan_get_measurement_time( scan );
	} else {
		normalization = -1.0;
	}

	for ( i = 0; i < scan->num_input_devices; i++ ) {
		input_device = input_device_array[i];

		mx_status = mx_convert_normalized_device_value_to_string(
				input_device, normalization,
				buffer, sizeof(buffer)-1 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		status = fprintf( gnuplot_data->pipe, " %s", buffer );

		CHECK_PLOTGNU_STATUS;
	}

	status = fprintf( gnuplot_data->pipe , "\n" );

	CHECK_PLOTGNU_STATUS;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxp_gnuplot_add_array_to_plot_buffer( MX_PLOT *plot,
		long position_type, long num_positions, void *position_array,
		long data_type, long num_data_points, void *data_array )
{
	const char fname[] = "mxp_gnuplot_add_array_to_plot_buffer()";

	MX_SCAN *scan;
	MX_PLOT_GNUPLOT *gnuplot_data;
	long *long_position_array, *long_data_array;
	double *double_position_array, *double_data_array;
	long i;
	int status, saved_errno;

	MX_DEBUG( 2,("%s invoked.", fname));

	/* ---- Get and check a zillion pointers. ---- */

	if ( plot == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PLOT pointer passed was NULL." );
	}

	scan = (MX_SCAN *) (plot->scan);

	if ( scan == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Scan pointer for MX_PLOT pointer = %p was NULL.",
			plot );
	}

	gnuplot_data = (MX_PLOT_GNUPLOT *) (plot->plot_type_struct);

	if ( gnuplot_data == NULL ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"A connection to 'plotgnu' is not currently active.");
	}

	if ( gnuplot_data->pipe == NULL ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"The most recent attempt to connect to 'plotgnu' failed.");
	}

	long_position_array = long_data_array = NULL;
	double_position_array = double_data_array = NULL;

	/* Construct data type specific array pointers. */

	switch( position_type ) {
	case MXFT_LONG:
		long_position_array = (void *) position_array;
		break;
	case MXFT_DOUBLE:
		double_position_array = (void *) position_array;
		break;
	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"Only MXFT_LONG or MXFT_DOUBLE position arrays are supported." );
		break;
	}
	
	switch( data_type ) {
	case MXFT_LONG:
		long_data_array = (void *) data_array;
		break;
	case MXFT_DOUBLE:
		double_data_array = (void *) data_array;
		break;
	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"Only MXFT_LONG or MXFT_DOUBLE data arrays are supported." );
		break;
	}
	
	/* ---- Write the arrays out to plotgnu. ---- */

	status = fprintf( gnuplot_data->pipe, "data" );

	/* Plot the current motor positions (if any). */

	CHECK_PLOTGNU_STATUS;

	if ( scan->num_motors == 0 ) {
		status = fprintf( gnuplot_data->pipe,
				" %3ld", gnuplot_data->plotfile_step_count );

		CHECK_PLOTGNU_STATUS;

		(gnuplot_data->plotfile_step_count)++;
	} else {
		switch( position_type ) {
		case MXFT_LONG:
			for ( i = 0; i < num_positions; i++ ) {
				status = fprintf( gnuplot_data->pipe,
					    " %ld", long_position_array[i] );

				CHECK_PLOTGNU_STATUS;
			}
			break;
		case MXFT_DOUBLE:
			for ( i = 0; i < num_positions; i++ ) {
				status = fprintf( gnuplot_data->pipe,
					    " %g", double_position_array[i] );

				CHECK_PLOTGNU_STATUS;
			}
			break;
		}
	}

	/* Plot the scaler measurements. */

	switch( data_type ) {
	case MXFT_LONG:
		for ( i = 0; i < num_data_points; i++ ) {
			status = fprintf( gnuplot_data->pipe, " %ld",
					long_data_array[i] );

			CHECK_PLOTGNU_STATUS;
		}
		break;
	case MXFT_DOUBLE:
		for ( i = 0; i < num_data_points; i++ ) {
			status = fprintf( gnuplot_data->pipe, " %g",
					double_data_array[i] );

			CHECK_PLOTGNU_STATUS;
		}
		break;
	}

	status = fprintf( gnuplot_data->pipe , "\n" );

	CHECK_PLOTGNU_STATUS;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxp_gnuplot_display_plot( MX_PLOT *plot )
{
	const char fname[] = "mxp_gnuplot_display_plot()";

	MX_PLOT_GNUPLOT *gnuplot_data;
	int status, saved_errno;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( plot == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PLOT pointer passed was NULL." );
	}

	gnuplot_data = (MX_PLOT_GNUPLOT *) (plot->plot_type_struct);

	if ( gnuplot_data == NULL ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"A connection to 'plotgnu' is not currently active.");
	}

	if ( gnuplot_data->pipe == NULL ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"The most recent attempt to connect to 'plotgnu' failed.");
	}

	/* ---- Tell plotgnu to replot the graph. ---- */

	status = fprintf( gnuplot_data->pipe, "plot\n" );

	CHECK_PLOTGNU_STATUS;

	status = fflush( gnuplot_data->pipe );

	CHECK_PLOTGNU_STATUS;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxp_gnuplot_set_x_range( MX_PLOT *plot, double x_min, double x_max )
{
	const char fname[] = "mxp_gnuplot_set_x_range()";

	MX_PLOT_GNUPLOT *gnuplot_data;
	MX_SCAN *scan;
	char buffer[100];

	MX_DEBUG( 2,("%s invoked.", fname));

	gnuplot_data = (MX_PLOT_GNUPLOT *) (plot->plot_type_struct);

	if ( gnuplot_data == NULL ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"A connection to 'gnuplot' is not currently active.");
	}

	if ( gnuplot_data->pipe == NULL ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"The most recent attempt to connect to 'gnuplot' failed.");
	}

	scan = (MX_SCAN *) (plot->scan);

	if ( scan == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Pointer to MX_SCAN structure is NULL.");
	}

	sprintf( buffer, "set xrange [%g:%g]", x_min, x_max );

	if ( fprintf( gnuplot_data->pipe, "%s\n", buffer ) < 0 ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"An attempt to set the x range for 'gnuplot' failed.");
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxp_gnuplot_set_y_range( MX_PLOT *plot, double y_min, double y_max )
{
	const char fname[] = "mxp_gnuplot_set_y_range()";

	MX_PLOT_GNUPLOT *gnuplot_data;
	MX_SCAN *scan;
	char buffer[100];

	MX_DEBUG( 2,("%s invoked.", fname));

	gnuplot_data = (MX_PLOT_GNUPLOT *) (plot->plot_type_struct);

	if ( gnuplot_data == NULL ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"A connection to 'gnuplot' is not currently active.");
	}

	if ( gnuplot_data->pipe == NULL ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"The most recent attempt to connect to 'gnuplot' failed.");
	}

	scan = (MX_SCAN *) (plot->scan);

	if ( scan == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Pointer to MX_SCAN structure is NULL.");
	}

	sprintf( buffer, "set yrange [%g:%g]", y_min, y_max );

	if ( fprintf( gnuplot_data->pipe, "%s\n", buffer ) < 0 ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"An attempt to set the x range for 'gnuplot' failed.");
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxp_gnuplot_start_plot_section( MX_PLOT *plot )
{
	const char fname[] = "mxp_gnuplot_start_plot_section()";

	MX_SCAN *scan;
	MX_PLOT_GNUPLOT *gnuplot_data;
	MX_RECORD **motor_record_array;
	MX_MOTOR *motor;
	MX_RECORD *energy_record;
	MX_MOTOR *energy_motor;
	double motor_position;
	double *motor_position_array;
	long *motor_is_independent_variable_array;
	int status;
	long i, j, innermost_index, num_independent_variables;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	scan = (MX_SCAN *)(plot->scan);

	gnuplot_data = (MX_PLOT_GNUPLOT *) (plot->plot_type_struct);

	if ( gnuplot_data == NULL ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"A connection to 'gnuplot' is not currently active.");
	}

	/*** Find the motor record corresponding to the innermost loop.
	 *** In other words, this is the motor whose position value
	 *** changes most frequently.
	 ***/

	innermost_index = -1;

	if ( scan->plot.num_x_motors == 0 ) {

		/* If no alternate X motors were specified, we use the
		 * primary scan motors.
		 */

		for ( i = 0, j = 0; i < scan->num_motors; i++ ) {
			if ( (scan->motor_is_independent_variable)[i] ) {
				if (j+1 >= scan->num_independent_variables) {

					innermost_index = i;

					break;	/* Exit the for() loop. */
				}
				j++;
			}
		}
	} else {
		/* Otherwise, use the provided alternate X motors. */

		innermost_index = scan->plot.num_x_motors - 1;
	}

	/****** Start new plot for plotgnu. ******/

	if ( scan->plot.num_x_motors == 0 ) {
		num_independent_variables = scan->num_independent_variables;
	} else {
		num_independent_variables = scan->plot.num_x_motors;
	}

	status = fprintf( gnuplot_data->pipe,
			"start_plot;%ld;%ld;%s\n",
			num_independent_variables,
			innermost_index,
			plot->plot_arguments );

	if ( status == EOF ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"An attempt to send plot parameters to 'plotgnu' failed." );
	}

	/****** Set the plot title and xlabel for this section. ******/

	/* Set the plot title. */

	status = fprintf( gnuplot_data->pipe,
			"set title 'Scan = %s  Datafile = ",
				scan->record->name );

	status = fprintf( gnuplot_data->pipe, "%s", scan->datafile.filename );

	if ( status == EOF ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
			"Attempt to set the gnuplot plot title failed." );
	}

	if ( scan->plot.num_x_motors > 0 ) {

		/* If defined, show the alternate X axis motors
		 * except for the last one which is assumed to
		 * be the X axis of the graph.
		 */

		motor_record_array = scan->plot.x_motor_array;

		for ( i = 0; i < innermost_index; i++ ) {
			motor = (MX_MOTOR *)
				    motor_record_array[i]->record_class_struct;

			mx_status = mx_motor_get_position(
						motor_record_array[i],
						&motor_position );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			status = fprintf( gnuplot_data->pipe,
					"  %s = %.*g %s",
					motor_record_array[i]->name,
					motor_record_array[i]->precision,
					motor_position,
					motor->units );

			if ( status < 0 ) {
				return mx_error(
					MXE_IPC_IO_ERROR, fname,
		"Attempt to set gnuplot plot title for motor '%s' failed.",
					motor_record_array[i]->name );
			}
		}
		status = fprintf( gnuplot_data->pipe, "'\n" );

		if ( status == EOF ) {
			return mx_error( MXE_IPC_IO_ERROR, fname,
		    "Attempt to send newline for gnuplot plot title failed." );
		}

	} else if ( scan->num_motors == 0 ) {

		motor_record_array = NULL;

		status = fprintf(gnuplot_data->pipe, "   Scaler scan'\n");

		if ( status == EOF ) {
			return mx_error( MXE_IPC_IO_ERROR, fname,
		"Attempt to set gnuplot plot title for scaler scan failed." );
		}

	} else {
		/* If alternate X axis motors have not been specified,
		 * then show the primary scan motors.
		 */

		motor_position_array = scan->motor_position;
		motor_record_array = scan->motor_record_array;
		motor_is_independent_variable_array
					= scan->motor_is_independent_variable;

		/* Note that the last independent motor is not
		 * shown in the scan title.
		 */

		for ( i = 0; i < scan->num_motors; i++ ) {
		    if ( motor_is_independent_variable_array[i] ) {
			if ( i != innermost_index ) {

			    motor = (MX_MOTOR *)
				motor_record_array[i]->record_class_struct;

			    status = fprintf( gnuplot_data->pipe,
					"  %s = %.*g %s",
					motor_record_array[i]->name,
					motor_record_array[i]->precision,
					motor_position_array[i],
					motor->units );

			    if ( status < 0 ) {
				return mx_error(
					MXE_IPC_IO_ERROR, fname,
		"Attempt to set gnuplot plot title for motor '%s' failed.",
					motor_record_array[i]->name );
			    }
			}
		    }
		}
		status = fprintf( gnuplot_data->pipe, "'\n" );

		if ( status == EOF ) {
			return mx_error( MXE_IPC_IO_ERROR, fname,
		    "Attempt to send newline for gnuplot plot title failed." );
		}
	}

	/* Set the plot xlabel. */

	if ( ( scan->record->mx_class == MXS_XAFS_SCAN )
	  && ( scan->plot.num_x_motors == 0 ) )
	{
		/* For XAFS scans, if no alternate X axis motors have been
		 * specified, then the X axis should be 'energy'.
		 */

		energy_record = mx_get_record( scan->record, "energy" );

		if ( energy_record == (MX_RECORD *) NULL ) {
				return mx_error( MXE_NOT_FOUND, fname,
				"The MX database does not have a record named "
				"'energy', which is required for XAFS scans." );
		}

		energy_motor = (MX_MOTOR *)
				energy_record->record_class_struct;

		status = fprintf( gnuplot_data->pipe,
				"set xlabel '%s (%s)'\n",
				energy_record->name,
				energy_motor->units );
	} else {
		if ( innermost_index >= 0 ) {

			motor = (MX_MOTOR *)
		(motor_record_array[innermost_index]->record_class_struct);

			status = fprintf( gnuplot_data->pipe,
				"set xlabel '%s (%s)'\n",
				motor_record_array[innermost_index]->name,
				motor->units );
		}
	}

	if ( status == EOF ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
				"Attempt to set gnuplot xlabel failed." );
	}

	return MX_SUCCESSFUL_RESULT;
}

