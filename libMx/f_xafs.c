/*
 * Name:    f_xafs.c
 *
 * Purpose: Datafile driver for MRCAT XAFS data file.
 *
 *          Note that this is a special purpose data file format and is not
 *          useable for all types of scans.  In particular, motor position
 *          list scans and input device scans are not supported.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999-2006, 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_scan.h"
#include "mx_amplifier.h"
#include "mx_datafile.h"
#include "mx_analog_input.h"
#include "mx_variable.h"
#include "mx_measurement.h"

#include "mx_scan_linear.h"
#include "mx_scan_xafs.h"
#include "mx_scan_quick.h"
#include "f_xafs.h"

MX_DATAFILE_FUNCTION_LIST mxdf_xafs_datafile_function_list = {
	mxdf_xafs_open,
	mxdf_xafs_close,
	mxdf_xafs_write_main_header,
	mxdf_xafs_write_segment_header,
	mxdf_xafs_write_trailer,
	mxdf_xafs_add_measurement_to_datafile,
	mxdf_xafs_add_array_to_datafile
};

#define CHECK_FPRINTF_STATUS \
	if ( status == EOF ) { \
		saved_errno = errno; \
		return mx_error( MXE_FILE_IO_ERROR, fname, \
		"Error writing data to datafile '%s'.  Reason = '%s'", \
			datafile->filename, strerror( saved_errno ) ); \
	}

#define NUM_GAINS_AND_OFFSETS	16

MX_EXPORT mx_status_type
mxdf_xafs_open( MX_DATAFILE *datafile )
{
	static const char fname[] = "mxdf_xafs_open()";

	MX_DATAFILE_XAFS *xafs_file_struct;
	MX_RECORD *energy_motor_record;
	MX_SCAN *scan;
	int saved_errno;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( datafile == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DATAFILE pointer passed was NULL.");
	}

	xafs_file_struct
		= (MX_DATAFILE_XAFS *) malloc( sizeof(MX_DATAFILE_XAFS) );

	if ( xafs_file_struct == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_DATAFILE_XAFS struct for datafile '%s'",
			datafile->filename );
	}

	datafile->datafile_type_struct = xafs_file_struct;

	/* Open the output data file. */

	xafs_file_struct->file = fopen(datafile->filename, "w");

	saved_errno = errno;

	if ( xafs_file_struct->file == NULL ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
			"Cannot open datafile '%s'.  Reason = '%s'",
			datafile->filename, strerror( saved_errno ) );
	}

	if (setvbuf(xafs_file_struct->file, (char *)NULL, _IOLBF, BUFSIZ) != 0)
	{
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot set line buffering on datafile '%s'.  Reason = '%s'",
			datafile->filename, strerror( saved_errno ) );
	}

	/* Find the monochromator energy pseudo motor. */

	scan = (MX_SCAN *)(datafile->scan);

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"No scan record attached to datafile '%s'.",
			datafile->filename);
	}

	energy_motor_record = mx_get_record(scan->record->list_head, "energy");

	if ( energy_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
"Cannot find a motor record named 'energy'.  "
"The 'energy' pseudo motor is required for the MRCAT XAFS file format.");
	}

	if ( energy_motor_record->mx_superclass != MXR_DEVICE
	  || energy_motor_record->mx_class      != MXC_MOTOR ){
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The record named 'energy' is not a motor record." );
	}

	xafs_file_struct->energy_motor_record = energy_motor_record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxdf_xafs_close( MX_DATAFILE *datafile )
{
	static const char fname[] = "mxdf_xafs_close()";

	MX_DATAFILE_XAFS *xafs_file_struct;
	int status, saved_errno;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( datafile == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DATAFILE pointer passed was NULL.");
	}

	xafs_file_struct
		= (MX_DATAFILE_XAFS *)(datafile->datafile_type_struct);

	if ( xafs_file_struct == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_DATAFILE_XAFS pointer for datafile '%s' is NULL.",
			datafile->filename );
	}

	if ( xafs_file_struct->file == NULL ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Datafile '%s' was not open.", datafile->filename );
	}

	status = fclose( xafs_file_struct->file );

	saved_errno = errno;

	xafs_file_struct->file = NULL;

	free( xafs_file_struct );

	datafile->datafile_type_struct = NULL;

	if ( status == EOF ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Attempt to close datafile '%s' failed.  Reason = '%s'",
			datafile->filename, strerror( saved_errno ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxdf_xafs_write_header( MX_DATAFILE *datafile,
			FILE *file,
			mx_bool_type is_mca_file )
{
	static const char fname[] = "mxdf_xafs_write_header()";

	MX_DATAFILE_XAFS *xafs_file_struct;
	MX_SCAN *scan;
	MX_LINEAR_SCAN *linear_scan;
	MX_XAFS_SCAN *xafs_scan;
	MX_QUICK_SCAN *quick_scan;
	MX_RECORD **input_device_array;
	MX_RECORD *input_device_record;
	FILE *output_file;
	long i, num_input_devices;
	long num_regions, num_boundaries;
	long num_energy_regions, num_k_regions;
	double *region_boundary, *region_step_size, *region_measurement_time;
	double estimated_step_size;

	double dark_current;
	int status, saved_errno;
	mx_status_type mx_status;

	char beamline_name[80];
	char header_name[20];
	char xafs_header[80];
	char *label;
	double ring_energy;
	double edge_energy;

	MX_RECORD *amplifier_list_record;
	MX_RECORD *amplifier_record;
	char amplifier_list_record_name[40];
	char **amplifier_list_array;
	char *amp_name;
	double measurement_time, gain_value;
	size_t blank_length;
	MX_RECORD *keithley_gain_record;
	char keithley_gain_record_name[40];
	long num_dimensions, *dimension_array, field_type;
	long num_keithleys, *keithley_gain_array;
	void *pointer_to_value;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( datafile == (MX_DATAFILE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DATAFILE pointer passed was NULL.");
	}

	if ( is_mca_file ) {
		if ( file == (FILE *) NULL ) {
			return mx_error( MXE_NULL_ARGUMENT, fname,
			"The file pointer for an XAFS MCA file was NULL." );
		}

		output_file = file;
	} else {
		xafs_file_struct
			= (MX_DATAFILE_XAFS *)(datafile->datafile_type_struct);

		if ( xafs_file_struct == (MX_DATAFILE_XAFS *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_DATAFILE_XAFS pointer for datafile '%s' is NULL.",
				datafile->filename );
		}

		output_file = xafs_file_struct->file;

		if ( output_file == NULL ) {
			return mx_error( MXE_FILE_IO_ERROR, fname,
			"Datafile '%s' is not currently open.",
				datafile->filename );
		}
	}

	scan = (MX_SCAN *) (datafile->scan);

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The datafile '%s' is not attached to any scan.  scan ptr = NULL.",
			datafile->filename );
	}

	mx_status = mx_get_measurement_time( &(scan->measurement),
						&measurement_time );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*======== Write out the text of the header. ========*/

	status = fprintf( output_file,
			"MRCAT_XAFS V0.3 Datafile\n" );
	CHECK_FPRINTF_STATUS;

	mx_status = mx_get_string_variable_by_name( scan->record->list_head,
		"beamline_name", beamline_name, sizeof(beamline_name) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	status = fprintf( output_file,
			"\"%s\" created at %s on %s\n",
				datafile->filename,
				beamline_name,
				mx_ctime_string() );
	CHECK_FPRINTF_STATUS;
	
	mx_status = mx_get_double_variable_by_name( scan->record->list_head,
				"ring_energy", &ring_energy );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	status = fprintf(output_file, "Ring energy= %.2f GeV\n", ring_energy);
	CHECK_FPRINTF_STATUS;

	mx_status = mx_get_double_variable_by_name( scan->record->list_head,
				"edge_energy", &edge_energy );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	status = fprintf( output_file, "E0= %7.2f\n", edge_energy );

	CHECK_FPRINTF_STATUS;

	linear_scan = NULL;
	xafs_scan = NULL;
	quick_scan = NULL;
	num_regions = 0;
	num_boundaries = 0;
	num_energy_regions = 0;
	num_k_regions = 0;
	region_boundary = NULL;
	region_step_size = NULL;
	region_measurement_time = NULL;
	num_input_devices = 0;
	input_device_array = NULL;

	switch( scan->record->mx_class ) {
	case MXS_LINEAR_SCAN:
		if ( scan->record->mx_type == MXS_LIN_INPUT ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The MRCAT XAFS datafile type may not be used with an input device scan." );
		}

		linear_scan = (MX_LINEAR_SCAN *)
					scan->record->record_class_struct;
		num_regions = 1;
		num_boundaries = 2;
		break;
	case MXS_XAFS_SCAN:
		xafs_scan = (MX_XAFS_SCAN *)
					scan->record->record_class_struct;
		num_regions = xafs_scan->num_regions;
		num_boundaries = xafs_scan->num_boundaries;
		num_energy_regions = xafs_scan->num_energy_regions;
		num_k_regions = xafs_scan->num_k_regions;

		region_boundary = xafs_scan->region_boundary;
		region_step_size = xafs_scan->region_step_size;
		region_measurement_time = xafs_scan->region_measurement_time;
		break;
	case MXS_QUICK_SCAN:
		quick_scan = (MX_QUICK_SCAN *)
					scan->record->record_class_struct;
		num_regions = 1;
		num_boundaries = 2;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The MRCAT XAFS datafile type cannot be used with a scan from scan class = %ld",
			scan->record->mx_class );
	}

	status = fprintf( output_file, "NUM_REGIONS= %ld\n", num_regions);

	CHECK_FPRINTF_STATUS;

	/*=== Start position ===*/

	status = fprintf( output_file,
			"SRB= " );
	CHECK_FPRINTF_STATUS;

	num_input_devices = scan->num_input_devices;

	input_device_array = scan->input_device_array;

	switch( scan->record->mx_class ) {
	case MXS_LINEAR_SCAN:
		status = fprintf( output_file, "%.*g\n",
					scan->record->precision,
					linear_scan->start_position[0] );
		CHECK_FPRINTF_STATUS;
		break;

	case MXS_XAFS_SCAN:
		/* Print the region boundaries. */

		for ( i = 0; i <= num_energy_regions; i++ ) {
			status = fprintf( output_file, "%.*g ",
					scan->record->precision,
					region_boundary[i] );
			CHECK_FPRINTF_STATUS;
		}
		for ( i = num_energy_regions + 1; i < num_boundaries; i++ ) {
			status = fprintf( output_file, "%.*gk ",
					scan->record->precision,
					region_boundary[i] );
			CHECK_FPRINTF_STATUS;
		}
		fprintf( output_file, "\n" );
		CHECK_FPRINTF_STATUS;
		break;

	case MXS_QUICK_SCAN:
		status = fprintf( output_file, "%.*g\n",
					scan->record->precision,
					quick_scan->start_position[0] );
		CHECK_FPRINTF_STATUS;
		break;
	}

	/*=== Step size ===*/

	status = fprintf( output_file, "SRSS= " );
	CHECK_FPRINTF_STATUS;

	switch( scan->record->mx_class ) {
	case MXS_LINEAR_SCAN:
		status = fprintf( output_file, "%.*g\n",
					scan->record->precision,
					linear_scan->step_size[0] );
		CHECK_FPRINTF_STATUS;
		break;
	case MXS_XAFS_SCAN:
		for ( i = 0; i < num_energy_regions; i++ ) {
			status = fprintf( output_file, "%.*g ",
					scan->record->precision,
					region_step_size[i] );
			CHECK_FPRINTF_STATUS;
		}
		for ( i = num_energy_regions; i < num_regions; i++ ) {
			status = fprintf( output_file, "%.*gk ",
					scan->record->precision,
					region_step_size[i] );
			CHECK_FPRINTF_STATUS;
		}
		fprintf( output_file, "\n" );
		CHECK_FPRINTF_STATUS;
		break;
	case MXS_QUICK_SCAN:
		estimated_step_size = mx_divide_safely(
		  quick_scan->end_position[0] - quick_scan->start_position[0],
		    (double) (quick_scan->requested_num_measurements - 1L) );

		status = fprintf( output_file, "%.*g\n",
				scan->record->precision,
				estimated_step_size );

		CHECK_FPRINTF_STATUS;
		break;
	}

	/*=== Integration time ===*/

	status = fprintf( output_file, "SPP= " );
	CHECK_FPRINTF_STATUS;

	switch( scan->record->mx_class ) {
	case MXS_LINEAR_SCAN:
		status = fprintf( output_file, "%.*g\n",
					scan->record->precision,
					measurement_time );
		CHECK_FPRINTF_STATUS;
		break;
	case MXS_XAFS_SCAN:
		for ( i = 0; i < num_regions; i++ ) {
			status = fprintf( output_file, "%.*g ",
					scan->record->precision,
					region_measurement_time[i] );
			CHECK_FPRINTF_STATUS;
		}
		status = fprintf( output_file, "\n" );
		CHECK_FPRINTF_STATUS;
		break;
	case MXS_QUICK_SCAN:
		/* Another estimate. */

		status = fprintf( output_file, "%.*g\n",
			scan->record->precision,
			mx_quick_scan_get_measurement_time(quick_scan) );

		CHECK_FPRINTF_STATUS;
		break;
	}

	/*=== Settling time ===*/

	status = fprintf( output_file, "Settling time= " );
	CHECK_FPRINTF_STATUS;

	switch( scan->record->mx_class ) {
	case MXS_LINEAR_SCAN:
	case MXS_XAFS_SCAN:
	case MXS_QUICK_SCAN:
		status = fprintf( output_file, "%g\n",
					scan->settling_time );
		CHECK_FPRINTF_STATUS;
		break;
	}

	/*=== Scaler offsets ===*/

	status = fprintf( output_file, "\n\nOffsets= " );
	CHECK_FPRINTF_STATUS;

	for ( i = 0; i < num_input_devices; i++ ) {
		input_device_record = input_device_array[i];

		if ( input_device_record->mx_class != MXC_SCALER ) {
			fprintf( output_file, "0.00 " );
		} else {
			mx_status = mx_scaler_get_dark_current(
					input_device_record, &dark_current);

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			fprintf( output_file, "%.2f ", dark_current );
		}
		CHECK_FPRINTF_STATUS;
	}

	for ( i = num_input_devices; i < NUM_GAINS_AND_OFFSETS; i++ ) {
		fprintf( output_file, "0.00 " );
		CHECK_FPRINTF_STATUS;
	}

	status = fprintf( output_file, "\n" );
	CHECK_FPRINTF_STATUS;

	/*=== Keithley settings ===*/

	strcpy( amplifier_list_record_name, "amplifier_list" );

	amplifier_list_record = mx_get_record( scan->record->list_head,
						amplifier_list_record_name );

	if ( amplifier_list_record != (MX_RECORD *) NULL ) {

		/* Use the amplifier list to contact the real amplifiers. */

		if ( amplifier_list_record->mx_superclass != MXR_VARIABLE ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
"The record '%s' is not a variable record.  Instead, it's superclass = %ld",
				amplifier_list_record_name,
				amplifier_list_record->mx_superclass );
		}

		mx_status = mx_get_variable_parameters( amplifier_list_record,
			&num_dimensions, &dimension_array, &field_type,
			&pointer_to_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( field_type != MXFT_STRING ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
"The field '%s.value' has the wrong field type.  It should be of type %s, "
"but is actually of type %s.",
				amplifier_list_record_name,
				mx_get_field_type_string( MXFT_STRING ),
				mx_get_field_type_string( field_type ) );
		}

		if ( num_dimensions != 2 ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
"The field '%s.value' has the wrong number of dimensions.  It should have "
"2 dimensions, but actually has %ld dimensions.", 
				amplifier_list_record_name, num_dimensions );
		}

		num_keithleys = dimension_array[0];

		if ( num_keithleys > num_input_devices ) {
			num_keithleys = num_input_devices;
		}

		amplifier_list_array = (char **) pointer_to_value;

		status = fprintf( output_file,
				"Gains=" );
		CHECK_FPRINTF_STATUS;

		for ( i = 0; i < num_keithleys; i++ ) {
			amp_name = amplifier_list_array[i];

			blank_length = strspn( amp_name, " \t" );

			if ( blank_length >= strlen(amp_name) ) {
				status = fprintf(output_file, " 1.00" );
			} else {
				amplifier_record = mx_get_record(
						scan->record,
						amplifier_list_array[i] );

				if ( amplifier_record == NULL ) {
					return mx_error(
						MXE_ILLEGAL_ARGUMENT, fname,
				"The amplifier record '%s' mentioned in the "
				"variable 'amplifier_list' does not exist.",
					amplifier_list_array[i] );
				}
				mx_status = mx_amplifier_get_gain(
						amplifier_record,
						&gain_value );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				status = fprintf(output_file, " %ld.00",
					mx_round( log10( gain_value ) ) );
			}
			CHECK_FPRINTF_STATUS;
		}

		for ( i = num_keithleys; i < NUM_GAINS_AND_OFFSETS; i++ ) {
			status = fprintf( output_file, " 1.00" );
			CHECK_FPRINTF_STATUS;
		}

		status = fprintf( output_file,
				"\n" );
		CHECK_FPRINTF_STATUS;
	} else {

		/* Use the old 'keithley_gains' record to get a static list. */

		strcpy( keithley_gain_record_name, "keithley_gains" );

		keithley_gain_record = mx_get_record( scan->record->list_head,
						keithley_gain_record_name );

		if ( keithley_gain_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_NOT_FOUND, fname,
				"The record '%s' does not exist.", 
				keithley_gain_record_name );
		}

		if ( keithley_gain_record->mx_superclass != MXR_VARIABLE ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
"The record '%s' is not a variable record.  Instead, it's superclass = %ld",
				keithley_gain_record_name,
				keithley_gain_record->mx_superclass );
		}

		mx_status = mx_get_variable_parameters( keithley_gain_record,
			&num_dimensions, &dimension_array, &field_type,
			&pointer_to_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( field_type != MXFT_LONG ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
"The field '%s.value' has the wrong field type.  It should be of type %s, "
"but is actually of type %s.",
				keithley_gain_record_name,
				mx_get_field_type_string( MXFT_LONG ),
				mx_get_field_type_string( field_type ) );
		}

		if ( num_dimensions != 1 ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
"The field '%s.value' has the wrong number of dimensions.  It should have "
"1 dimension, but actually has %ld dimensions.", 
				keithley_gain_record_name, num_dimensions );
		}

		num_keithleys = dimension_array[0];

