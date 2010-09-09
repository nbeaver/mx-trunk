/*
 * Name:    d_mlfsom.c
 *
 * Purpose: MX area detector simulator that uses the mlfsom program written
 *          by James Holton of Lawrence Berkeley National Laboratory.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2009-2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_MLFSOM_DEBUG		TRUE

#define MXD_MLFSOM_WORK_SCRIPT		"mx_mlfsom.sh"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_osdef.h"

#if defined(OS_UNIX) || defined(OS_CYGWIN)
#include <sys/stat.h>
#endif

#include "mx_util.h"
#include "mx_record.h"
#include "mx_thread.h"
#include "mx_bit.h"
#include "mx_image.h"
#include "mx_motor.h"
#include "mx_area_detector.h"
#include "d_mlfsom.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_mlfsom_record_function_list = {
	mxd_mlfsom_initialize_type,
	mxd_mlfsom_create_record_structures,
	mxd_mlfsom_finish_record_initialization,
	NULL,
	NULL,
	mxd_mlfsom_open,
	mxd_mlfsom_close
};

MX_AREA_DETECTOR_FUNCTION_LIST
mxd_mlfsom_ad_function_list = {
	mxd_mlfsom_arm,
	mxd_mlfsom_trigger,
	mxd_mlfsom_stop,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_mlfsom_get_extended_status,
	mxd_mlfsom_readout_frame,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_mlfsom_get_parameter,
	mxd_mlfsom_set_parameter
};

MX_RECORD_FIELD_DEFAULTS mxd_mlfsom_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
	MXD_MLFSOM_STANDARD_FIELDS
};

long mxd_mlfsom_num_record_fields
		= sizeof( mxd_mlfsom_rf_defaults )
		  / sizeof( mxd_mlfsom_rf_defaults[0]);

MX_RECORD_FIELD_DEFAULTS *mxd_mlfsom_rfield_def_ptr
			= &mxd_mlfsom_rf_defaults[0];

/*---*/

