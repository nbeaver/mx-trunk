/*
 * Name:    i_handel.c
 *
 * Purpose: MX driver for the X-Ray Instrumentation Associates Handel library
 *          used by the DXP series of multichannel analyzer.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003-2006, 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_HANDEL_DEBUG	TRUE

#define MXI_HANDEL_DEBUG_TIMING	FALSE

#include <stdio.h>
#include <limits.h>

#include "mxconfig.h"

#if HAVE_XIA_HANDEL

#include <stdlib.h>
#include <errno.h>

#if defined(OS_WIN32)
#include <direct.h>
#endif

#include <xia_version.h>
#include <handel.h>
#include <handel_errors.h>
#include <handel_generic.h>
#include <md_generic.h>

#include "mx_constants.h"
#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_stdint.h"
#include "mx_array.h"
#include "mx_unistd.h"
#include "mx_mca.h"
#include "i_handel.h"
#include "d_handel_mca.h"
#include "u_xia_dxp.h"

#if MXI_HANDEL_DEBUG_TIMING
#  include "mx_hrt_debug.h"
#endif

MX_RECORD_FUNCTION_LIST mxi_handel_record_function_list = {
	mxi_handel_initialize_type,
	mxi_handel_create_record_structures,
	mxi_handel_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_handel_open,
	NULL,
	NULL,
	mxi_handel_resynchronize,
	mxi_handel_special_processing_setup
};

MX_RECORD_FIELD_DEFAULTS mxi_handel_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_HANDEL_STANDARD_FIELDS
};

long mxi_handel_num_record_fields
		= sizeof( mxi_handel_record_field_defaults )
			/ sizeof( mxi_handel_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_handel_rfield_def_ptr
			= &mxi_handel_record_field_defaults[0];

static mx_status_type mxi_handel_process_function( void *record_ptr,
						void *record_field_ptr,
						int operation );

extern const char *mxi_handel_strerror( int xia_status );

static mx_status_type
mxi_handel_get_pointers( MX_MCA *mca,
			MX_HANDEL_MCA **handel_mca,
			MX_HANDEL **handel,
			const char *calling_fname )
{
	static const char fname[] = "mxi_handel_get_pointers()";

	MX_RECORD *handel_record;

	if ( mca == (MX_MCA *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MCA pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( handel_mca == (MX_HANDEL_MCA **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_HANDEL_MCA pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( handel == (MX_HANDEL **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_HANDEL pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( mca->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_RECORD pointer for the MX_MCA pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*handel_mca = (MX_HANDEL_MCA *) mca->record->record_type_struct;

	handel_record = (*handel_mca)->handel_record;

	if ( handel_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The handel_record pointer for MCA record '%s' is NULL.",
			mca->record->name );
	}

	*handel = (MX_HANDEL *) handel_record->record_type_struct;

	if ( (*handel) == (MX_HANDEL *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_HANDEL pointer for XIA DXP record '%s' is NULL.",
			handel_record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*==========================*/

MX_EXPORT mx_status_type
mxi_handel_initialize_type( long record_type )
{
	static const char fname[] = "mxi_handel_initialize_type()";

	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
	long num_active_detector_channels_varargs_cookie;
	MX_DRIVER *driver;
	MX_RECORD_FIELD_DEFAULTS *field;
	long referenced_field_index;
	mx_status_type mx_status;

	driver = mx_get_driver_by_type( record_type );

	if ( driver == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Record type %ld not found.",
			record_type );
	}

	record_field_defaults = *(driver->record_field_defaults_ptr);

	if ( record_field_defaults == (MX_RECORD_FIELD_DEFAULTS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'record_field_defaults' for record type '%s' is NULL.",
			driver->name );
	}

	if ( driver->num_record_fields == (long *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'num_record_fields' pointer for record type '%s' is NULL.",
			driver->name );
	}

	/* Get varargs cookie for 'num_active_detector_channels'. */

	mx_status = mx_find_record_field_defaults_index(
			record_field_defaults,
			*(driver->num_record_fields),
			"num_active_detector_channels",
			&referenced_field_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_construct_varargs_cookie( referenced_field_index, 0,
				&num_active_detector_channels_varargs_cookie );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set varargs cookie for 'active_detector_channel_array'. */

	mx_status = mx_find_record_field_defaults( record_field_defaults,
			*(driver->num_record_fields),
			"active_detector_channel_array",
			&field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = num_active_detector_channels_varargs_cookie;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_handel_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_handel_create_record_structures()";

	MX_HANDEL *handel;

	/* Allocate memory for the necessary structures. */

	handel = (MX_HANDEL *) malloc( sizeof(MX_HANDEL) );

	if ( handel == (MX_HANDEL *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Can't allocate memory for MX_HANDEL structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;;
	record->record_type_struct = handel;

	record->class_specific_function_list = NULL;

	handel->record = record;

	handel->last_measurement_interval = -1.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_handel_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_handel_finish_record_initialization()";

	MX_HANDEL *handel;

	handel = (MX_HANDEL *) record->record_type_struct;

	if ( handel == (MX_HANDEL *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_HANDEL pointer for record '%s' is NULL.",
			record->name );
	}

	/* We cannot allocate the following structures until
	 * mxi_handel_open() is invoked, so for now we
	 * initialize these data structures to all zeros.
	 */

	handel->num_mcas = 0;
	handel->mca_record_array = NULL;
	handel->save_filename[0] = '\0';

	return MX_SUCCESSFUL_RESULT;
}

/* mxi_handel_set_data_available_flags() sets mca->new_data_available
 * and handel_mca->new_statistics_available for all of the MCAs controlled
 * by this Handel interface.
 */

MX_EXPORT mx_status_type
mxi_handel_set_data_available_flags( MX_HANDEL *handel,
					int flag_value )
{
	static const char fname[] =
		"mxi_handel_set_data_available_flags()";

	MX_RECORD *mca_record, **mca_record_array;
	MX_MCA *mca;
	MX_HANDEL_MCA *handel_mca;
	unsigned long i;

	if ( handel == (MX_HANDEL *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_HANDEL pointer passed was NULL." );
	}

	MX_DEBUG( 2,("%s invoked for '%s', flag_value = %d",
		fname, handel->record->name, flag_value));

	mca_record_array = handel->mca_record_array;

	if ( mca_record_array == (MX_RECORD **) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The mca_record_array pointer for 'handel' record '%s'.",
			handel->record->name );
	}

	for ( i = 0; i < handel->num_mcas; i++ ) {

		mca_record = (handel->mca_record_array)[i];

		if ( mca_record == (MX_RECORD *) NULL ) {
			/* Skip this MCA slot. */

			continue;
		}

		mca = (MX_MCA *) mca_record->record_class_struct;

		if ( mca == (MX_MCA *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_MCA pointer for MCA record '%s' is NULL.",
				mca_record->name );
		}

		handel_mca = (MX_HANDEL_MCA *) mca_record->record_type_struct;

		if ( handel_mca == (MX_HANDEL_MCA *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		    "The MX_HANDEL_MCA pointer for MCA record '%s' is NULL.",
				mca_record->name );
		}

		mca->new_data_available = flag_value;
		handel_mca->new_statistics_available = flag_value;
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxi_handel_load_config( MX_HANDEL *handel )
{
	static const char fname[] = "mxi_handel_load_config()";

	MX_RECORD *mca_record;
	MX_HANDEL_MCA *handel_mca;
	int i, xia_status;

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	/* Load the configuration file. */

	mx_info("Loading Handel configuration '%s'.",
				handel->config_filename);

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	xia_status = xiaLoadSystem( "handel_ini", handel->config_filename );

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"xiaLoadSystem(\"handel_ini\", \"%s\")",
		handel->config_filename );
#endif

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Unable to load the Handel detector configuration file '%s'.  "
		"Error code = %d, '%s'",
			handel->config_filename,
			xia_status, mxi_handel_strerror( xia_status ) );
	}

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	xia_status = xiaStartSystem();

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, "xiaStartSystem()" );
#endif
	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		    "Unable to start the MCA using Handel "
		    "configuration file '%s'.  Error code = %d, '%s'",
		    handel->config_filename,
			xia_status, mxi_handel_strerror( xia_status ) );
	}

	/* Invalidate the old presets for all of the MCAs by setting the
	 * old preset type to an illegal value.
	 */

	if ( handel->mca_record_array != NULL ) {
		for ( i = 0; i < handel->num_mcas; i++ ) {
			mca_record = handel->mca_record_array[i];

			if ( mca_record != NULL ) {
				handel_mca = (MX_HANDEL_MCA *)
					mca_record->record_type_struct;

				handel_mca->old_preset_type
					= (unsigned long) MX_ULONG_MAX;
			}
		}
	}

	/* Add the active detector channels to the
	 * active detector channel set.
	 */

	for ( i = 0; i < handel->num_active_detector_channels; i++ ) {
		xia_status = xiaAddChannelSetElem(
			MX_HANDEL_ACTIVE_DETECTOR_CHANNEL_SET,
			(int) handel->active_detector_channel_array[i] );

		if ( xia_status != XIA_SUCCESS ) {
			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"Unable to add detector channel %d to active "
			"detector channel set %d.  Error code = %d, '%s'",
			(int) handel->active_detector_channel_array[i],
			MX_HANDEL_ACTIVE_DETECTOR_CHANNEL_SET,
			xia_status, mxi_handel_strerror( xia_status ) );
		}
	}

	mx_info("Successfully loaded Handel configuration '%s'.",
				handel->config_filename);

	return MX_SUCCESSFUL_RESULT;
}

/*
 * mxi_handel_load_new_config() encapsulates all of the operations that
 * must happen when a new configuration file is loaded by writing to the
 * MX record field 'config_filename'.
 */

