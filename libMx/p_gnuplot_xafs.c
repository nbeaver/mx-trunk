/*
 * Name:    p_gnuplot_xafs.c
 *
 * Purpose: "gnuplot" plotting support for XAFS scans.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_plot.h"
#include "mx_measurement.h"
#include "mx_analog_input.h"

#include "mx_scan.h"
#include "mx_scan_linear.h"
#include "p_gnuplot_xafs.h"

#ifdef OS_WIN32
#define popen _popen
#define pclose _pclose
#endif

MX_PLOT_FUNCTION_LIST mxp_gnuxafs_function_list = {
		mxp_gnuxafs_open,
		mxp_gnuxafs_close,
		mxp_gnuxafs_add_measurement_to_plot_buffer,
		mxp_gnuxafs_add_array_to_plot_buffer,
		mxp_gnuxafs_display_plot,
		mxp_gnuxafs_set_x_range,
		mxp_gnuxafs_set_y_range,
		mxp_gnuxafs_start_plot_section,
};

#define CHECK_PLOTGNU_STATUS \
	if ( status == EOF ) { \
		saved_errno = errno; \
		return mx_error( MXE_IPC_IO_ERROR, fname, \
		"Error writing data to 'plotgnu'.  Reason = '%s'", \
			strerror( saved_errno ) ); \
	}

MX_EXPORT mx_status_type
mxp_gnuxafs_open( MX_PLOT *plot )
{
	const char fname[] = "mxp_gnuxafs_open()";

	MX_SCAN *scan;
	MX_PLOT_GNUXAFS *gnuxafs_data;
	MX_RECORD *energy_motor_record;
	MX_MOTOR *motor;
	int status;

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

	/* Find a pointer to the energy motor. */

	energy_motor_record = mx_get_record( scan->record, "energy" );

	if ( energy_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"There is no record named 'energy' in this database." );
	}

	if ( energy_motor_record->mx_type != MXT_MTR_ENERGY ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The record named 'energy' is not of type 'energy_motor'." );
	}

	/* Set up an MX_PLOT_GNUXAFS structure to save plot type specific
	 * data into.
	 */

	gnuxafs_data = (MX_PLOT_GNUXAFS *) malloc( sizeof(MX_PLOT_GNUXAFS) );

	if ( gnuxafs_data == (MX_PLOT_GNUXAFS *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Out of memory trying to allocate gnuplot data structure.");
	}

	plot->plot_type_struct = gnuxafs_data;

	gnuxafs_data->energy_motor_record = energy_motor_record;

	/* Try to open the pipe to 'plotgnu' */

#if defined(OS_VXWORKS)
	return mx_error( MXE_UNSUPPORTED, fname,
	  "Plotting with Gnuplot is not supported for this operating system." );
#else
	gnuxafs_data->pipe = popen( "perl -S -- plotgnu.pl", "w" );
#endif

	if ( gnuxafs_data->pipe == NULL ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
			"Unable to start the 'plotgnu' program." );
	}

	/*** Set stream buffering to line buffered. ***/

	setvbuf( gnuxafs_data->pipe, (char *)NULL, _IOLBF, BUFSIZ);

	/* Set parameters for plotgnu. */

	status = fprintf( gnuxafs_data->pipe,
			"start_plot;%ld;%ld;%s\n",
			1L, 0L, plot->plot_arguments );

	if ( status < 0 ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"An attempt to send plot parameters to 'plotgnu' failed." );
	}

	/* Set the plot title and xlabel. */

	status = fprintf( gnuxafs_data->pipe,
			"set title 'Scan = %s  Datafile = %s'\n",
			scan->record->name, scan->datafile.filename );

	if ( status < 0 ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"Attempt to set gnuplot plot title failed." );
	}

	/* Set the plot xlabel. */

	motor = (MX_MOTOR *)
		(gnuxafs_data->energy_motor_record->record_class_struct);

	status = fprintf( gnuxafs_data->pipe,
			"set xlabel '%s (%s)'\n",
			gnuxafs_data->energy_motor_record->name,
			motor->units );

	if ( status < 0 ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"Attempt to set gnuplot xlabel failed." );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxp_gnuxafs_close( MX_PLOT *plot )
{
	const char fname[] = "mxp_gnuxafs_close()";

	MX_PLOT_GNUXAFS *gnuxafs_data;
	int status;

	MX_DEBUG( 2,("%s invoked.", fname));

	gnuxafs_data = (MX_PLOT_GNUXAFS *) (plot->plot_type_struct);

	if ( gnuxafs_data == NULL ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"A connection to 'gnuplot' is not currently active.");
	}

	if ( gnuxafs_data->pipe == NULL ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"The most recent attempt to connect to 'gnuplot' failed.");
	}

	if ( fprintf( gnuxafs_data->pipe, "exit\n" ) < 0 ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"An attempt to send the 'exit' command to 'gnuplot' failed." );
	}