#if 0
		if ( num_keithleys < num_input_devices ) {
			mx_warning( "The number of Keithley gains = %ld "
			 "is less than the number of input devices = %ld.",
				num_keithleys, num_input_devices );
		}
#endif

		if ( num_keithleys > num_input_devices ) {
			num_keithleys = num_input_devices;
		}

		keithley_gain_array = (long *) pointer_to_value;

		status = fprintf( output_file,
				"Gains=" );
		CHECK_FPRINTF_STATUS;

		for ( i = 0; i < num_keithleys; i++ ) {
			status = fprintf(output_file, " %ld.00",
						keithley_gain_array[i]);
			CHECK_FPRINTF_STATUS;
		}

		for ( i = num_keithleys; i < NUM_GAINS_AND_OFFSETS; i++ ) {
			status = fprintf( output_file, " 1.00" );
			CHECK_FPRINTF_STATUS;
		}

		status = fprintf( output_file,
				"\n" );
		CHECK_FPRINTF_STATUS;
	}

	/*=== Scan header description ===*/

	for ( i = 1; i <= 3; i++ ) {

		sprintf( header_name, "xafs_header%ld", i );

		mx_status = mx_get_string_variable_by_name(
			scan->record->list_head,
			header_name, xafs_header, sizeof(xafs_header) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		status = fprintf( output_file, "%s\n", xafs_header );

		CHECK_FPRINTF_STATUS;
	}

	status = fprintf( output_file,
		"------------------------------------------------"
		"------------------------------------------------\n" );
	CHECK_FPRINTF_STATUS;

	if ( is_mca_file ) {
		for ( i = 0; i < num_input_devices; i++ ) {
			if ( input_device_array[i]->mx_class
					== MXC_MULTICHANNEL_ANALYZER ) 
			{
				label = input_device_array[i]->label;

				if ( strlen(label) > 0 ) {
					status = fprintf( output_file,
						"%10s ", label );
				} else {
					status = fprintf( output_file,
					 "%10s ", input_device_array[i]->name );
				}
			}
		}
	} else {
		/* Print out the line of headers that identifies each column
		 * in the data file.
		 */

		status = fprintf( output_file, " %8s ", "energy" );

		CHECK_FPRINTF_STATUS;

		for ( i = 0; i < num_input_devices; i++ ) {
			label = input_device_array[i]->label;

			if ( strlen(label) > 0 ) {
				status = fprintf( output_file,
					"%8s ", label );
			} else {
				status = fprintf( output_file,
					"%8s ", input_device_array[i]->name );
			}
			CHECK_FPRINTF_STATUS;
		}
	}

	status = fprintf( output_file,
			"\n" );
	CHECK_FPRINTF_STATUS;

	/* We are done, so make sure the header text has been flushed out
	 * to the file.
	 */

	status = fflush( output_file );

	if ( status == EOF ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Error writing data to datafile '%s'.  Reason = '%s'",
			datafile->filename, strerror( saved_errno ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxdf_xafs_write_main_header( MX_DATAFILE *datafile )
{
	return mxdf_xafs_write_header( datafile, NULL, FALSE );
}

MX_EXPORT mx_status_type
mxdf_xafs_write_segment_header( MX_DATAFILE *datafile )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxdf_xafs_write_trailer( MX_DATAFILE *datafile )
{
	return MX_SUCCESSFUL_RESULT;
}

/* At present, MXS_LINEAR_SCAN and MXS_XAFS_SCAN scans use the function
 * mxdf_xafs_add_measurement_to_datafile(), while MXS_QUICK_SCAN scans use
 * mxdf_xafs_add_array_to_datafile().
 */

MX_EXPORT mx_status_type
mxdf_xafs_add_measurement_to_datafile( MX_DATAFILE *datafile )
{
	static const char fname[] = "mxdf_xafs_add_measurement_to_datafile()";

	MX_DATAFILE_XAFS *xafs_file_struct;
	MX_RECORD **input_device_array;
	MX_RECORD *input_device;
	MX_SCALER *scaler;
	MX_ANALOG_INPUT *analog_input;
	MX_SCAN *scan;
	MX_RECORD *energy_motor_record;
	MX_MOTOR *motor;
	FILE *output_file;
	double monochromator_energy;
	double scaler_counts_per_second;
	double analog_input_value;
	double measurement_time;
	char buffer[80];
	long i, num_mcas;
	int status, saved_errno;
	mx_bool_type early_move_flag;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( datafile == (MX_DATAFILE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DATAFILE pointer passed was NULL.");
	}

	xafs_file_struct
		= (MX_DATAFILE_XAFS *)(datafile->datafile_type_struct);

	if ( xafs_file_struct == (MX_DATAFILE_XAFS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_DATAFILE_XAFS pointer for datafile '%s' is NULL.",
			datafile->filename );
	}

	output_file = xafs_file_struct->file;

	if ( output_file == NULL ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Datafile '%s' is not currently open.",
			datafile->filename );
	}

	scan = (MX_SCAN *) (datafile->scan);

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The datafile '%s' is not attached to any scan.  scan ptr = NULL.",
			datafile->filename );
	}

	mx_status = mx_scan_get_early_move_flag( scan, &early_move_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get_measurement_time( &(scan->measurement),
						&measurement_time );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	input_device_array = (MX_RECORD **)(scan->input_device_array);

	if ( input_device_array == (MX_RECORD **) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The scan '%s' has a NULL input device array.",
			scan->record->name );
	}

	/* Print out the current energy. */

	energy_motor_record = xafs_file_struct->energy_motor_record;

	if ( early_move_flag ) {
		motor = (MX_MOTOR *) energy_motor_record->record_class_struct;

		monochromator_energy = motor->old_destination;
	} else {
		mx_status = mx_motor_get_position( energy_motor_record,
							&monochromator_energy );
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	status = fprintf( output_file, " %-10.*g",
			xafs_file_struct->energy_motor_record->precision,
			monochromator_energy );

	CHECK_FPRINTF_STATUS;

	/* Print out the input device measurements. */

	num_mcas = 0;

	mx_status = MX_SUCCESSFUL_RESULT;

	for ( i = 0; i < scan->num_input_devices; i++ ) {
		input_device = input_device_array[i];

		switch( input_device->mx_class ) {
		case MXC_MULTICHANNEL_ANALYZER:

			num_mcas++;
			break;

		case MXC_AREA_DETECTOR:
			mx_status = mx_scan_save_area_detector_image(
						scan, input_device );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
			break;

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

			status = fprintf(output_file, " %.*g",
						scan->record->precision,
						scaler_counts_per_second);
			break;

		case MXC_ANALOG_INPUT:
			analog_input = (MX_ANALOG_INPUT *)
					input_device->record_class_struct;

			analog_input_value = analog_input->value;

			status = fprintf(output_file, " %.*g",
						scan->record->precision,
						analog_input_value);
			break;

		default:
			mx_status =
			    mx_convert_normalized_device_value_to_string(
				input_device, -1, buffer, sizeof(buffer)-1 );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			status = fprintf(output_file, " %s", buffer);
			break;
		}
		CHECK_FPRINTF_STATUS;
	}

	status = fprintf( output_file, "\n" );

	CHECK_FPRINTF_STATUS;

	status = fflush( output_file );

	CHECK_FPRINTF_STATUS;

	if ( num_mcas == 0 ) {
		return MX_SUCCESSFUL_RESULT;
	} else {
		mx_status = mx_scan_save_mca_measurements( scan, num_mcas );

		return mx_status;
	}
}

MX_EXPORT mx_status_type
mxdf_xafs_add_array_to_datafile( MX_DATAFILE *datafile,
		long position_type, long num_positions, void *position_array,
		long data_type, long num_data_points, void *data_array )
{
	static const char fname[] = "mxdf_xafs_add_array_to_datafile()";

	MX_DATAFILE_XAFS *xafs_file_struct;
	MX_SCAN *scan;
	FILE *output_file;
	long *long_position_array, *long_data_array;
	double *double_position_array, *double_data_array;
	double scaler_counts_per_second;
	double measurement_time;
	long i;
	int status, saved_errno;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( datafile == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DATAFILE pointer passed was NULL.");
	}

	xafs_file_struct
		= (MX_DATAFILE_XAFS *)(datafile->datafile_type_struct);

	if ( xafs_file_struct == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_DATAFILE_XAFS pointer for datafile '%s' is NULL.",
			datafile->filename );
	}

	output_file = xafs_file_struct->file;

	if ( output_file == NULL ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Datafile '%s' is not currently open.",
			datafile->filename );
	}

	scan = (MX_SCAN *) (datafile->scan);

	if ( scan == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The datafile '%s' is not attached to any scan.  scan ptr = NULL.",
			datafile->filename );
	}

	mx_status = mx_get_measurement_time( &(scan->measurement),
						&measurement_time );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

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
	}

	/* MXFT_DOUBLE data arrays are not supported since the inputs
	 * are expected to all be scalers.
	 */
	
	switch( data_type ) {
	case MXFT_LONG:
		long_data_array = (void *) data_array;
		break;
	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"Only MXFT_LONG data arrays are supported." );
	}
	
	/* Print out the current motor positions (if any). */

	switch( position_type ) {
	case MXFT_LONG:
		for ( i = 0; i < num_positions; i++ ) {
			status = fprintf( output_file, " %-10ld",
				long_position_array[i] );

			CHECK_FPRINTF_STATUS;
		}
		break;
	case MXFT_DOUBLE:
		for ( i = 0; i < num_positions; i++ ) {
			status = fprintf( output_file, " %-10.*g",
				scan->record->precision,
				double_position_array[i] );

			CHECK_FPRINTF_STATUS;
		}
		break;
	}

	/* Print out the scaler measurements. */

	for ( i = 0; i < num_data_points; i++ ) {
		scaler_counts_per_second = ( (double) long_data_array[i] )
				/ measurement_time;

		status = fprintf( output_file, " %.*g",
					scan->record->precision,
					scaler_counts_per_second );

		CHECK_FPRINTF_STATUS;
	}

	status = fprintf( output_file, "\n" );

	CHECK_FPRINTF_STATUS;

	status = fflush( output_file );

	CHECK_FPRINTF_STATUS;	/* The same logic works for fflush(). */

	return MX_SUCCESSFUL_RESULT;
}