static mx_status_type
mxi_handel_load_new_config( MX_HANDEL *handel )
{
	static const char fname[] = "mxi_handel_load_new_config()";

	MX_RECORD *mca_record;
	MX_HANDEL_MCA *handel_mca;
	int i, xia_status;
	double num_scas;
	mx_status_type mx_status;

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	/* Remove the old active detector channel set. */

	xia_status = xiaRemoveChannelSet(
			MX_HANDEL_ACTIVE_DETECTOR_CHANNEL_SET );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Attempt to delete active detector channel set %d "
		"failed for MCA '%s'.  Error code = %d, '%s'",
		MX_HANDEL_ACTIVE_DETECTOR_CHANNEL_SET,
		handel->record->name,
		xia_status, mxi_handel_strerror( xia_status ) );
	}

	/* Load the new configuration from the config file. */

	mx_status = mxi_handel_load_config( handel );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If possible, set the number of SCAs to 16 for each MCA. */

	for ( i = 0; i < handel->num_mcas; i++ ) {
		mca_record = handel->mca_record_array[i];

		if ( mca_record != (MX_RECORD *) NULL ) {
			handel_mca = (MX_HANDEL_MCA *)
					mca_record->record_type_struct;

			if ( handel_mca->hardware_scas_are_enabled ) {
				num_scas = 16.0;

				MX_DEBUG( 2,
				("%s: Setting 'number_of_scas' to %g",
					fname, num_scas));

				xia_status = xiaSetAcquisitionValues(
					handel_mca->detector_channel,
					"number_of_scas",
					(void *) &num_scas );

				if ( xia_status != XIA_SUCCESS ) {
					return mx_error(
					    MXE_INTERFACE_ACTION_FAILED, fname,
		"Attempt to set the number of SCAs to 16 for MCA '%s' failed.  "
		"Error code = %d, '%s'", mca_record->name,
		xia_status, mxi_handel_strerror( xia_status ) );
				}
			}
		}
	}

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"for record '%s'", handel->record->name );
#endif
	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxi_handel_save_config( MX_HANDEL *handel )
{
	static const char fname[] = "mxi_handel_save_config()";

	int xia_status;
	mx_status_type mx_status;

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	xia_status = xiaSaveSystem( "handel_ini", handel->save_filename );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Attempt to save the Handel configuration to the file '%s' "
		"failed for MCA '%s'.  Error code = %d, '%s'",
		handel->save_filename, handel->record->name,
		xia_status, mxi_handel_strerror( xia_status ) );
	}

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"for record '%s'", handel->record->name );
#endif
	return MX_SUCCESSFUL_RESULT;
}

static void
mxi_handel_redirect_log_output( MX_HANDEL *handel )
{
	char log_filename[MXU_FILENAME_LENGTH+1];
	char *mxdir;
	int xia_status;

	if ( strlen( handel->log_filename ) > 0 ) {
		strlcpy( log_filename, handel->log_filename,
						MXU_FILENAME_LENGTH );
	} else {
		mxdir = getenv("MXDIR");

		if ( mxdir == NULL ) {
			strcpy( log_filename, "/opt/mx/log/handel.log" );
		} else {
			sprintf( log_filename, "%s/log/handel.log", mxdir );
		}
	}

	mx_info( "Redirecting Handel log output to '%s'.", log_filename );

	/* Redirect the log output. */

	xia_status = xiaSetLogOutput( log_filename );

	if ( xia_status != XIA_SUCCESS ) {
		mx_warning(
		    "Unable to redirect XIA Handel log output to file '%s'.  "
		    "Error code = %d, '%s'", log_filename, xia_status,
			mxi_handel_strerror( xia_status ) );
	}

	return;
}

MX_EXPORT mx_status_type
mxi_handel_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_handel_open()";

	MX_HANDEL *handel;
	char detector_alias[MAXALIAS_LEN+1];
	char version_string[80];
	int xia_status, display_config;
	unsigned int i, j, num_detectors, num_modules;
	unsigned int num_mcas, total_num_mcas;
	void *void_ptr;
	long dimension_array[2];
	size_t size_array[2];
	mx_status_type mx_status;

#if MX_IGNORE_XIA_NULL_STRING
	char *ignore_this_pointer;
#endif

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	/* The following statement exist only to suppress the GCC warning
	 * "warning: `XIA_NULL_STRING' defined but not used".  You should
	 * not actually use the variable 'ignore_this_pointer' for anything.
	 */

#if MX_IGNORE_XIA_NULL_STRING
	ignore_this_pointer = XIA_NULL_STRING;
#endif

	MX_DEBUG( 2,("%s invoked.", fname));

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	handel = (MX_HANDEL *) (record->record_type_struct);

	if ( handel == (MX_HANDEL *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_HANDEL pointer for record '%s' is NULL.", record->name);
	}

	display_config = handel->handel_flags &
			MXF_HANDEL_DISPLAY_CONFIGURATION_AT_STARTUP;

#if 0
	/* Redirect Handel and Xerxes output to the MX output functions. */

	mx_status = mxu_xia_dxp_replace_output_functions();

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
#endif

	/* Initialize mca_record_array to NULL, so that load_config
	 * below knows not to try to change the MCA presets.
	 */

	handel->mca_record_array = NULL;

	/* Initialize Handel. */

	xia_status = xiaInitHandel();

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Unable to initialize Handel.  "
		"Error code = %d, '%s'",
			xia_status, mxi_handel_strerror( xia_status ) );
	}

	/* If requested, redirect log output to a file. */

	if (handel->handel_flags & MXF_HANDEL_WRITE_LOG_OUTPUT_TO_FILE)
	{
		mxi_handel_redirect_log_output( handel );
	}

	/* Set the logging level. */

	if ( handel->handel_log_level < MD_ERROR ) {
		handel->handel_log_level = MD_ERROR;
	}
	if ( handel->handel_log_level > MD_DEBUG ) {
		handel->handel_log_level = MD_DEBUG;
	}

	xia_status = xiaSetLogLevel( handel->handel_log_level );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"An attempt to set the Handel log level to %d failed.  "
		"Error code = %d, '%s'", handel->handel_log_level,
			xia_status, mxi_handel_strerror( xia_status ) );
	}

	/* Display the version of Handel/Xerxes. */

	xiaGetVersionInfo( NULL, NULL, NULL, version_string );

	mx_info("MX is using Handel %s", version_string);

	/* Load the detector configuration. */

	mx_status = mxi_handel_load_config( handel );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* How many detectors are there? */

	xia_status = xiaGetNumDetectors( &num_detectors );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Unable to find out how many detectors are in "
		"the current configuration '%s'.  "
		"Error code = %d, '%s'",
			handel->config_filename,
			xia_status, mxi_handel_strerror( xia_status ) );
	}

	handel->num_detectors = num_detectors;

	total_num_mcas = 0;

	/* Find out how many detector channels (MCAs) there are
	 * for all detectors.
	 */

	for ( i = 0; i < num_detectors; i++ ) {

		xia_status = xiaGetDetectors_VB( i, detector_alias );

		if ( xia_status != XIA_SUCCESS ) {
			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Unable to get the detector alias for XIA detector %d.  "
		"Error code = %d, '%s'", i,
			xia_status, mxi_handel_strerror( xia_status ) );
		}

		void_ptr = (void *) &num_mcas;

		xia_status = xiaGetDetectorItem( detector_alias,
						"number_of_channels",
						void_ptr );

		if ( xia_status != XIA_SUCCESS ) {
			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
	"Unable to get the number of channels for XIA detector %d '%s'.  "
	"Error code = %d, '%s'", i, detector_alias,
			xia_status, mxi_handel_strerror( xia_status ) );
		}

		total_num_mcas += num_mcas;
	}

	handel->num_mcas = total_num_mcas;

	/* Allocate and initialize an array to store pointers to all of
	 * the MCA records used by this interface.
	 */

	handel->mca_record_array = ( MX_RECORD ** ) malloc(
					num_mcas * sizeof( MX_RECORD * ) );

	if ( handel->mca_record_array == ( MX_RECORD ** ) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %ld element array "
		"of MX_RECORD pointers for the mca_record_array data "
		"structure of record '%s'.",
			handel->num_mcas, record->name );
	}

	for ( i = 0; i < handel->num_mcas; i++ ) {
		handel->mca_record_array[i] = NULL;
	}

	/* Find out how many modules there are. */

	xia_status = xiaGetNumModules( &num_modules );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Unable to find out how many modules are in "
		"the current configuration '%s'.  "
		"Error code = %d, '%s'",
			handel->config_filename,
			xia_status, mxi_handel_strerror( xia_status ) );
	}

	handel->num_modules = num_modules;

	/* Allocate a two-dimensional module pointer array. */

	handel->mcas_per_module = 4;	/* FIXME: For the DXP-XMAP. */

	dimension_array[0] = handel->num_modules;
	dimension_array[1] = handel->mcas_per_module;

	size_array[0] = sizeof(MX_RECORD *);
	size_array[1] = sizeof(MX_RECORD **);

	handel->module_array = mx_allocate_array( 2,
				dimension_array, size_array );

	if ( handel->module_array == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a ( %ld x %ld ) "
		"module array for Handel interface '%s'.",
			handel->num_modules, handel->mcas_per_module,
			record->name );
	}

	for ( i = 0; i < handel->num_modules; i++ ) {
		for ( j = 0; j < handel->mcas_per_module; j++ ) {
			handel->module_array[i][j] = NULL;
		}
	}

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"for record '%s'", record->name );
#endif

	if ( display_config ) {
		mx_info( "XIA Handel library initialized for MX record '%s'.",
			record->name );
		mx_info(
		"num detectors = %ld, num channels = %ld, num modules = %ld",
			handel->num_detectors,
			handel->num_mcas,
			handel->num_modules );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_handel_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxi_handel_resynchronize()";

	MX_HANDEL *handel;
	mx_status_type mx_status;

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	handel = (MX_HANDEL *) (record->record_type_struct);

	if ( handel == (MX_HANDEL *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_HANDEL pointer for record '%s' is NULL.", record->name);
	}

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	mx_status = mxi_handel_load_config( handel );

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, "for record '%s'", record->name );
#endif

	return mx_status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_handel_is_busy( MX_MCA *mca, mx_bool_type *busy_flag )
{
	static const char fname[] = "mxi_handel_is_busy()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL *handel;
	unsigned long run_active, mask;
	int xia_status;
	mx_status_type mx_status;

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxi_handel_get_pointers( mca,
					&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	xia_status = xiaGetRunData( handel_mca->detector_channel,
					"run_active", &run_active );

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"for record '%s'", mca->record->name );
#endif

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot get DXP running status for MCA '%s'.  "
		"Error code = %d, '%s'",
			mca->record->name, xia_status,
			mxi_handel_strerror( xia_status ) );
	}

	if ( handel_mca->debug_flag ) {
		MX_DEBUG(-2,("%s: run_active = %#lx", fname, run_active));
	}