static mx_status_type
mxd_mlfsom_get_pointers( MX_AREA_DETECTOR *ad,
			MX_MLFSOM **mlfsom,
			const char *calling_fname )
{
	static const char fname[] = "mxd_mlfsom_get_pointers()";

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_AREA_DETECTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (mlfsom == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_MLFSOM pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*mlfsom = (MX_MLFSOM *) ad->record->record_type_struct;

	if ( *mlfsom == (MX_MLFSOM *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
  "MX_MLFSOM pointer for record '%s' passed by '%s' is NULL.",
			ad->record->name, calling_fname );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*---*/

static mx_status_type
mxd_mlfsom_mlfsom_monitor_thread( MX_THREAD *thread, void *args )
{
	static const char fname[] = "mxd_mlfsom_mlfsom_monitor_thread()";

	MX_MLFSOM_THREAD_INFO *tinfo;
	unsigned long child_process_id;
	long child_process_status;
	mx_status_type mx_status;

	if ( args == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	tinfo = (MX_MLFSOM_THREAD_INFO *) args;

#if MXD_MLFSOM_DEBUG
	MX_DEBUG(-2,
	  ("vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv"));
	MX_DEBUG(-2,("%s invoked for record '%s'.",
		fname, tinfo->record->name));
#endif

#if MXD_MLFSOM_DEBUG
	MX_DEBUG(-2,("%s: command = '%s'", fname, tinfo->command));
#endif
	mx_status = mx_spawn( tinfo->command, 0, &child_process_id );

	if ( mx_status.code != MXE_SUCCESS ) {
		tinfo->mlfsom->mlfsom_thread = NULL;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Thread is exiting due to an error in mx_spawn()." );
	}

#if MXD_MLFSOM_DEBUG
	MX_DEBUG(-2,("%s: About to wait for process ID %lu",
		fname, child_process_id ));
#endif

	mx_status = mx_wait_for_process_id( child_process_id,
					&child_process_status );

	if ( mx_status.code != MXE_SUCCESS ) {
		tinfo->mlfsom->mlfsom_thread = NULL;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
	    "Thread is exiting due to an error in mx_wait_for_process_id()." );
	}

#if MXD_MLFSOM_DEBUG
	MX_DEBUG(-2,
	("%s is complete for record '%s', child_process_status = %lu.",
		fname, tinfo->record->name, child_process_status));
	MX_DEBUG(-2,
	  ("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"));
#endif
	tinfo->mlfsom->mlfsom_thread = NULL;

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_mlfsom_initialize_type( long record_type )
{
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
	long num_record_fields;
	long maximum_num_rois_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_area_detector_initialize_type( record_type,
					&num_record_fields,
					&record_field_defaults,
					&maximum_num_rois_varargs_cookie );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mlfsom_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_mlfsom_create_record_structures()";

	MX_AREA_DETECTOR *ad;
	MX_MLFSOM *mlfsom = NULL;

	ad = (MX_AREA_DETECTOR *) malloc( sizeof(MX_AREA_DETECTOR) );

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_AREA_DETECTOR structure." );
	}

	mlfsom = (MX_MLFSOM *) malloc( sizeof(MX_MLFSOM) );

	if ( mlfsom == (MX_MLFSOM *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_MLFSOM structure." );
	}

	record->record_class_struct = ad;
	record->record_type_struct = mlfsom;
	record->class_specific_function_list = &mxd_mlfsom_ad_function_list;

	memset( &(ad->sequence_parameters), 0, sizeof(ad->sequence_parameters));

	ad->record = record;
	mlfsom->record = record;

	ad->trigger_mode = 0;
	ad->initial_correction_flags = 0;

	/* Allocate memory for structures to contain memory to be passed
	 * to the monitor threads.
	 */

	mlfsom->ano_sfall_tinfo =
	    (MX_MLFSOM_THREAD_INFO *) malloc( sizeof(MX_MLFSOM_THREAD_INFO) );

	if ( mlfsom->ano_sfall_tinfo == (MX_MLFSOM_THREAD_INFO *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate an "
			"MX_MLFSOM_THREAD_INFO structure for record '%s'.",
				record->name );
	}

	mlfsom->ano_sfall_tinfo->record = record;
	mlfsom->ano_sfall_tinfo->ad = ad;
	mlfsom->ano_sfall_tinfo->mlfsom = mlfsom;

	mlfsom->mlfsom_tinfo =
	    (MX_MLFSOM_THREAD_INFO *) malloc( sizeof(MX_MLFSOM_THREAD_INFO) );

	if ( mlfsom->mlfsom_tinfo == (MX_MLFSOM_THREAD_INFO *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate a second "
			"MX_MLFSOM_THREAD_INFO structure for record '%s'.",
				record->name );
	}

	mlfsom->mlfsom_tinfo->record = record;
	mlfsom->mlfsom_tinfo->ad = ad;
	mlfsom->mlfsom_tinfo->mlfsom = mlfsom;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mlfsom_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] = "mxd_mlfsom_finish_record_initialization()";

	MX_AREA_DETECTOR *ad;
	MX_MLFSOM *mlfsom = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_mlfsom_get_pointers( ad, &mlfsom, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MLFSOM_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif
	mx_status = mx_area_detector_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mlfsom_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_mlfsom_open()";

	MX_AREA_DETECTOR *ad;
	MX_MLFSOM *mlfsom = NULL;
	char *ccp4_master;
	long i;
	unsigned long mask;
	mx_bool_type all_commands_found, cmd_found;
	mx_status_type mx_status;

	static char cmdlist[][20] = {
		"fit2d", "ano_sfall.com", "hkl2XYphi.awk",
		"mlfsom.com", "msdate.com" };

	static int num_cmds = sizeof(cmdlist) / sizeof(cmdlist[0]);

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_mlfsom_get_pointers( ad, &mlfsom, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MLFSOM_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	ad->header_length = 0;

	ad->last_frame_number = -1;
	ad->total_num_frames = 0;
	ad->status = 0;

	ad->datafile_format = MXT_IMAGE_FILE_SMV;

	ad->sequence_parameters.sequence_type = MXT_SQ_ONE_SHOT;
	ad->sequence_parameters.num_parameters = 1;
	ad->sequence_parameters.parameter_array[0] = 1.0;

	/* FIXME: Get the maximum framesize from somewhere else? */

	ad->maximum_framesize[0] = 3072;
	ad->maximum_framesize[1] = 3072;

	/*---*/

	ad->framesize[0] = ad->maximum_framesize[0];
	ad->framesize[1] = ad->maximum_framesize[1];

	ad->binsize[0] = 1;
	ad->binsize[1] = 1;

	ad->image_format = MXT_IMAGE_FORMAT_GREY16;

	mx_status = mx_image_get_image_format_name_from_type(
						ad->image_format,
						ad->image_format_name,
						sizeof(ad->image_format_name) );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->frame_file_format = MXT_IMAGE_FILE_SMV;

	ad->byte_order = mx_native_byteorder();

	ad->bytes_per_pixel = 2;
	ad->bits_per_pixel  = 16;

	ad->bytes_per_frame = 
	  mx_round( ad->bytes_per_pixel * ad->framesize[0] * ad->framesize[1] );

	/* Zero out the ROI boundaries. */

	for ( i = 0; i < ad->maximum_num_rois; i++ ) {
		ad->roi_array[i][0] = 0;
		ad->roi_array[i][1] = 0;
		ad->roi_array[i][2] = 0;
		ad->roi_array[i][3] = 0;
	}

	/* Load the image correction files. */

	mx_status = mx_area_detector_load_correction_files( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Configure automatic saving or loading of image frames. */

	mask = MXF_AD_SAVE_FRAME_AFTER_ACQUISITION
		| MXF_AD_LOAD_FRAME_AFTER_ACQUISITION;

	if ( ad->area_detector_flags & mask ) {
		mx_status =
		    mx_area_detector_setup_datafile_management( record, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Check to see if the CCP4 runtime environment is set up. */

	ccp4_master = getenv( "CCP4_MASTER" );

	if ( ccp4_master == NULL ) {
		return mx_error( MXE_SOFTWARE_CONFIGURATION_ERROR, fname,
		"The environment variable CCP4_MASTER does not exist.  "
		"Perhaps the script '$CCP4_MASTER/setup-scripts/sh/ccp4.setup' "
		"has not yet been run?" );
	}

	/* See if the commands we need are found in the PATH variable. */

	all_commands_found = TRUE;

	for ( i = 0; i < num_cmds; i++ ) {
		cmd_found = mx_command_found( cmdlist[i] );

		if ( cmd_found == FALSE ) {
			mx_warning( "Required command '%s' was not found "
			"in the PATH.", cmdlist[i] );

			all_commands_found = FALSE;
		}
	}

	if ( all_commands_found == FALSE ) {
		return mx_error( MXE_SOFTWARE_CONFIGURATION_ERROR, fname,
		"One or more of the 'fit2d' and 'mlfsom' commands required "
		"by this driver were not found in the path." );
	}

#if MXD_MLFSOM_DEBUG
	MX_DEBUG(-2,("%s complete for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mlfsom_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mlfsom_arm( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_mlfsom_arm()";

	MX_MLFSOM *mlfsom = NULL;
	MX_MLFSOM_THREAD_INFO *tinfo = NULL;
	MX_SEQUENCE_PARAMETERS *sp = NULL;
	double exposure_time, detector_distance, energy, phi;
	int saved_errno, os_status;
	FILE *work_script_file;
	mx_status_type mx_status;

	mx_status = mxd_mlfsom_get_pointers( ad, &mlfsom, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MLFSOM_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif
	/* Get the current energy, phi, and detector distance. */

	mx_status = mx_motor_get_position( mlfsom->energy_record, &energy );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_position( mlfsom->phi_record, &phi );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_position( mlfsom->detector_distance_record,
						&detector_distance );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Construct the filename of the temporary MX work script for 'mlfsom'.
	 *
	 * This temporary script is necessary so that it can change the
	 * current directory of the script to the 'mlfsom' work directory
	 * without changing the current directory of the main MX program.
	 */

	tinfo = mlfsom->mlfsom_tinfo;

	snprintf( tinfo->command, sizeof(tinfo->command),
		"%s/%s", mlfsom->work_directory, MXD_MLFSOM_WORK_SCRIPT );

	/* Create the temporary MX work script. */

#if MXD_MLFSOM_DEBUG
	MX_DEBUG(-2,("%s: Creating MX 'mlfsom' work script '%s'.",
		fname, tinfo->command ));
#endif

	work_script_file = fopen( tinfo->command, "w" );

	if ( work_script_file == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot open the temporary MX work script '%s' used by the "
		"'mlfsom' driver for area detector '%s'.  "
		"Errno = %d, error message = '%s'.",
			tinfo->command, ad->record->name,
			saved_errno, strerror( saved_errno ) );
	}

	/* Write a Unix-style script header. */

	fprintf( work_script_file, "#! /bin/sh\n" );
	fprintf( work_script_file, "set -x\n" );
	fprintf( work_script_file, "#\n" );
	fprintf( work_script_file, "# This file is a temporary script header "
				"file created by the MX 'mlfsom' driver.\n" );
	fprintf( work_script_file, "# It is safe to delete this file.\n" );
	fprintf( work_script_file, "#\n" );
	fprintf( work_script_file, "\n" );

	/* Tell the script to change to the work directory. */

	fprintf( work_script_file, "cd %s\n", mlfsom->work_directory );
	fprintf( work_script_file, "\n" );

	/* Tell the script to symbolically link the 'ideal_ano.mtz' file
	 * generated by ano_sfall.com to 'pristine.mtz'.
	 */

	fprintf( work_script_file, "ln -sf ideal_ano.mtz pristine.mtz\n" );
	fprintf( work_script_file, "\n" );

	/* Construct the command line for the external 'mlfsom' program. */

	if ( mlfsom->mlfsom_filename[0] == '/' ) {
		fprintf( work_script_file, "%s ", mlfsom->mlfsom_filename );
	} else {
		fprintf( work_script_file, "%s/%s ", mlfsom->work_directory,
						mlfsom->mlfsom_filename);
	}

	/* Add the filename of the output SMV file. */

	fprintf( work_script_file, "mx_mlfsom.img " );

	/* Add the 'mlfsom' parameters. */

	sp = &(ad->sequence_parameters);

	exposure_time = sp->parameter_array[0];

	fprintf( work_script_file, "%.1fs %.1fmm %.1feV phi=%.1f ",
		exposure_time, detector_distance, energy, phi );

	/* Add the prefix for the output files. */

	fprintf( work_script_file, "outprefix=./\n" );

	/* Tell the script file to return the status of 'mlfsom.com' as
	 * the script's return status.
	 */

	fprintf( work_script_file, "exit $?\n" );

	/* Close the script file. */

	os_status = fclose( work_script_file );

	if ( os_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot close the temporary MX work script '%s' used by the "
		"'mlfsom' driver for area detector '%s'.  "
		"Errno = %d, error message = '%s'.",
			tinfo->command, ad->record->name,
			saved_errno, strerror( saved_errno ) );
	}

#if defined(OS_UNIX) || defined(OS_CYGWIN)

	/* Make the script executable. */

	os_status = chmod( tinfo->command, 0755 );

	if ( os_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot make executable the temporary MX work script '%s' "
		"used by the 'mlfsom' driver for area detector '%s'.  "
		"Errno = %d, error message = '%s'.",
			tinfo->command, ad->record->name,
			saved_errno, strerror( saved_errno ) );
	}
#endif

#if MXD_MLFSOM_DEBUG
	MX_DEBUG(-2,("%s: command = '%s'", fname, tinfo->command));
#endif
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mlfsom_trigger( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_mlfsom_trigger()";

	MX_MLFSOM *mlfsom = NULL;
	mx_status_type mx_status;

	mx_status = mxd_mlfsom_get_pointers( ad, &mlfsom, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MLFSOM_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif
	ad->trigger = TRUE;

	/* Create a thread to run the 'mlfsom' program from. */

	mx_status = mx_thread_create( &(mlfsom->mlfsom_thread),
				mxd_mlfsom_mlfsom_monitor_thread,
				mlfsom->mlfsom_tinfo );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mlfsom_stop( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_mlfsom_stop()";

	MX_MLFSOM *mlfsom = NULL;
	mx_status_type mx_status;

	mx_status = mxd_mlfsom_get_pointers( ad, &mlfsom, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MLFSOM_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	/* If present, tell the 'mlfsom' thread to stop. */

	if ( mlfsom->mlfsom_thread == NULL )
		return MX_SUCCESSFUL_RESULT;

	mx_status = mx_thread_kill( mlfsom->mlfsom_thread );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mlfsom->mlfsom_thread = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mlfsom_get_extended_status( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
			"mxd_mlfsom_get_extended_status()";

	MX_MLFSOM *mlfsom = NULL;
	mx_status_type mx_status;

	mx_status = mxd_mlfsom_get_pointers( ad, &mlfsom, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0 && MXD_MLFSOM_DEBUG
	MX_DEBUG(-2,("%s: ad->trigger = %d, mlfsom->mlfsom_thread = %p",
		fname, (int) ad->trigger, mlfsom->mlfsom_thread));
#endif

	if ( ad->trigger == FALSE ) {
		/* If we have not been triggered, then report that nothing
		 * is happening.
		 */

		ad->status = 0;
	} else {
		/* We have been triggered. */

		if ( mlfsom->mlfsom_thread == NULL ) {
			/* The monitor thread has exited. */

			ad->status = 0;
			ad->last_frame_number = 0;
			ad->total_num_frames++;
			ad->trigger = FALSE;

#if MXD_MLFSOM_DEBUG
			MX_DEBUG(-2,("%s: Exposure has completed.", fname));
#endif
		} else {
			ad->status = MXSF_AD_ACQUISITION_IN_PROGRESS;
		}
	}

	mx_area_detector_update_extended_status_string( ad );

#if 0 && MXD_MLFSOM_DEBUG
	MX_DEBUG(-2,
	("%s: last_frame_number = %ld, total_num_frames = %ld, status = %#lx",
	    fname, ad->last_frame_number, ad->total_num_frames, ad->status));
#endif
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mlfsom_readout_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_mlfsom_readout_frame()";

	MX_MLFSOM *mlfsom = NULL;
	char smv_filename[MXU_FILENAME_LENGTH+1];
	mx_status_type mx_status;

	mx_status = mxd_mlfsom_get_pointers( ad, &mlfsom, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MLFSOM_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s', frame %ld.",
		fname, ad->record->name, ad->readout_frame ));
#endif

	snprintf( smv_filename, sizeof(smv_filename),
		"%s/mx_000.img", mlfsom->work_directory );

#if MXD_MLFSOM_DEBUG
	MX_DEBUG(-2,("%s: Reading SMV file '%s'.", fname, smv_filename));
#endif

	mx_status = mx_image_read_file( &(ad->image_frame),
					ad->datafile_format,
					smv_filename );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mlfsom_get_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_mlfsom_get_parameter()";

	MX_MLFSOM *mlfsom = NULL;
	mx_status_type mx_status;

	mx_status = mxd_mlfsom_get_pointers( ad, &mlfsom, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MLFSOM_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, ad->record->name, ad->parameter_type));
#endif

	switch( ad->parameter_type ) {
	case MXLV_AD_MAXIMUM_FRAME_NUMBER:
		break;

	case MXLV_AD_MAXIMUM_FRAMESIZE:
		break;

	case MXLV_AD_FRAMESIZE:
		break;

	case MXLV_AD_BINSIZE:
		break;

	case MXLV_AD_CORRECTION_FLAGS:
		break;

	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
		break;

	case MXLV_AD_BYTE_ORDER:
		break;

	case MXLV_AD_TRIGGER_MODE:
		break;

	case MXLV_AD_BYTES_PER_FRAME:
		break;

	case MXLV_AD_BYTES_PER_PIXEL:
		break;

	case MXLV_AD_BITS_PER_PIXEL:
		break;

	case MXLV_AD_DETECTOR_READOUT_TIME:
		break;

	case MXLV_AD_SEQUENCE_START_DELAY:
		break;

	case MXLV_AD_TOTAL_ACQUISITION_TIME:
		break;

	case MXLV_AD_TOTAL_SEQUENCE_TIME:
		break;

	case MXLV_AD_SEQUENCE_TYPE:
		break;

	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY:
		break;

	case MXLV_AD_ROI:
		break;

	case MXLV_AD_SUBFRAME_SIZE:
		break;

	case MXLV_AD_USE_SCALED_DARK_CURRENT:
		break;

	case MXLV_AD_SHUTTER_ENABLE:
		break;

	case MXLV_AD_DATAFILE_DIRECTORY:
		break;

	case MXLV_AD_DATAFILE_FORMAT:
		break;

	case MXLV_AD_DATAFILE_NAME:
		break;

	case MXLV_AD_DATAFILE_PATTERN:
		break;

	case MXLV_AD_LAST_DATAFILE_NAME:
		break;

	default:
		mx_status =
			mx_area_detector_default_get_parameter_handler( ad );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mlfsom_set_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_mlfsom_set_parameter()";

	MX_MLFSOM *mlfsom = NULL;
	mx_status_type mx_status;

	mx_status = mxd_mlfsom_get_pointers( ad, &mlfsom, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MLFSOM_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, ad->record->name, ad->parameter_type));
#endif

	switch( ad->parameter_type ) {
	case MXLV_AD_FRAMESIZE:
		break;

	case MXLV_AD_BINSIZE:
		break;

	case MXLV_AD_CORRECTION_FLAGS:
		break;

	case MXLV_AD_TRIGGER_MODE:
		break;

	case MXLV_AD_SEQUENCE_TYPE:
		break;

	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY:
		break;

	case MXLV_AD_ROI:
		break;

	case MXLV_AD_SUBFRAME_SIZE:
		break;

	case MXLV_AD_USE_SCALED_DARK_CURRENT:
		break;

	case MXLV_AD_GEOM_CORR_AFTER_FLOOD:
		break;

	case MXLV_AD_CORRECTION_FRAME_GEOM_CORR_LAST:
		break;

	case MXLV_AD_CORRECTION_FRAME_NO_GEOM_CORR:
		break;

	case MXLV_AD_SHUTTER_ENABLE:
		break;

	case MXLV_AD_DATAFILE_DIRECTORY:
		break;

	case MXLV_AD_DATAFILE_FORMAT:
		break;

	case MXLV_AD_DATAFILE_NAME:
		break;

	case MXLV_AD_DATAFILE_PATTERN:
		break;

	case MXLV_AD_LAST_DATAFILE_NAME:
		break;

	case MXLV_AD_BYTES_PER_FRAME:
	case MXLV_AD_BYTES_PER_PIXEL:
	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
	case MXLV_AD_BYTE_ORDER:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Changing the parameter '%s' for area detector '%s' "
		"is not supported.", mx_get_field_label_string( ad->record,
			ad->parameter_type ), ad->record->name );
	default:
		mx_status =
			mx_area_detector_default_set_parameter_handler( ad );
		break;
	}

	return mx_status;
}