#if defined(OS_VXWORKS)
	status = EOF;
#else
	status = pclose( gnuxafs_data->pipe );
#endif

	if ( status == EOF ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
"An error occurred while trying to close the connection to 'gnuplot'");
	}

	gnuxafs_data->pipe = NULL;
	gnuxafs_data->energy_motor_record = NULL;

	free( gnuxafs_data );

	plot->plot_type_struct = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxp_gnuxafs_add_measurement_to_plot_buffer( MX_PLOT *plot )
{
	const char fname[] = "mxp_gnuxafs_add_measurement_to_plot_buffer()";

	MX_SCAN *scan;
	MX_PLOT_GNUXAFS *gnuxafs_data;
	MX_RECORD **input_device_array;
	MX_RECORD *input_device;
	MX_SCALER *scaler;
	MX_ANALOG_INPUT *analog_input;
	double *motor_position;
	double scaler_counts_per_second;
	double analog_input_value;
	double monochromator_energy;
	double measurement_time;
	char buffer[80];
	long i;
	int status, saved_errno;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	/* ---- Get and check a zillion pointers. ---- */

	if ( plot == (MX_PLOT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PLOT pointer passed was NULL." );
	}

	scan = (MX_SCAN *) (plot->scan);

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Scan pointer for MX_PLOT pointer = %p was NULL.",
			plot );
	}

	mx_status = mx_get_measurement_time( &(scan->measurement),
						&measurement_time );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scan->num_motors > 0 ) {
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

	gnuxafs_data = (MX_PLOT_GNUXAFS *) (plot->plot_type_struct);

	if ( gnuxafs_data == NULL ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"A connection to 'gnuplot' is not currently active.");
	}

	if ( gnuxafs_data->pipe == NULL ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"The most recent attempt to connect to 'gnuplot' failed.");
	}

	/* ---- Write the most recent measurement out to the gnuplot ---- */
	/* ---- temporary data file. ------------------------------------ */

	/* For XAFS scans, always plot 'energy' as the independent variable. */

	mx_status = mx_motor_get_position( gnuxafs_data->energy_motor_record,
						&monochromator_energy );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	status = fprintf(gnuxafs_data->pipe, "data %g",monochromator_energy);

	CHECK_PLOTGNU_STATUS;

	for ( i = 0; i < scan->num_input_devices; i++ ) {
		input_device = input_device_array[i];

		switch ( input_device->mx_class ) {
		case MXC_SCALER:
			/* Scaler readings are proportional to the
			 * integration time, so we must normalize
			 * them to counts per second.
			 */
			scaler = (MX_SCALER *)
					input_device->record_class_struct;

			scaler_counts_per_second = mx_divide_safely(
					(double) scaler->value,
					measurement_time );

			status = fprintf(gnuxafs_data->pipe,
					" %g", scaler_counts_per_second);
			break;

		case MXC_ANALOG_INPUT:
			analog_input = (MX_ANALOG_INPUT *)
					input_device->record_class_struct;

			analog_input_value = analog_input->value;

			status = fprintf(gnuxafs_data->pipe,
					" %g", analog_input_value);

			break;

		default:
			mx_status =
			    mx_convert_normalized_device_value_to_string(
				input_device, -1, buffer, sizeof(buffer)-1 );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			status = fprintf(gnuxafs_data->pipe, " %s", buffer);

			break;
		}
		CHECK_PLOTGNU_STATUS;
	}
	status = fprintf(gnuxafs_data->pipe, "\n");

	CHECK_PLOTGNU_STATUS;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxp_gnuxafs_add_array_to_plot_buffer( MX_PLOT *plot,
		long position_type, long num_positions, void *position_array,
		long data_type, long num_data_points, void *data_array )
{
	const char fname[] = "mxp_gnuxafs_add_array_to_plot_buffer()";

	MX_SCAN *scan;
	MX_PLOT_GNUXAFS *gnuxafs_data;
	long *long_data_array;
	double *double_position_array;
	double scaler_counts_per_second;
	double measurement_time;
	long i;
	int status, saved_errno;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	/* ---- Get and check a zillion pointers. ---- */

	if ( plot == (MX_PLOT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PLOT pointer passed was NULL." );
	}

	scan = (MX_SCAN *) (plot->scan);

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Scan pointer for MX_PLOT pointer = %p was NULL.",
			plot );
	}

	mx_status = mx_get_measurement_time( &(scan->measurement),
						&measurement_time );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	gnuxafs_data = (MX_PLOT_GNUXAFS *) (plot->plot_type_struct);

	if ( gnuxafs_data == NULL ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"A connection to 'gnuplot' is not currently active.");
	}

	if ( gnuxafs_data->pipe == NULL ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"The most recent attempt to connect to 'gnuplot' failed.");
	}

	long_data_array = NULL;
	double_position_array = NULL;

	/* Construct data type specific array pointers. */

	switch( position_type ) {
	case MXFT_DOUBLE:
		double_position_array = (void *) position_array;
		break;
	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
			"Only MXFT_DOUBLE position arrays are supported." );
	}
	
	switch( data_type ) {
	case MXFT_LONG:
		long_data_array = (void *) data_array;
		break;
	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
			"Only MXFT_LONG data arrays are supported." );
	}
	
	/* ---- Write the most recent measurement out to the gnuplot ---- */
	/* ---- temporary data file. ------------------------------------ */

	/* For XAFS scans, the position variable should be energy. */

	status = fprintf( gnuxafs_data->pipe, "data %g",
					double_position_array[0] );

	CHECK_PLOTGNU_STATUS;

	for ( i = 0; i < scan->num_input_devices; i++ ) {

		scaler_counts_per_second = ( (double) long_data_array[i] )
					/ measurement_time;

		status = fprintf(gnuxafs_data->pipe, " %g",
					scaler_counts_per_second);

		CHECK_PLOTGNU_STATUS;
	}
	status = fprintf(gnuxafs_data->pipe, "\n");

	CHECK_PLOTGNU_STATUS;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxp_gnuxafs_display_plot( MX_PLOT *plot )
{
	const char fname[] = "mxp_gnuxafs_display_plot()";

	MX_PLOT_GNUXAFS *gnuxafs_data;
	int status, saved_errno;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( plot == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_PLOT pointer passed was NULL." );
	}

	gnuxafs_data = (MX_PLOT_GNUXAFS *) (plot->plot_type_struct);

	if ( gnuxafs_data == NULL ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"A connection to 'plotgnu' is not currently active.");
	}

	if ( gnuxafs_data->pipe == NULL ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"The most recent attempt to connect to 'plotgnu' failed.");
	}

	/* ---- Tell plotgnu to replot the graph. ---- */

	status = fprintf( gnuxafs_data->pipe, "plot\n" );

	CHECK_PLOTGNU_STATUS;

	status = fflush( gnuxafs_data->pipe );

	CHECK_PLOTGNU_STATUS;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxp_gnuxafs_set_x_range( MX_PLOT *plot, double x_min, double x_max )
{
	const char fname[] = "mxp_gnuxafs_set_x_range()";

	MX_PLOT_GNUXAFS *gnuxafs_data;
	MX_SCAN *scan;
	char buffer[100];

	MX_DEBUG( 2,("%s invoked.", fname));

	gnuxafs_data = (MX_PLOT_GNUXAFS *) (plot->plot_type_struct);

	if ( gnuxafs_data == NULL ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"A connection to 'gnuplot' is not currently active.");
	}

	if ( gnuxafs_data->pipe == NULL ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"The most recent attempt to connect to 'gnuplot' failed.");
	}

	scan = (MX_SCAN *) (plot->scan);

	if ( scan == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Pointer to MX_SCAN structure is NULL.");
	}

	sprintf( buffer, "set xrange [%g:%g]", x_min, x_max );

	if ( fprintf( gnuxafs_data->pipe, "%s\n", buffer ) < 0 ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"An attempt to set the x range for 'gnuplot' failed.");
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxp_gnuxafs_set_y_range( MX_PLOT *plot, double y_min, double y_max )
{
	const char fname[] = "mxp_gnuxafs_set_y_range()";

	MX_PLOT_GNUXAFS *gnuxafs_data;
	MX_SCAN *scan;
	char buffer[100];

	MX_DEBUG( 2,("%s invoked.", fname));

	gnuxafs_data = (MX_PLOT_GNUXAFS *) (plot->plot_type_struct);

	if ( gnuxafs_data == NULL ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"A connection to 'gnuplot' is not currently active.");
	}

	if ( gnuxafs_data->pipe == NULL ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"The most recent attempt to connect to 'gnuplot' failed.");
	}

	scan = (MX_SCAN *) (plot->scan);

	if ( scan == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Pointer to MX_SCAN structure is NULL.");
	}

	sprintf( buffer, "set yrange [%g:%g]", y_min, y_max );

	if ( fprintf( gnuxafs_data->pipe, "%s\n", buffer ) < 0 ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"An attempt to set the x range for 'gnuplot' failed.");
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxp_gnuxafs_start_plot_section( MX_PLOT *plot )
{
	/* For XAFS scans, all the regions are to be plotted on the
	 * same graph, which makes this function a no-op.
	 */

	return MX_SUCCESSFUL_RESULT;
}