#if 0
	mask = XIA_RUN_HARDWARE | XIA_RUN_HANDEL | XIA_RUN_CT;
#else
	mask = XIA_RUN_HARDWARE;
#endif

	if ( run_active & mask ) {
		*busy_flag = TRUE;
	} else {
		*busy_flag = FALSE;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_handel_get_run_data( MX_MCA *mca,
			char *name,
			void *value_ptr )
{
	static const char fname[] = "mxi_handel_get_run_data()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL *handel;
	unsigned long run_active, mask;
	int xia_status;
	mx_status_type mx_status;

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif
	mx_status = mxi_handel_get_pointers( mca,
					&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( name == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'name' argument passed for MCA '%s' was NULL.",
			mca->record->name );
	}
	if ( value_ptr == (void *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'value_ptr' argument passed for MCA '%s' was NULL.",
			mca->record->name );
	}

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	xia_status = xiaGetRunData( handel_mca->detector_channel,
					name, value_ptr );

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"for record '%s', name '%s'",
		mca->record->name, name );
#endif

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot get DXP run data for '%s' of MCA '%s'.  "
		"Error code = %d, '%s'", name,
			mca->record->name, xia_status,
			mxi_handel_strerror( xia_status ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* NOTE: According to Patrick Franz, Handel acquisition values are always
 *       doubles, in spite of the fact that the API uses void pointers.
 */

MX_EXPORT mx_status_type
mxi_handel_get_acquisition_values( MX_MCA *mca,
				char *value_name,
				double *value_ptr )
{
	static const char fname[] = "mxi_handel_get_acquisition_values()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL *handel;
	int xia_status;
	mx_status_type mx_status;

	mx_status = mxi_handel_get_pointers( mca,
					&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( value_name == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'value_name' argument passed for MCA '%s' was NULL.",
			mca->record->name );
	}
	if ( value_ptr == (double *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'value_ptr' argument passed for MCA '%s' was NULL.",
			mca->record->name );
	}

	MX_DEBUG(-2,("%s: getting acquisition value '%s' for MCA '%s'.",
		fname, value_name, mca->record->name ));

	xia_status = xiaGetAcquisitionValues( handel_mca->detector_channel,
					value_name, (void *) value_ptr );

	switch( xia_status ) {
	case XIA_SUCCESS:
		break;		/* Everything is OK. */
	case XIA_UNKNOWN_VALUE:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The requested acquisition value '%s' for MCA '%s' "
			"does not exist.", value_name, mca->record->name );
		break;
	default:
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"Cannot get acquisition value '%s' for MCA '%s'.  "
			"Error code = %d, '%s'", 
					value_name,
					mca->record->name,
					xia_status,
					mxi_handel_strerror( xia_status ) );
	}

	MX_DEBUG(-2,("%s: acquisition value '%s' for MCA '%s' = %g.",
			fname, value_name, mca->record->name, *value_ptr ));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_handel_set_acquisition_values( MX_MCA *mca,
				char *value_name,
				double *value_ptr,
				mx_bool_type apply_flag )
{
	static const char fname[] = "mxi_handel_set_acquisition_values()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL *handel;
	int xia_status, ignored;
	mx_status_type mx_status;

	mx_status = mxi_handel_get_pointers( mca,
					&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( value_name == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'value_name' argument passed for MCA '%s' was NULL.",
			mca->record->name );
	}
	if ( value_ptr == (double *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'value_ptr' argument passed for MCA '%s' was NULL.",
			mca->record->name );
	}

	MX_DEBUG(-2,("%s: setting acquisition value '%s' for MCA '%s' to %g.",
		fname, value_name, mca->record->name, *value_ptr ));

	xia_status = xiaSetAcquisitionValues( handel_mca->detector_channel,
					value_name, (void *) value_ptr );

	switch( xia_status ) {
	case XIA_SUCCESS:
		break;		/* Everything is OK. */
	case XIA_UNKNOWN_VALUE:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The requested acquisition value '%s' for MCA '%s' "
			"does not exist.", value_name, mca->record->name );
		break;
	default:
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Cannot set acquisition value '%s' for MCA '%s' to %g.  "
		"Error code = %d, '%s'", 
					value_name,
					mca->record->name,
					value_ptr,
					xia_status,
					mxi_handel_strerror( xia_status ) );
	}

	if ( apply_flag ) {
		mx_status = mxi_handel_apply( mca, TRUE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_handel_set_acq_for_all_channels( MX_MCA *mca,
				char *value_name,
				double *value_ptr,
				mx_bool_type apply_flag )
{
	static const char fname[] = "mxi_handel_set_acq_for_all_channels()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL *handel;
	int xia_status;
	mx_status_type mx_status;

	mx_status = mxi_handel_get_pointers( mca,
					&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( value_name == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'value_name' argument passed for MCA '%s' was NULL.",
			mca->record->name );
	}
	if ( value_ptr == (double *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'value_ptr' argument passed for MCA '%s' was NULL.",
			mca->record->name );
	}

	MX_DEBUG(-2,("%s: setting acquisition value '%s' for MCA '%s' to %g.",
		fname, value_name, mca->record->name, *value_ptr ));

	xia_status = xiaSetAcquisitionValues( -1,
					value_name, (void *) value_ptr );

	switch( xia_status ) {
	case XIA_SUCCESS:
		break;		/* Everything is OK. */
	case XIA_UNKNOWN_VALUE:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The requested acquisition value '%s' for MCA '%s' "
			"does not exist.", value_name, mca->record->name );
		break;
	default:
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Cannot set acquisition value '%s' for MCA '%s' to %g.  "
		"Error code = %d, '%s'", 
					value_name,
					mca->record->name,
					value_ptr,
					xia_status,
					mxi_handel_strerror( xia_status ) );
	}

	if ( apply_flag ) {
		mx_status = mxi_handel_apply( mca, TRUE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_handel_apply( MX_MCA *mca, mx_bool_type apply_to_all )
{
	static const char fname[] = "mxi_handel_apply()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL *handel;
	int m, xia_status, ignored;
	MX_RECORD *first_mca_record;
	MX_HANDEL_MCA *first_handel_mca;
	mx_status_type mx_status;

	mx_status = mxi_handel_get_pointers( mca,
					&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( handel_mca->debug_flag ) {
		MX_DEBUG(-2,("%s for MCA '%s', apply_to_all = %d",
			fname, mca->record->name, (int) apply_to_all ));
	}

	if ( apply_to_all == FALSE ) {
		ignored = 0;

		xia_status = xiaBoardOperation( handel_mca->detector_channel,
						"apply", (void *) &ignored );

		if ( xia_status != XIA_SUCCESS ) {
			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"Cannot apply acquisition values for MCA '%s'.  "
			"Error code = %d, '%s'", 
				mca->record->name,
				xia_status,
				mxi_handel_strerror( xia_status ) );
		}
	} else {
		for ( m = 0; m < handel->num_modules; m++ ) {
			/* Find the first MCA for each module and then
			 * do an apply to it.
			 */

			first_mca_record = handel->module_array[m][0];

			if ( first_mca_record == NULL ) {
				continue;  /* Go back to the top of the loop. */
			}

			first_handel_mca = first_mca_record->record_type_struct;

			if ( first_handel_mca == NULL ) {
				return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
				fname, "The MX_HANDEL_MCA pointer for Handel "
				"MCA '%s' is NULL.",
					first_mca_record->name );
			}

			if ( handel_mca->debug_flag ) {
				MX_DEBUG(-2,("%s: applying to MCA '%s'",
					fname, first_mca_record->name ));
			}

			ignored = 0;

			xia_status = xiaBoardOperation(
					first_handel_mca->detector_channel,
					"apply", (void *) &ignored );

			if ( xia_status != XIA_SUCCESS ) {
				return mx_error( MXE_INTERFACE_ACTION_FAILED,
				fname, "Cannot apply acquisition values for "
				"MCA '%s' on behalf of MCA '%s'.  "
				"Error code = %d, '%s'",
					first_mca_record->name,
					mca->record->name,
					xia_status,
					mxi_handel_strerror( xia_status ) );
			}
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_handel_read_parameter( MX_MCA *mca,
			char *parameter_name,
			unsigned long *value_ptr )
{
	static const char fname[] = "mxi_handel_read_parameter()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL *handel;
	unsigned short short_value;
	int xia_status;
	mx_status_type mx_status;

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxi_handel_get_pointers( mca,
					&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( handel_mca->debug_flag ) {
		MX_DEBUG(-2,
		("%s: Record '%s', channel %d, parameter '%s'.",
			fname, mca->record->name,
			handel_mca->detector_channel, parameter_name));
	}

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	xia_status = xiaGetParameter( handel_mca->detector_channel,
						parameter_name, &short_value );

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"for record '%s', channel %d, parameter '%s'",
		mca->record->name,
		handel_mca->detector_channel,
		parameter_name);
#endif

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot read DXP parameter '%s' for detector channel %d "
		"of XIA DXP interface '%s'.  "
		"Error code = %d, '%s'",
			parameter_name, handel_mca->detector_channel,
			mca->record->name, xia_status,
			mxi_handel_strerror( xia_status ) );
	}

	*value_ptr = (unsigned long) short_value;

	if ( handel_mca->debug_flag ) {
		MX_DEBUG(-2,("%s: value = %lu", fname, *value_ptr));
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_handel_write_parameter( MX_MCA *mca,
			char *parameter_name,
			unsigned long value )
{
	static const char fname[] = "mxi_handel_write_parameter()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL *handel;
	unsigned short short_value;
	int xia_status;
	mx_status_type mx_status;

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxi_handel_get_pointers( mca,
					&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( handel_mca->debug_flag ) {
		MX_DEBUG(-2,
		("%s: Record '%s', channel %d, parameter '%s', value = %lu.",
			fname, mca->record->name, handel_mca->detector_channel,
			parameter_name, (unsigned long) value));
	}

	/* Send the parameter value. */

	short_value = (unsigned short) value;

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	xia_status = xiaSetParameter( handel_mca->detector_channel,
						parameter_name, short_value );

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"for record '%s', channel %d, parameter '%s'",
		mca->record->name,
		handel_mca->detector_channel,
		parameter_name);
#endif

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot write the value %hu to DXP parameter '%s' "
		"for detector channel %d of XIA DXP interface '%s'.  "
		"Error code = %d, '%s'", short_value,
			parameter_name, handel_mca->detector_channel,
			mca->record->name, xia_status,
			mxi_handel_strerror( xia_status ) );
	}

	if ( handel_mca->debug_flag ) {
		MX_DEBUG(-2,
		("%s: Write to parameter '%s' succeeded.",
			fname, parameter_name));
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_handel_write_parameter_to_all_channels( MX_MCA *mca,
			char *parameter_name,
			unsigned long value )
{
	static const char fname[] =
		"mxi_handel_write_parameter_to_all_channels()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL *handel;
	unsigned short short_value;
	int xia_status;
	mx_status_type mx_status;

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxi_handel_get_pointers( mca,
					&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( handel_mca->debug_flag ) {
		MX_DEBUG(-2,("%s: Record '%s', parameter '%s', value = %lu.",
			fname, mca->record->name,
			parameter_name, (unsigned long) value));
	}

	short_value = (unsigned short) value;

#if 0
	/* I don't know yet whether there is a Handel function for this. */

#else
	/* A temporary kludge which does work. */

	{
		MX_RECORD *mca_record;
		MX_HANDEL_MCA *local_dxp_mca;
		unsigned long i;

		for ( i = 0; i < handel->num_mcas; i++ ) {

			mca_record = handel->mca_record_array[i];

			if ( mca_record == (MX_RECORD *) NULL ) {

				continue;    /* Skip this one. */
			}

			local_dxp_mca = (MX_HANDEL_MCA *)
					mca_record->record_type_struct;

			MX_DEBUG( 2,("%s: writing to detector channel %d.",
				fname, local_dxp_mca->detector_channel));

#if MXI_HANDEL_DEBUG_TIMING
			MX_HRT_START( measurement );
#endif
			xia_status = xiaSetParameter(
					local_dxp_mca->detector_channel,
					parameter_name, short_value );

#if MXI_HANDEL_DEBUG_TIMING
			MX_HRT_END( measurement );

			MX_HRT_RESULTS( measurement, fname,
				"for record '%s', channel %d, parameter '%s'",
				mca_record->name,
				local_dxp_mca->detector_channel,
				parameter_name );
#endif

			if ( xia_status != XIA_SUCCESS ) {
				return mx_error( MXE_DEVICE_ACTION_FAILED,fname,
			"Cannot write the value %hu to DXP parameter '%s' "
			"for detector channel %d of XIA DXP interface '%s'.  "
			"Error code = %d, '%s'", short_value,
				parameter_name, local_dxp_mca->detector_channel,
				mca->record->name, xia_status,
				mxi_handel_strerror( xia_status ) );
			}
		}
	}
#endif
	if ( handel_mca->debug_flag ) {
		MX_DEBUG(-2,
		("%s: Write parameter '%s' to all channels succeeded.",
			fname, parameter_name));
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_handel_start_run( MX_MCA *mca, mx_bool_type clear_flag )
{
	static const char fname[] = "mxi_handel_start_run()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL *handel;
	unsigned short resume_flag;
	int xia_status;
	mx_status_type mx_status;

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxi_handel_get_pointers( mca,
					&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( handel_mca->debug_flag ) {
		MX_DEBUG(-2,("%s: Record '%s', clear_flag = %d.",
				fname, mca->record->name, (int) clear_flag));
	}

	mxi_handel_set_data_available_flags( handel, TRUE );

	if ( clear_flag ) {
		resume_flag = 0;
	} else {
		resume_flag = 1;
	}

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	xia_status = xiaStartRun( handel_mca->detector_channel, resume_flag );

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, mca->record->name );
#endif

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot start data acquisition for XIA DXP interface '%s'.  "
		"Error code = %d, '%s'", mca->record->name,
			xia_status, mxi_handel_strerror( xia_status ) );
	}

	return MX_SUCCESSFUL_RESULT;
}


MX_EXPORT mx_status_type
mxi_handel_stop_run( MX_MCA *mca )
{
	static const char fname[] = "mxi_handel_stop_run()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL *handel;
	int xia_status;
	mx_status_type mx_status;

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxi_handel_get_pointers( mca,
					&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( handel_mca->debug_flag ) {
		MX_DEBUG(-2,("%s: Record '%s'.", fname, mca->record->name));
	}

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	xia_status = xiaStopRun( handel_mca->detector_channel );

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, mca->record->name );
#endif

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot stop data acquisition for XIA DXP interface '%s'.  "
		"Error code = %d, '%s'", mca->record->name,
			xia_status, mxi_handel_strerror( xia_status ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_handel_read_spectrum( MX_MCA *mca )
{
	static const char fname[] = "mxi_handel_read_spectrum()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL *handel;
	int xia_status;
	unsigned long *array_ptr;
	mx_status_type mx_status;

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxi_handel_get_pointers( mca,
					&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: handel_mca->use_mca_channel_array = %d",
		fname, handel_mca->use_mca_channel_array));

	if ( handel_mca->use_mca_channel_array ) {
		array_ptr = mca->channel_array;
	} else {
		array_ptr = handel_mca->spectrum_array;
	}

	if ( handel_mca->debug_flag ) {
		MX_DEBUG(-2,("%s: reading out %ld channels from MCA '%s'.",
			fname, mca->current_num_channels, mca->record->name));
	}

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	xia_status = xiaGetRunData( handel_mca->detector_channel,
					"mca", array_ptr );

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, mca->record->name );
#endif

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot read the spectrum for XIA MCA '%s'.  "
		"Error code = %d, '%s'", mca->record->name,
			xia_status, mxi_handel_strerror( xia_status ) );
	}

#if 1
	/* FIXME - For some reason the value in the last bin is always bad.
	 * Thus, we set it to zero.  (W. Lavender)
	 */

	array_ptr[ mca->current_num_channels - 1 ] = 0;
#endif

	if ( handel_mca->debug_flag ) {
		MX_DEBUG(-2,("%s: readout from MCA '%s' complete.",
			fname, mca->record->name));
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_handel_read_statistics( MX_MCA *mca )
{
	static const char fname[] = "mxi_handel_read_statistics()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL *handel;
	double module_statistics[28];	/* FIXME: 28 is for DXP-XMAP */
	long channel_offset;
	mx_status_type mx_status;

	mx_status = mxi_handel_get_pointers( mca,
					&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_handel_get_run_data( mca, "module_statistics",
					(void *) module_statistics );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( strcmp( handel_mca->module_type, "xmap" ) == 0 ) {
		channel_offset = handel_mca->detector_channel % 4;

		mca->real_time = module_statistics[ channel_offset + 0 ];
		mca->live_time = module_statistics[ channel_offset + 1 ];

#if 0
		handel_mca->energy_live_time
				= module_statistics[ channel_offset + 2 ];
#endif
		handel_mca->num_triggers
				= module_statistics[ channel_offset + 3 ];
		handel_mca->num_events
				= module_statistics[ channel_offset + 4 ];
		handel_mca->input_count_rate
				= module_statistics[ channel_offset + 5 ];
		handel_mca->output_count_rate
				= module_statistics[ channel_offset + 6 ];
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"The reading of module statistics for module type '%s' "
		"of MCA '%s' is not currently supported.",
			handel_mca->module_type,
			mca->record->name );
	}

	if ( handel_mca->debug_flag ) {
		MX_DEBUG(-2,(
	"%s: Record '%s', channel %d, live_time = %g, real_time = %g, "
	"icr = %g, ocr = %g, nevent = %lu", fname,
			mca->record->name,
			handel_mca->detector_channel,
			mca->live_time,
			mca->real_time,
			handel_mca->input_count_rate,
			handel_mca->output_count_rate,
			handel_mca->num_events ));
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_handel_get_baseline_array( MX_MCA *mca )
{
	static const char fname[] =
			"mxi_handel_get_baseline_array()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL *handel;
	MX_RECORD_FIELD *field;
	int xia_status;
	void *void_ptr;
	unsigned baseline_length;
	mx_status_type mx_status;

	mx_status = mxi_handel_get_pointers( mca,
					&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for MCA '%s'.", fname, mca->record->name ));

	/* Get the current baseline length. */

	void_ptr = (void *) &baseline_length;

	xia_status = xiaGetRunData( handel_mca->detector_channel,
					"baseline_length", void_ptr );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"Cannot get the baseline length for MCA '%s'.  "
			"Error code = %d, '%s'", 
					mca->record->name,
					xia_status,
					mxi_handel_strerror( xia_status ) );
	}

	if ( baseline_length == 0 ) {
		return mx_error( MXE_CONTROLLER_INTERNAL_ERROR, fname,
	"The reported value of the baseline length for MCA '%s' is 0."
		"  This should not happen.", mca->record->name );
	}

	MX_DEBUG(-2,("%s: baseline_length = %lu",
			fname, baseline_length));

	/* Allocate or resize the baseline array as necessary. */

	if ( handel_mca->baseline_array == NULL ) {
	    handel_mca->baseline_array = (unsigned long *)
		malloc( baseline_length * sizeof(unsigned long) );

	    if ( handel_mca->baseline_array == (unsigned long *) NULL)
	    {
		handel_mca->baseline_length = 0;

		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory attempting to allocate a "
			"%lu element baseline array for MCA '%s'.",
				baseline_length, mca->record->name );
	    }

	    handel_mca->baseline_length = baseline_length;
	} else
	if ( baseline_length > handel_mca->baseline_length ) {
	    handel_mca->baseline_array = (unsigned long *)
		realloc( handel_mca->baseline_array,
			baseline_length * sizeof(unsigned long) );

	    if ( handel_mca->baseline_array == (unsigned long *) NULL)
	    {
		handel_mca->baseline_length = 0;

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory attempting to resize the baseline "
			"array to %lu elements for MCA '%s'.",
				baseline_length, mca->record->name );
	    }

	    handel_mca->baseline_length = baseline_length;
	}

	MX_DEBUG(-2,("%s: getting baseline array for MCA '%s'.",
		fname, mca->record->name ));

	/* Now read the baseline array. */

	void_ptr = (void *) &(handel_mca->baseline_array[0]);

	xia_status = xiaGetRunData( handel_mca->detector_channel,
					"baseline", void_ptr );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"Cannot get the baseline array for MCA '%s'.  "
			"Error code = %d, '%s'", 
					mca->record->name,
					xia_status,
					mxi_handel_strerror( xia_status ) );
	}

#if 1
	{
		unsigned long i;

		fprintf(stderr, "Start of baseline array:\n");

		for ( i = 0; i < 100; i++ ) {
			fprintf( stderr, "%lu ",
				handel_mca->baseline_array[i] );
		}

		fprintf(stderr, "\nEnd of baseline array\n");
	}
#endif

	/* If everything worked, we must change the dimension value in
	 * the record field.
	 */

	mx_status = mx_find_record_field( mca->record,
					"baseline_array", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = handel_mca->baseline_length;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_handel_set_gain_change( MX_MCA *mca )
{
	static const char fname[] = "mxi_handel_set_gain_change()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL *handel;
	int xia_status;
	mx_status_type mx_status;

	mx_status = mxi_handel_get_pointers( mca,
					&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for MCA '%s', gain_change = %g",
		fname, mca->record->name, handel_mca->gain_change));

	xia_status = xiaGainChange( handel_mca->detector_channel,
					handel_mca->gain_change );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"Cannot change the gain for MCA '%s' to %g.  "
			"Error code = %d, '%s'", 
					mca->record->name,
					handel_mca->gain_change,
					xia_status,
					mxi_handel_strerror( xia_status ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_handel_set_gain_calibration( MX_MCA *mca )
{
	static const char fname[] = "mxi_handel_set_gain_calibration()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL *handel;
	int xia_status;
	mx_status_type mx_status;

	mx_status = mxi_handel_get_pointers( mca,
					&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for MCA '%s', gain_calibration = %g",
		fname, mca->record->name, handel_mca->gain_calibration));

	xia_status = xiaGainCalibrate( handel_mca->detector_channel,
					handel_mca->gain_calibration );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"Cannot set the gain calibration for MCA '%s' to %g.  "
			"Error code = %d, '%s'", 
					mca->record->name,
					handel_mca->gain_calibration,
					xia_status,
					mxi_handel_strerror( xia_status ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_handel_get_adc_trace_array( MX_MCA *mca )
{
	static const char fname[] = "mxi_handel_get_adc_trace_array()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL *handel;
	MX_RECORD_FIELD *field;
	int xia_status;
	void *void_ptr;
	unsigned adc_trace_length;
	mx_status_type mx_status;

	/* Tracewait of 5 microseconds (in nanoseconds). */

	double info[2];

	mx_status = mxi_handel_get_pointers( mca,
					&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for MCA '%s'.", fname, mca->record->name ));

	/* Get the current ADC trace length. */

	void_ptr = (void *) &adc_trace_length;

	xia_status = xiaGetSpecialRunData( handel_mca->detector_channel,
					"adc_trace_length", void_ptr );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"Cannot get the ADC trace length for MCA '%s'.  "
			"Error code = %d, '%s'", 
					mca->record->name,
					xia_status,
					mxi_handel_strerror( xia_status ) );
	}

	if ( adc_trace_length == 0 ) {
		return mx_error( MXE_CONTROLLER_INTERNAL_ERROR, fname,
		"The reported value of the ADC trace length for MCA '%s' is 0."
		"  This should not happen.", mca->record->name );
	}

	MX_DEBUG(-2,("%s: adc_trace_length = %lu", fname, adc_trace_length));

	/* Allocate or resize the ADC trace array as necessary. */

	if ( handel_mca->adc_trace_array == NULL ) {
		handel_mca->adc_trace_array = (unsigned long *)
			malloc( adc_trace_length * sizeof(unsigned long) );

		if ( handel_mca->adc_trace_array == (unsigned long *) NULL ) {
			handel_mca->adc_trace_length = 0;

			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory attempting to allocate a "
			"%lu element ADC trace array for MCA '%s'.",
				adc_trace_length, mca->record->name );
		}

		handel_mca->adc_trace_length = adc_trace_length;
	} else
	if ( adc_trace_length > handel_mca->adc_trace_length ) {
		handel_mca->adc_trace_array = (unsigned long *)
			realloc( handel_mca->adc_trace_array,
				adc_trace_length * sizeof(unsigned long) );

		if ( handel_mca->adc_trace_array == (unsigned long *) NULL ) {
			handel_mca->adc_trace_length = 0;

			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory attempting to resize the ADC trace "
			"array to %lu elements for MCA '%s'.",
				adc_trace_length, mca->record->name );
		}

		handel_mca->adc_trace_length = adc_trace_length;
	}

	/* Start the special run. */

	info[0] = 1.0;
	info[1] = handel_mca->adc_trace_step_size;

	void_ptr = (void *) &(info[0]);

	xia_status = xiaDoSpecialRun( handel_mca->detector_channel,
					"adc_trace", void_ptr );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"Cannot start special run 'adc_trace' for MCA '%s'.  "
			"Error code = %d, '%s'", 
					mca->record->name,
					xia_status,
					mxi_handel_strerror( xia_status ) );
	}

#if 1
	{
		unsigned long seconds, milliseconds, microseconds;
		double total_delay, ulong_max;

		total_delay = 1.0e-9 * handel_mca->adc_trace_step_size
				* (double) handel_mca->adc_trace_length;

		ulong_max = (double) MX_ULONG_MAX;

		MX_DEBUG(-2,("%s: total_delay = %g seconds, ulong_max = %g",
			fname, total_delay, ulong_max));

		if ( total_delay <= (1.0e-3 * ulong_max) ) {
			seconds = mx_round( total_delay );

			mx_sleep( seconds );
		} else
		if ( total_delay <= (1.0e-6 * ulong_max) ) {
			milliseconds = mx_round( 1.0e3 * total_delay );

			mx_msleep( milliseconds );
		} else {
			microseconds = mx_round( 1.0e6 * total_delay );

			mx_usleep( microseconds );
		}
	}
#endif

	MX_DEBUG(-2,("%s: getting ADC trace array for MCA '%s'.",
		fname, mca->record->name ));

	/* Finally, we can now read the ADC trace array. */

	void_ptr = (void *) &(handel_mca->adc_trace_array[0]);

	xia_status = xiaGetSpecialRunData( handel_mca->detector_channel,
					"adc_trace", void_ptr );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"Cannot get the ADC trace array for MCA '%s'.  "
			"Error code = %d, '%s'", 
					mca->record->name,
					xia_status,
					mxi_handel_strerror( xia_status ) );
	}

#if 1
	{
		unsigned long i;

		fprintf(stderr, "Start of ADC trace array:\n");

		for ( i = 0; i < 100; i++ ) {
			fprintf( stderr, "%lu ",
				handel_mca->adc_trace_array[i] );
		}

		fprintf(stderr, "\nEnd of ADC trace array\n");
	}
#endif

	/* If everything worked, we must change the dimension value in
	 * the record field.
	 */

	mx_status = mx_find_record_field( mca->record,
					"adc_trace_array", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = handel_mca->adc_trace_length;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_handel_get_baseline_history_array( MX_MCA *mca )
{
	static const char fname[] =
			"mxi_handel_get_baseline_history_array()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL *handel;
	MX_RECORD_FIELD *field;
	int xia_status;
	void *void_ptr;
	int info[1];
	unsigned baseline_history_length;
	mx_status_type mx_status;

	mx_status = mxi_handel_get_pointers( mca,
					&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for MCA '%s'.", fname, mca->record->name ));

	/* Get the current baseline history length. */

	void_ptr = (void *) &baseline_history_length;

	xia_status = xiaGetSpecialRunData( handel_mca->detector_channel,
					"baseline_history_length", void_ptr );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"Cannot get the baseline history length for MCA '%s'.  "
			"Error code = %d, '%s'", 
					mca->record->name,
					xia_status,
					mxi_handel_strerror( xia_status ) );
	}

	if ( baseline_history_length == 0 ) {
		return mx_error( MXE_CONTROLLER_INTERNAL_ERROR, fname,
	"The reported value of the baseline history length for MCA '%s' is 0."
		"  This should not happen.", mca->record->name );
	}

	MX_DEBUG(-2,("%s: baseline_history_length = %lu",
			fname, baseline_history_length));

	/* Allocate or resize the baseline history array as necessary. */

	if ( handel_mca->baseline_history_array == NULL ) {
	    handel_mca->baseline_history_array = (unsigned long *)
		malloc( baseline_history_length * sizeof(unsigned long) );

	    if ( handel_mca->baseline_history_array == (unsigned long *) NULL)
	    {
		handel_mca->baseline_history_length = 0;

		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory attempting to allocate a "
			"%lu element baseline history array for MCA '%s'.",
				baseline_history_length, mca->record->name );
	    }

	    handel_mca->baseline_history_length = baseline_history_length;
	} else
	if ( baseline_history_length > handel_mca->baseline_history_length ) {
	    handel_mca->baseline_history_array = (unsigned long *)
		realloc( handel_mca->baseline_history_array,
			baseline_history_length * sizeof(unsigned long) );

	    if ( handel_mca->baseline_history_array == (unsigned long *) NULL)
	    {
		handel_mca->baseline_history_length = 0;

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory attempting to resize the baseline history "
			"array to %lu elements for MCA '%s'.",
				baseline_history_length, mca->record->name );
	    }

	    handel_mca->baseline_history_length = baseline_history_length;
	}

	/* Start the special run. */

	info[0] = 1;

	void_ptr = (void *) &(info[0]);

	xia_status = xiaDoSpecialRun( handel_mca->detector_channel,
					"baseline_history", void_ptr );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Cannot start special run 'baseline_history' for MCA '%s'.  "
		"Error code = %d, '%s'", 
				mca->record->name,
				xia_status,
				mxi_handel_strerror( xia_status ) );
	}

	MX_DEBUG(-2,("%s: getting baseline history array for MCA '%s'.",
		fname, mca->record->name ));

	/* Finally, we can now read the baseline history array. */

	void_ptr = (void *) &(handel_mca->baseline_history_array[0]);

	xia_status = xiaGetSpecialRunData( handel_mca->detector_channel,
					"baseline_history", void_ptr );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"Cannot get the baseline history array for MCA '%s'.  "
			"Error code = %d, '%s'", 
					mca->record->name,
					xia_status,
					mxi_handel_strerror( xia_status ) );
	}

#if 1
	{
		unsigned long i;

		fprintf(stderr, "Start of baseline history array:\n");

		for ( i = 0; i < 100; i++ ) {
			fprintf( stderr, "%lu ",
				handel_mca->baseline_history_array[i] );
		}

		fprintf(stderr, "\nEnd of baseline history array\n");
	}
#endif

	/* If everything worked, we must change the dimension value in
	 * the record field.
	 */

	mx_status = mx_find_record_field( mca->record,
					"baseline_history_array", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = handel_mca->baseline_history_length;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_handel_get_mx_parameter( MX_MCA *mca )
{
	static const char fname[] = "mxi_handel_get_mx_parameter()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL *handel;
	char acquisition_value_name[MAXALIAS_LEN+1];
	double acquisition_value;
	long i, handel_preset_type;
	int xia_status;
	mx_status_type mx_status;

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxi_handel_get_pointers( mca,
					&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_HANDEL_DEBUG		
	MX_DEBUG(-2,("%s invoked for MCA '%s', parameter type '%s' (%d).",
		fname, mca->record->name,
		mx_get_field_label_string(mca->record,mca->parameter_type),
		mca->parameter_type));
#endif

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	switch( mca->parameter_type ) {
	case MXLV_MCA_CHANNEL_NUMBER:

		/* These items are stored in memory and are not retrieved
		 * from the hardware.
		 */

		break;

	case MXLV_MCA_CURRENT_NUM_CHANNELS:
		mx_status = mxi_handel_get_acquisition_values( mca,
				"number_mca_channels", &acquisition_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mca->current_num_channels = mx_round( acquisition_value );
		break;

	case MXLV_MCA_PRESET_TYPE:
		mx_status = mxi_handel_get_acquisition_values( mca,
				"preset_type", &acquisition_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		handel_preset_type = mx_round( acquisition_value );

		switch( handel_preset_type ) {
		case MXF_HANDEL_MCA_PRESET_NONE:
			mca->preset_type = MXF_MCA_PRESET_NONE;
			break;
		case MXF_HANDEL_MCA_PRESET_REAL_TIME:
			mca->preset_type = MXF_MCA_PRESET_REAL_TIME;
			break;
		case MXF_HANDEL_MCA_PRESET_LIVE_TIME:
			mca->preset_type = MXF_MCA_PRESET_LIVE_TIME;
			break;
		case MXF_HANDEL_MCA_PRESET_OUTPUT_EVENTS:
		case MXF_HANDEL_MCA_PRESET_INPUT_COUNTS:
			return mx_error( MXE_UNSUPPORTED, fname,
				"Handel preset type %ld is not currently "
				"supported for MCA '%s'.", 
					handel_preset_type,
					mca->record->name );
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"Unrecognized Handel preset type %ld "
				"returned by MCA '%s'.",
					handel_preset_type,
					mca->record->name );
			break;
		}
		break;

	case MXLV_MCA_PRESET_REAL_TIME:
		if ( mca->preset_type != MXLV_MCA_PRESET_REAL_TIME ) {

			mca->preset_type    = MXF_MCA_PRESET_REAL_TIME;
			mca->parameter_type = MXLV_MCA_PRESET_TYPE;

			mx_status = mxi_handel_set_mx_parameter( mca );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		mx_status = mxi_handel_get_acquisition_values( mca,
				"preset_value", &(mca->preset_real_time) );
		break;

	case MXLV_MCA_PRESET_LIVE_TIME:
		if ( mca->preset_type != MXLV_MCA_PRESET_LIVE_TIME ) {

			mca->preset_type    = MXF_MCA_PRESET_LIVE_TIME;
			mca->parameter_type = MXLV_MCA_PRESET_TYPE;

			mx_status = mxi_handel_set_mx_parameter( mca );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		mx_status = mxi_handel_get_acquisition_values( mca,
				"preset_value", &(mca->preset_live_time) );
		break;

	case MXLV_MCA_PRESET_COUNT:
		if ( mca->preset_type != MXLV_MCA_PRESET_COUNT ) {

			mca->preset_type    = MXF_MCA_PRESET_COUNT;
			mca->parameter_type = MXLV_MCA_PRESET_TYPE;

			mx_status = mxi_handel_set_mx_parameter( mca );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		mx_status = mxi_handel_get_acquisition_values( mca,
				"preset_value", &acquisition_value );

		mca->preset_count = mx_round( acquisition_value );
		break;

	case MXLV_MCA_CHANNEL_VALUE:
		mca->channel_value = mca->channel_array[ mca->channel_number ];
		break;

	case MXLV_MCA_ROI_ARRAY:
		for ( i = 0; i < mca->current_num_rois; i++ ) {
		    if ( handel_mca->sca_has_been_initialized[i] == FALSE ) {

			/* xiaGetAcquisitionValues() will fail if the SCA
			 * has not been initialized, so we just report 0-0.
			 */

			mca->roi_array[i][0] = 0;
			mca->roi_array[i][1] = 0;

		    } else {
			if ( handel_mca->hardware_scas_are_enabled ) {
				sprintf( acquisition_value_name,
						"sca%ld_lo", i );
			} else {
				sprintf( acquisition_value_name,
						"SCA%ldLO", i );
			}
			
			xia_status = xiaGetAcquisitionValues(
				handel_mca->detector_channel,
				acquisition_value_name,
				(void *) &acquisition_value );

			if ( xia_status != XIA_SUCCESS ) {
				return mx_error(
				    MXE_INTERFACE_ACTION_FAILED, fname,
			"Cannot read the value '%s' from MCA '%s'.  "
			"Error code = %d, '%s'", acquisition_value_name,
					mca->record->name, xia_status,
					mxi_handel_strerror( xia_status ) );
			}

			mca->roi_array[i][0] = mx_round( acquisition_value );

			if ( handel_mca->hardware_scas_are_enabled ) {
				sprintf( acquisition_value_name,
						"sca%ld_hi", i );
			} else {
				sprintf( acquisition_value_name,
						"SCA%ldHI", i );
			}
			
			xia_status = xiaGetAcquisitionValues(
				handel_mca->detector_channel,
				acquisition_value_name,
				(void *) &acquisition_value );

			if ( xia_status != XIA_SUCCESS ) {
				return mx_error(
				    MXE_INTERFACE_ACTION_FAILED, fname,
			"Cannot read the value '%s' from MCA '%s'.  "
			"Error code = %d, '%s'", acquisition_value_name,
					mca->record->name, xia_status,
					mxi_handel_strerror( xia_status ) );
			}

			mca->roi_array[i][1] = mx_round( acquisition_value );
		    }
		}
		break;

	case MXLV_MCA_ROI_INTEGRAL_ARRAY:
		if ( handel_mca->hardware_scas_are_enabled ) {
			/* This system supports SCA integrals computed
			 * by the firmware.
			 */

			xia_status = xiaGetRunData(
				handel_mca->detector_channel,
				"sca", (void *) mca->roi_integral_array );

			if ( xia_status != XIA_SUCCESS ) {
				return mx_error(
				    MXE_INTERFACE_ACTION_FAILED, fname,
			"Cannot read the region of interest integrals "
			"from MCA '%s'.  Error code = %d, '%s'",
					mca->record->name, xia_status,
					mxi_handel_strerror( xia_status ) );
			}
		} else {
			/* Pass this on to the default code. */

			return mxd_handel_mca_default_get_mx_parameter( mca );
		}
		break;

	case MXLV_MCA_ROI:
		i = mca->roi_number;

		if ( handel_mca->sca_has_been_initialized[i] == FALSE ) {

		    /* xiaGetAcquisitionValues() will fail if the SCA
		     * has not been initialized, so we just report 0-0.
		     */

		    mca->roi[0] = 0;
		    mca->roi[1] = 0;

		    mca->roi_array[i][0] = 0;
		    mca->roi_array[i][1] = 0;
		} else {

		    if ( handel_mca->hardware_scas_are_enabled ) {
			sprintf( acquisition_value_name, "sca%ld_lo", i );
		    } else {
			sprintf( acquisition_value_name, "SCA%ldLO", i );
		    }
			
		    xia_status = xiaGetAcquisitionValues(
				handel_mca->detector_channel,
				acquisition_value_name,
				(void *) &acquisition_value );

		    if ( xia_status != XIA_SUCCESS ) {
			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"Cannot read the value '%s' from MCA '%s'.  "
			"Error code = %d, '%s'", acquisition_value_name,
				mca->record->name, xia_status,
				mxi_handel_strerror( xia_status ) );
		    }

#if MXI_HANDEL_DEBUG		
		    MX_DEBUG(-2,
		("%s: acquisition_value_name = '%s', acquisition_value = %g",
			fname, acquisition_value_name, acquisition_value));
#endif
			
		    mca->roi[0] = mx_round( acquisition_value );

		    mca->roi_array[i][0] = mca->roi[0];

		    if ( handel_mca->hardware_scas_are_enabled ) {
			sprintf( acquisition_value_name, "sca%ld_hi", i );
		    } else {
			sprintf( acquisition_value_name, "SCA%ldHI", i );
		    }
			
		    xia_status = xiaGetAcquisitionValues(
				handel_mca->detector_channel,
				acquisition_value_name,
				(void *) &acquisition_value );

		    if ( xia_status != XIA_SUCCESS ) {
			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"Cannot read the value '%s' from MCA '%s'.  "
			"Error code = %d, '%s'", acquisition_value_name,
				mca->record->name, xia_status,
				mxi_handel_strerror( xia_status ) );
		    }

#if MXI_HANDEL_DEBUG		
		    MX_DEBUG(-2,
		("%s: acquisition_value_name = '%s', acquisition_value = %g",
			fname, acquisition_value_name, acquisition_value));
#endif
			
		    mca->roi[1] = mx_round( acquisition_value );

		    mca->roi_array[i][1] = mca->roi[1];
		}
		break;
	
	case MXLV_MCA_ROI_INTEGRAL:
		if ( handel_mca->hardware_scas_are_enabled ) {
			/* This system supports SCA integrals computed
			 * by the firmware.
			 */

			xia_status = xiaGetRunData(
				handel_mca->detector_channel,
				"sca", (void *) mca->roi_integral_array );

			if ( xia_status != XIA_SUCCESS ) {
				return mx_error(
				    MXE_INTERFACE_ACTION_FAILED, fname,
			"Cannot read the region of interest integrals "
			"from MCA '%s'.  Error code = %d, '%s'",
					mca->record->name, xia_status,
					mxi_handel_strerror( xia_status ) );
			}

			mca->roi_integral =
				mca->roi_integral_array[ mca->roi_number ];
		} else {
			/* Pass this on to the default code. */

			return mxd_handel_mca_default_get_mx_parameter( mca );
		}
		break;

	case MXLV_MCA_REAL_TIME:
		xia_status = xiaGetRunData( handel_mca->detector_channel,
			"runtime", (void *) &(mca->real_time) );

		if ( xia_status != XIA_SUCCESS ) {
			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"Cannot read the value '%s' from MCA '%s'.  "
			"Error code = %d, '%s'", acquisition_value_name,
					mca->record->name, xia_status,
					mxi_handel_strerror( xia_status ) );
		}
		break;

	case MXLV_MCA_LIVE_TIME:
		xia_status = xiaGetRunData( handel_mca->detector_channel,
			"trigger_livetime", (void *) &(mca->live_time) );

		if ( xia_status != XIA_SUCCESS ) {
			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"Cannot read the value '%s' from MCA '%s'.  "
			"Error code = %d, '%s'", acquisition_value_name,
					mca->record->name, xia_status,
					mxi_handel_strerror( xia_status ) );
		}
		break;

	case MXLV_MCA_COUNTS:
		xia_status = xiaGetRunData( handel_mca->detector_channel,
			"events_in_run", (void *) &(mca->live_time) );

		if ( xia_status != XIA_SUCCESS ) {
			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"Cannot read the value '%s' from MCA '%s'.  "
			"Error code = %d, '%s'", acquisition_value_name,
					mca->record->name, xia_status,
					mxi_handel_strerror( xia_status ) );
		}
		break;

	default:
		return mx_mca_default_get_parameter_handler( mca );
		break;
	}

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, "parameter type '%s' (%d)",
		mx_get_field_label_string(mca->record, mca->parameter_type),
		mca->parameter_type );
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_handel_set_mx_parameter( MX_MCA *mca )
{
	static const char fname[] = "mxi_handel_set_mx_parameter()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL *handel;
	char acquisition_value_name[MAXALIAS_LEN+1];
	double acquisition_value;
	long i, handel_preset_type;
	int xia_status;
	mx_status_type mx_status;

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxi_handel_get_pointers( mca,
					&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_HANDEL_DEBUG		
	MX_DEBUG(-2,("%s invoked for MCA '%s', parameter type '%s' (%d).",
		fname, mca->record->name,
		mx_get_field_label_string(mca->record,mca->parameter_type),
		mca->parameter_type));
#endif

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	switch( mca->parameter_type ) {
	case MXLV_MCA_CURRENT_NUM_CHANNELS:
		acquisition_value = (double) mca->current_num_channels;

		mx_status = mxi_handel_set_acquisition_values( mca,
				"number_mca_channels", &acquisition_value,
				TRUE );
		break;

	case MXLV_MCA_PRESET_TYPE:
		switch( mca->preset_type ) {
		case MXF_MCA_PRESET_NONE:
			handel_preset_type = MXF_HANDEL_MCA_PRESET_NONE;
			break;
		case MXF_MCA_PRESET_REAL_TIME:
			handel_preset_type = MXF_HANDEL_MCA_PRESET_REAL_TIME;
			break;
		case MXF_MCA_PRESET_LIVE_TIME:
			handel_preset_type = MXF_HANDEL_MCA_PRESET_LIVE_TIME;
			break;
		default:
			return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
				"MX preset type %ld is not yet implemented "
				"for MCA '%s'.",
				mca->preset_type, mca->record->name );
			break;
		}
		acquisition_value = (double) handel_preset_type;

		mx_status = mxi_handel_set_acquisition_values( mca,
				"preset_type", &acquisition_value, TRUE );
		break;

	case MXLV_MCA_PRESET_REAL_TIME:
		if ( mca->preset_type != MXLV_MCA_PRESET_REAL_TIME ) {

			mca->preset_type    = MXF_MCA_PRESET_REAL_TIME;
			mca->parameter_type = MXLV_MCA_PRESET_TYPE;

			mx_status = mxi_handel_set_mx_parameter( mca );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		mx_status = mxi_handel_set_acquisition_values( mca,
			"preset_value", &(mca->preset_real_time), TRUE );
		break;

	case MXLV_MCA_PRESET_LIVE_TIME:
		if ( mca->preset_type != MXLV_MCA_PRESET_LIVE_TIME ) {

			mca->preset_type    = MXF_MCA_PRESET_LIVE_TIME;
			mca->parameter_type = MXLV_MCA_PRESET_TYPE;

			mx_status = mxi_handel_set_mx_parameter( mca );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		mx_status = mxi_handel_set_acquisition_values( mca,
			"preset_value", &(mca->preset_live_time), TRUE );
		break;

	case MXLV_MCA_PRESET_COUNT:
		if ( mca->preset_type != MXLV_MCA_PRESET_COUNT ) {

			mca->preset_type    = MXF_MCA_PRESET_COUNT;
			mca->parameter_type = MXLV_MCA_PRESET_TYPE;

			mx_status = mxi_handel_set_mx_parameter( mca );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		acquisition_value = (double) mca->preset_count;

		mx_status = mxi_handel_set_acquisition_values( mca,
			"preset_value", &acquisition_value, TRUE );
		break;


	case MXLV_MCA_ROI_ARRAY:
		for ( i = 0; i < mca->current_num_rois; i++ ) {
			sprintf( acquisition_value_name, "sca%ld_lo", i );

			acquisition_value = (double) mca->roi_array[i][0];
			
			mx_status = mxi_handel_set_acquisition_values( mca,
					acquisition_value_name,
					&acquisition_value, FALSE );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			sprintf( acquisition_value_name, "sca%ld_hi", i );
			
			acquisition_value = (double) mca->roi_array[i][1];
			
			mx_status = mxi_handel_set_acquisition_values( mca,
					acquisition_value_name,
					&acquisition_value, FALSE );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			handel_mca->sca_has_been_initialized[i] = TRUE;
		}

		mx_status = mxi_handel_apply( mca, -1 );

		break;

	case MXLV_MCA_ROI:
		i = mca->roi_number;

		sprintf( acquisition_value_name, "sca%ld_lo", i );

		acquisition_value = (double) mca->roi[0];

		MX_DEBUG( 2,
		("%s: acquisition_value_name = '%s', acquisition_value = %g",
			fname, acquisition_value_name, acquisition_value));

		mx_status = mxi_handel_set_acquisition_values( mca,
					acquisition_value_name,
					&acquisition_value, FALSE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
			
		mca->roi_array[i][0] = mca->roi[0];

		/*---*/

		sprintf( acquisition_value_name, "sca%ld_hi", i );

		acquisition_value = (double) mca->roi[1];
			
		MX_DEBUG( 2,
		("%s: acquisition_value_name = '%s', acquisition_value = %g",
			fname, acquisition_value_name, acquisition_value));
			
		mx_status = mxi_handel_set_acquisition_values( mca,
					acquisition_value_name,
					&acquisition_value, FALSE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
			
		mca->roi_array[i][1] = mca->roi[1];

		handel_mca->sca_has_been_initialized[i] = TRUE;

		mx_status = mxi_handel_apply( mca, -1 );

		break;

	default:
		return mx_mca_default_set_parameter_handler( mca );
		break;
	}

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, "parameter type '%s' (%d)",
		mx_get_field_label_string(mca->record, mca->parameter_type),
		mca->parameter_type );
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_handel_special_processing_setup( MX_RECORD *record )
{
	static const char fname[] = "mxi_handel_special_processing_setup()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG(2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_HANDEL_CONFIG_FILENAME:
		case MXLV_HANDEL_SAVE_FILENAME:
			record_field->process_function
					    = mxi_handel_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

const char *
mxi_handel_strerror( int handel_status )
{
	static struct {
		int handel_error_code;
		const char handel_error_message[140];
	} error_code_table[] = {
		{   0, "XIA_SUCCESS" },
		{   1, "XIA_OPEN_FILE" },
		{   2, "XIA_FILEERR" },
		{   3, "XIA_NOSECTION" },
		{   4, "XIA_FORMAT_ERROR" },
		{   5, "XIA_ILLEGAL_OPERATION - "
			"Attempted to configure options in an illegal order" },
		{   6, "XIA_FILE_RA - "
			"File random access unable to find name-value pair" },
		{ 201, "XIA_UNKNOWN_DECIMATION - "
	"The decimation read from the hardware does not match a known value" },
		{ 202, "XIA_SLOWLEN_OOR - "
			"Calculated SLOWLEN value is out-of-range" },
		{ 203, "XIA_SLOWGAP_OOR - "
			"Calculated SLOWGAP value is out-of-range" },
		{ 204, "XIA_SLOWFILTER_OOR - "
			"Attempt to set the Peaking or Gap time s.t. P+G>31" },
		{ 205, "XIA_FASTLEN_OOR - "
			"Calculated FASTLEN value is out-of-range" },
		{ 206, "XIA_FASTGAP_OOR - "
			"Calculated FASTGAP value is out-of-range" },
		{ 207, "XIA_FASTFILTER_OOR - "
			"Attempt to set the Peaking or Gap time s.t. P+G>31" },
		{ 301, "XIA_INITIALIZE" },
		{ 302, "XIA_NO_ALIAS" },
		{ 303, "XIA_ALIAS_EXISTS" },
		{ 304, "XIA_BAD_VALUE" },
		{ 305, "XIA_INFINITE_LOOP" },
		{ 306, "XIA_BAD_NAME - Specified name is not valid" },
		{ 307, "XIA_BAD_PTRR - "
			"Specified PTRR is not valid for this alias" },
		{ 308, "XIA_ALIAS_SIZE - Alias name has too many characters" },
		{ 309, "XIA_NO_MODULE - "
			"Must define at least one module before" },
		{ 310, "XIA_BAD_INTERFACE - "
			"The specified interface does not exist" },
		{ 311, "XIA_NO_INTERFACE - "
	"An interface must be defined before more information is specified" },
		{ 312, "XIA_WRONG_INTERFACE - "
	"Specified information doesn't apply to this interface" },
		{ 313, "XIA_NO_CHANNELS - "
			"Number of channels for this module is set to 0" },
		{ 314, "XIA_BAD_CHANNEL - "
			"Specified channel index is invalid or out-of-range" },
		{ 315, "XIA_NO_MODIFY - "
			"Specified name cannot be modified once set" },
		{ 316, "XIA_INVALID_DETCHAN - "
			"Specified detChan value is invalid" },
		{ 317, "XIA_BAD_TYPE - "
			"The DetChanElement type specified is invalid" },
		{ 318, "XIA_WRONG_TYPE - "
		"This routine only operates on detChans that are sets" },
		{ 319, "XIA_UNKNOWN_BOARD - Board type is unknown" },
		{ 320, "XIA_NO_DETCHANS - No detChans are currently defined" },
		{ 321, "XIA_NOT_FOUND - "
			"Unable to locate the Acquisition value requested" },
		{ 322, "XIA_PTR_CHECK - "
			"Pointer is out of synch when it should be valid" },
		{ 323, "XIA_LOOKING_PTRR - "
	"FirmwareSet has a FDD file defined and this only works with PTRRs" },
		{ 324, "XIA_NO_FILENAME - "
			"Requested filename information is set to NULL" },
		{ 325, "XIA_BAD_INDEX - "
			"User specified an alias index that doesn't exist" },
		{ 350, "XIA_FIRM_BOTH - "
"A FirmwareSet may not contain both an FDD and seperate Firmware definitions" },
		{ 351, "XIA_PTR_OVERLAP - "
	"Peaking time ranges in the Firmware definitions may not overlap" },
		{ 352, "XIA_MISSING_FIRM - "
	"Either the FiPPI or DSP file is missing from a Firmware element" },
		{ 353, "XIA_MISSING_POL - "
	"A polarity value is missing from a Detector element" },
		{ 354, "XIA_MISSING_GAIN - "
	"A gain value is missing from a Detector element" },
		{ 355, "XIA_MISSING_INTERFACE - "
	"The interface this channel requires is missing" },
		{ 356, "XIA_MISSING_ADDRESS - "
	"The epp_address information is missing for this channel" },
		{ 357, "XIA_INVALID_NUMCHANS - "
	"The wrong number of channels are assigned to this module" },
		{ 358, "XIA_INCOMPLETE_DEFAULTS - "
	"Some of the required defaults are missing" },
		{ 359, "XIA_BINS_OOR - "
	"There are too many or too few bins for this module type" },
		{ 360, "XIA_MISSING_TYPE - "
	"The type for the current detector is not specified properly" },
		{ 361, "XIA_NO_MMU - "
	"No MMU defined and/or required for this module" },
		{ 401, "XIA_NOMEM - Unable to allocate memory" },
		{ 402, "XIA_XERXES - XerXes returned an error" },
		{ 403, "XIA_MD - MD layer returned an error" },
		{ 404, "XIA_EOF - EOF encountered" },
		{ 405, "XIA_XERXES_NORMAL_RUN_ACTIVE - "
			"XerXes says a normal run is still active" },
		{ 406, "XIA_HARDWARE_RUN_ACTIVE - "
			"The hardware says a control run is still active" },
		{ 501, "XIA_UNKNOWN" },
		{ 502, "XIA_LOG_LEVEL - Log level invalid" },
		{ 503, "XIA_NO_LIST - List size is zero" },
		{ 504, "XIA_NO_ELEM - No data to remove" },
		{ 505, "XIA_DATA_DUP - Data already in table" },
		{ 506, "XIA_REM_ERR - Unable to remove entry from hash table" },
		{ 507, "XIA_FILE_TYPE - Improper file type specified" },
		{ 508, "XIA_END - "
	"There are no more instances of the name specified. Pos set to end" },
		{ 601, "XIA_NOSUPPORT_FIRM - "
	"The specified firmware is not supported by this board type" },
		{ 602, "XIA_UNKNOWN_FIRM - "
			"The specified firmware type is unknown" },
		{ 603, "XIA_NOSUPPORT_VALUE - "
			"The specified acquisition value is not supported" },
		{ 604, "XIA_UNKNOWN_VALUE - "
			"The specified acquisition value is unknown" },
		{ 605, "XIA_PEAKINGTIME_OOR - "
	"The specified peaking time is out-of-range for this product" },
		{ 606, "XIA_NODEFINE_PTRR - "
	"The specified peaking time does not have a PTRR associated with it" },
		{ 607, "XIA_THRESH_OOR - "
			"The specified treshold is out-of-range" },
		{ 608, "XIA_ERROR_CACHE - "
			"The data in the values cache is out-of-sync" },
		{ 609, "XIA_GAIN_OOR - "
			"The specified gain is out-of-range for this product" },
		{ 610, "XIA_TIMEOUT - Timeout waiting for BUSY" },
		{ 611, "XIA_BAD_SPECIAL - "
	"The specified special run is not supported for this module" },
		{ 612, "XIA_TRACE_OOR - "
	"The specified value of tracewait (in ns) is out-of-range" },
		{ 613, "XIA_DEFAULTS - "
	"The PSL layer encountered an error creating a Defaults element" },
		{ 614, "XIA_BAD_FILTER - Error loading filter info "
	"from either a FDD file or the Firmware configuration" },
		{ 615, "XIA_NO_REMOVE - Specified acquisition value "
	"is required for this product and can't be removed" },
		{ 616, "XIA_NO_GAIN_FOUND - "
	"Handel was unable to converge on a stable gain value" },
		{ 617, "XIA_UNDEFINED_RUN_TYPE - "
	"Handel does not recognize this run type" },
		{ 618, "XIA_INTERNAL_BUFFER_OVERRUN - "
	"Handel attempted to overrun an internal buffer boundry" }, 
		{ 619, "XIA_EVENT_BUFFER_OVERRUN - "
	"Handel attempted to overrun the event buffer boundry" }, 
		{ 620, "XIA_BAD_DATA_LENGTH - "
	"Handel was asked to set a Data length to zero for readout" },
		{ 621, "XIA_NO_LINEAR_FIT - "
	"Handel was unable to perform a linear fit to some data" },
		{ 622, "XIA_MISSING_PTRR - Required PTRR is missing" },
		{ 623, "XIA_PARSE_DSP - Error parsing DSP" },
		{ 624, "XIA_UDXPS", },
		{ 625, "XIA_BIN_WIDTH - Specified bin width is out-of-range" },
		{ 626, "XIA_NO_VGA - "
"An attempt was made to set the gaindac on a board that doesn't have a VGA" },
		{ 627, "XIA_TYPEVAL_OOR - "
	"Specified detector type value is out-of-range" },
		{ 628, "XIA_LOW_LIMIT_OOR - "
	"Specified low MCA limit is out-of-range" },
		{ 629, "XIA_BPB_OOR - bytes_per_bin is out-of-range" },
		{ 630, "XIA_FIP_OOR - Specified FiPPI is out-fo-range" },
		{ 701, "XIA_XUP_VERSION - XUP version is not supported" },
		{ 702, "XIA_CHKSUM - checksum mismatch in the XUP" },
		{ 703, "XIA_BAK_MISSING - Requested BAK file cannot be opened"},
		{ 704, "XIA_SIZE_MISMATCH - "
			"Size read from file is incorrect" },
		{ 1001,"XIA_DEBUG" }
	};

	static const char unrecognized_error_message[]
		= "Unrecognized Handel error code";

	static int num_error_codes = sizeof( error_code_table )
				/ sizeof( error_code_table[0] );

	unsigned int i;

	for ( i = 0; i < num_error_codes; i++ ) {

		if ( handel_status == error_code_table[i].handel_error_code )
			break;		/* Exit the for() loop. */
	}

	if ( i >= num_error_codes ) {
		return &unrecognized_error_message[0];
	}

	return &( error_code_table[i].handel_error_message[0] );
}

#ifndef MX_PROCESS_GET

#define MX_PROCESS_GET	1
#define MX_PROCESS_PUT	2

#endif

static mx_status_type
mxi_handel_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mxi_handel_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_HANDEL *handel;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	handel = (MX_HANDEL *) (record->record_type_struct);

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_HANDEL_CONFIG_FILENAME:
			/* Nothing to do since the necessary filename is
			 * already stored in the 'config_filename' field.
			 */

			break;
		case MXLV_HANDEL_SAVE_FILENAME:
			/* Nothing to do since the necessary filename is
			 * already stored in the 'save_filename' field.
			 */

			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_GET label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		case MXLV_HANDEL_CONFIG_FILENAME:
			mx_status = mxi_handel_load_new_config(handel);

			break;
		case MXLV_HANDEL_SAVE_FILENAME:
			mx_status = mxi_handel_save_config(handel);

			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_PUT label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown operation code = %d", operation );
		break;
	}

	return mx_status;
}

#endif /* HAVE_XIA_HANDEL */
