/*
 * Name:    d_handel_mca.c
 *
 * Purpose: MX multichannel analyzer driver for the X-Ray Instrumentation
 *          Associates Handel library.  This driver controls an individual
 *          MCA in a multi-MCA module.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2001-2006, 2008-2012, 2015-2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_HANDEL_MCA_DEBUG			FALSE

#define MXD_HANDEL_MCA_DEBUG_STATISTICS		FALSE

#define MXD_HANDEL_MCA_DEBUG_TIMING		FALSE

#define MXD_HANDEL_MCA_DEBUG_DOUBLE_ROIS	FALSE

#define MXD_HANDEL_MCA_DEBUG_GET_PRESETS	FALSE

#define MXD_HANDEL_MCA_DEBUG_MCA_RECORD_ARRAY	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <float.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_hrt.h"
#include "mx_array.h"
#include "mx_socket.h"
#include "mx_net.h"
#include "mx_mutex.h"
#include "mx_thread.h"
#include "mx_mca.h"
#include "mx_mcs.h"

#include <handel.h>
#include <handel_errors.h>
#include <handel_generic.h>

#include "i_handel.h"
#include "d_handel_mca.h"
#include "d_handel_mcs.h"

#if MXD_HANDEL_MCA_DEBUG_TIMING
#  include "mx_hrt_debug.h"
#endif

/* Initialize the MCA driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_handel_mca_record_function_list = {
	mxd_handel_mca_initialize_driver,
	mxd_handel_mca_create_record_structures,
	mxd_handel_mca_finish_record_initialization,
	NULL,
	mxd_handel_mca_print_structure,
	mxd_handel_mca_open,
	mxd_handel_mca_close,
	NULL,
	NULL,
	mxd_handel_mca_special_processing_setup
};

MX_MCA_FUNCTION_LIST mxd_handel_mca_mca_function_list = {
	mxd_handel_mca_arm,
	mxd_handel_mca_trigger,
	mxd_handel_mca_stop,
	mxd_handel_mca_read,
	mxd_handel_mca_clear,
	mxd_handel_mca_busy,
	mxd_handel_mca_get_parameter,
	mxd_handel_mca_set_parameter,
};

MX_RECORD_FIELD_DEFAULTS mxd_handel_mca_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_MCA_STANDARD_FIELDS,
	MXD_HANDEL_MCA_STANDARD_FIELDS
};

#if MXD_HANDEL_MCA_DEBUG_STATISTICS
#   define HANDEL_MCA_DEBUG_STATISTICS(x) \
	MX_DEBUG(-2,("%s: MCA '%s', new_statistics_available = %d", \
		fname, (x)->record->name, (x)->new_statistics_available));
#else
#   define HANDEL_MCA_DEBUG_STATISTICS(x)
#endif

long mxd_handel_mca_num_record_fields
		= sizeof( mxd_handel_mca_record_field_defaults )
		  / sizeof( mxd_handel_mca_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_handel_mca_rfield_def_ptr
			= &mxd_handel_mca_record_field_defaults[0];

static mx_status_type mxd_handel_mca_process_function( void *record_ptr,
						void *record_field_ptr,
						void *socket_handler_ptr,
						int operation );

/* A private function for the use of the driver. */

static mx_status_type
mxd_handel_mca_get_pointers( MX_MCA *mca,
			MX_HANDEL_MCA **handel_mca,
			MX_HANDEL **handel,
			const char *calling_fname )
{
	static const char fname[] = "mxd_handel_mca_get_pointers()";

	MX_RECORD *mca_record, *handel_record;
	MX_HANDEL_MCA *handel_mca_ptr;

	if ( mca == (MX_MCA *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MCA pointer passed by '%s' was NULL.",
			calling_fname );
	}

	mca_record = mca->record;

	if ( mca_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_RECORD pointer for the MX_MCA pointer passed by '%s' is NULL.",
			calling_fname );
	}

	handel_mca_ptr = (MX_HANDEL_MCA *) mca_record->record_type_struct;

	if ( handel_mca_ptr == (MX_HANDEL_MCA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_HANDEL_MCA pointer for MCA record '%s' is NULL.",
			mca_record->name );
	}

	if ( handel_mca != (MX_HANDEL_MCA **) NULL ) {
		*handel_mca = handel_mca_ptr;
	}

	if ( handel != (MX_HANDEL **) NULL ) {
		handel_record = handel_mca_ptr->handel_record;

		if ( handel_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The handel_record pointer for record '%s' is NULL.",
				mca_record->name );
		}

		*handel = (MX_HANDEL *) handel_record->record_type_struct;

		if ( (*handel) == (MX_HANDEL *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_HANDEL pointer for Handel record '%s' is NULL.",
				handel_record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_handel_mca_initialize_driver( MX_DRIVER *driver )
{
	long maximum_num_channels_varargs_cookie;
	long maximum_num_rois_varargs_cookie;
	long num_soft_rois_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_mca_initialize_driver( driver,
				&maximum_num_channels_varargs_cookie,
				&maximum_num_rois_varargs_cookie,
				&num_soft_rois_varargs_cookie );

	return mx_status;
}


MX_EXPORT mx_status_type
mxd_handel_mca_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_handel_mca_create_record_structures()";

	MX_MCA *mca;
	MX_HANDEL_MCA *handel_mca = NULL;
	int i;


	/* Allocate memory for the necessary structures. */

	mca = (MX_MCA *) malloc( sizeof(MX_MCA) );

	if ( mca == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MCA structure." );
	}

	handel_mca = (MX_HANDEL_MCA *) malloc( sizeof(MX_HANDEL_MCA) );

	if ( handel_mca == (MX_HANDEL_MCA *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_HANDEL_MCA structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = mca;
	record->record_type_struct = handel_mca;
	record->class_specific_function_list =
				&mxd_handel_mca_mca_function_list;

	mca->record = record;
	handel_mca->record = record;

	handel_mca->child_mcs_record = NULL;

	handel_mca->adc_trace_step_size = 5000.0;	/* in nanoseconds */
	handel_mca->adc_trace_length = 0;
	handel_mca->adc_trace_array = NULL;

	handel_mca->baseline_history_length = 0;
	handel_mca->baseline_history_array = NULL;

	handel_mca->mca_record_array_index = -1;

	handel_mca->firmware_code_variant = (unsigned long) -1;
	handel_mca->firmware_code_revision = (unsigned long) -1;
	handel_mca->hardware_scas_are_enabled = FALSE;

	handel_mca->has_mapping_firmware = FALSE;

	for ( i = 0; i < MX_HANDEL_MCA_MAX_SCAS; i++ ) {
		handel_mca->sca_has_been_initialized[i] = FALSE;
	}

	handel_mca->acquisition_value_name[0] = '\0';
	handel_mca->acquisition_value = 0;
	handel_mca->acquisition_value_to_all = 0;
	handel_mca->apply_flag = FALSE;
	handel_mca->apply_to_all = FALSE;

	handel_mca->use_double_roi_integral_array = FALSE;
	handel_mca->double_roi_integral_array = NULL;

#if MXD_HANDEL_MCA_DEBUG
	handel_mca->debug_flag = TRUE;
#else
	handel_mca->debug_flag = FALSE;
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_handel_mca_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_handel_mca_finish_record_initialization()";

	MX_MCA *mca;
	MX_HANDEL_MCA *handel_mca = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed is NULL." );
	}

	mca = (MX_MCA *) record->record_class_struct;

	if ( mca == (MX_MCA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MCA pointer for record '%s' is NULL.", record->name );
	}

	if ( mca->maximum_num_rois > MX_HANDEL_MCA_MAX_SCAS ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
"The requested maximum number of ROIs (%ld) for XIA MCA '%s' is greater "
"than the maximum allowed value of %d.",
			mca->maximum_num_rois, record->name,
			MX_HANDEL_MCA_MAX_SCAS );
	}

	if ( mca->maximum_num_channels > MX_HANDEL_MCA_MAX_BINS ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
"The requested maximum number of channels (%ld) for XIA MCA '%s' is greater "
"than the maximum allowed value of %d.",
			mca->maximum_num_channels, record->name,
			MX_HANDEL_MCA_MAX_BINS );
	}

	mca->channel_array = ( unsigned long * )
		malloc( mca->maximum_num_channels * sizeof( unsigned long ) );

	if ( mca->channel_array == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory allocating an %ld channel data array.",
			mca->maximum_num_channels );
	}

	mca->preset_type = MXF_MCA_PRESET_LIVE_TIME;

	handel_mca = (MX_HANDEL_MCA *) record->record_type_struct;

	handel_mca->new_statistics_available = TRUE;

	HANDEL_MCA_DEBUG_STATISTICS( handel_mca );

	/* Do generic MCA record initialization. */

	mx_status = mx_mca_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_handel_mca_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_handel_mca_print_structure()";

	MX_MCA *mca;
	MX_HANDEL_MCA *handel_mca = NULL;
	MX_HANDEL *handel = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	mca = (MX_MCA *) (record->record_class_struct);

	mx_status = mxd_handel_mca_get_pointers( mca,
						&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MCA parameters for record '%s':\n", record->name);

	fprintf(file, "  MCA type              = HANDEL_MCA.\n\n");
	fprintf(file, "  XIA DXP record        = '%s'\n",
					handel_mca->handel_record->name);
	fprintf(file, "  mca label             = %s\n",
					handel_mca->mca_label);
	fprintf(file, "  maximum # of bins     = %ld\n",
					mca->maximum_num_channels);

	return MX_SUCCESSFUL_RESULT;
}

/* The following routine does Handel specific initialization. */

static mx_status_type
mxd_handel_mca_handel_open( MX_MCA *mca,
			MX_HANDEL_MCA *handel_mca,
			MX_RECORD *handel_record )
{
	static const char fname[] = "mxd_handel_mca_handel_open()";

	MX_HANDEL *handel = NULL;
	char mca_label_format[40];
	char *mca_label_copy, *ptr;
	char item_name[40];
	unsigned long ulong_value;
	int xia_status, num_items, display_config;
	unsigned long i, j;
	long module_channel, detector_channel_alt;
	mx_status_type mx_status;

#if MX_IGNORE_XIA_NULL_STRING
	char *ignore_this_pointer;
#endif

#if MXD_HANDEL_MCA_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

#if MXD_HANDEL_MCA_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	/* The following statement exist only to suppress the GCC warning
	 * "warning: `XIA_NULL_STRING' defined but not used".  You should
	 * not actually use the variable 'ignore_this_pointer' for anything.
	 */

#if MX_IGNORE_XIA_NULL_STRING
	ignore_this_pointer = XIA_NULL_STRING;
#endif

	if ( handel_mca->debug_flag ) {
		MX_DEBUG(-2,("%s invoked for mca '%s'",
			fname, mca->record->name ));
	}

	handel = (MX_HANDEL *) handel_record->record_type_struct;

	if ( handel->num_mcas == 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The record '%s' used by MCA '%s' has not yet been configured.  "
	"You should verify that the entry for MCA '%s' is listed in the "
	"MX database file _after_ the entry for '%s'.",
			handel_record->name, mca->record->name,
			mca->record->name, handel_record->name );
	}

	display_config = handel->handel_flags &
			MXF_HANDEL_DISPLAY_CONFIGURATION_AT_STARTUP;

	/* Allocate memory for the detector and module aliases. */

	handel_mca->detector_alias = (char *)
			malloc( (MAXALIAS_LEN+1) * sizeof(char) );

	if ( handel_mca->detector_alias == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate a module alias string of %d chars.",
			MAXALIAS_LEN+1 );
	}

	handel_mca->module_alias = (char *)
			malloc( (MAXALIAS_LEN+1) * sizeof(char) );

	if ( handel_mca->module_alias == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate a module alias string of %d chars.",
			MAXALIAS_LEN+1 );
	}

	handel_mca->module_type = (char *)
			malloc( (MAXALIAS_LEN+1) * sizeof(char) );

	if ( handel_mca->module_type == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate a module type string of %d chars.",
			MAXALIAS_LEN+1 );
	}

	/*-------------------------------------------------------------------*/

	/* Split up the mca_label into the module alias, detector alias
	 * and detector channel number.
	 */

	/* The detector channel number should be at the end of the string
	 * preceded by a ':' character.
	 */

	mca_label_copy = strdup( handel_mca->mca_label );

	if ( mca_label_copy == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to duplicate the MCA label "
		"for MCA '%s'.", handel_mca->record->name );
	}

	ptr = strrchr( mca_label_copy, ':' );

	if ( ptr == NULL ) {
		mx_status = mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Did not find the detector channel number at the end "
		"of the MCA label '%s' for MCA '%s'.  It should be "
		"preceded by a colon ':' character.",
			mca_label_copy, mca->record->name );

		mx_free( mca_label_copy );

		return mx_status;
	}

	/* Split the string at the ':' character and interpret the
	 * bytes after that as the detector channel number.
	 */

	*ptr = '\0';

	ptr++;

	handel_mca->detector_channel = mx_string_to_long( ptr );

	snprintf( mca_label_format, sizeof(mca_label_format),
		"%%%ds %%%ds", MAXALIAS_LEN, MAXALIAS_LEN );

	num_items = sscanf( mca_label_copy,
				mca_label_format,
				handel_mca->module_alias,
				handel_mca->detector_alias );

	mx_free( mca_label_copy );

	if ( num_items != 2 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Could not get the module alias and detector number from "
		"the 'mca_label' field '%s' for MCA '%s'.  For Handel, a "
		"valid 'mca_label' field should look something like "
		"\"module1 detector1:0\".",
			handel_mca->mca_label, mca->record->name );
	}

	num_items = sscanf( handel_mca->module_alias,
				"module%lu", &(handel_mca->module_number) );

	if ( num_items != 1 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Could not get the module number from the module alias '%s' "
		"for MCA '%s'.",  handel_mca->module_alias,
				handel_mca->record->name );
	}
			
	if ( display_config ) {
		mx_info( "MCA '%s': module alias = '%s', module_number = %ld, "
		"detector alias = '%s', detector channel = %ld",
			handel_mca->record->name,
			handel_mca->module_alias,
			handel_mca->module_number,
			handel_mca->detector_alias,
			handel_mca->detector_channel );
	}

	/*-------------------------------------------------------------------*/

	/* Get the module type. */

	MX_XIA_SYNC( xiaGetModuleItem( handel_mca->module_alias,
					"module_type",
					handel_mca->module_type ) );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot get the module type for module '%s', "
		"detector '%s' channel %ld used by MCA '%s'.  "
		"Error code = %d, '%s'",
			handel_mca->module_alias,
			handel_mca->detector_alias,
			handel_mca->detector_channel,
			mca->record->name, xia_status,
			mxi_handel_strerror( xia_status ) );
	}

	if ( display_config ) {
		mx_info( "MCA '%s': module_type = '%s'",
			handel_mca->record->name,
			handel_mca->module_type );
	}

	/* Verify that the detector channel matches the channel alias. */

	module_channel = handel_mca->detector_channel % handel->mcas_per_module;

	snprintf( item_name, sizeof(item_name),
		"channel%ld_alias", module_channel );

	MX_XIA_SYNC( xiaGetModuleItem( handel_mca->module_alias,
					item_name,
					&detector_channel_alt ) );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot get the detector channel id for module '%s', "
		"detector channel %ld used by MCA '%s'.  Error code = %d, '%s'",
			handel_mca->module_alias,
			handel_mca->detector_channel,
			mca->record->name, xia_status,
			mxi_handel_strerror( xia_status ) );
	}

	if ( detector_channel_alt != handel_mca->detector_channel ) {
		return mx_error( MXE_SOFTWARE_CONFIGURATION_ERROR, fname,
		"The detector channel number %ld from xiaGetModuleItem() "
		"does not match the detector channel number %ld from "
		"the MX database file for MCA '%s'.",
			detector_channel_alt,
			handel_mca->detector_channel,
			mca->record->name );
	}

#if 0
	if ( display_config ) {
		mx_info(
		"MCA '%s': det. alias = '%s', det. channel = %d",
			handel_mca->record->name,
			handel_mca->detector_alias,
			handel_mca->detector_channel );
	}
#endif

	/* Since we do not know what state the MCA is in, we send
	 * a stop run command.
	 */

	MX_XIA_SYNC( xiaStopRun( handel_mca->detector_channel ) );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot stop data acquisition for XIA DXP MCA '%s'.  "
		"Error code = %d, '%s'", handel_mca->record->name,
			xia_status, mxi_handel_strerror( xia_status ) );
	}

	handel_mca->hardware_scas_are_enabled = TRUE;

	/* Search for an empty slot in the MCA array. */

	for ( i = 0; i < handel->num_mcas; i++ ) {

		if ( handel->mca_record_array[i] == NULL ) {

			handel_mca->mca_record_array_index = i;

			handel->mca_record_array[i] = mca->record;

			handel_mca->dxp_module = -1;
			handel_mca->crate = -1;
			handel_mca->slot = -1;
			handel_mca->dxp_channel = -1;

			break;		/* Exit the for() loop. */
		}
	}

	if ( i >= handel->num_mcas ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"There are too many MCA records for Handel record '%s'.  "
		"Handel record '%s' says that there are %ld MCAs, but "
		"MCA record '%s' would be MCA number %ld.",
			handel->record->name,
			handel->record->name,
			handel->num_mcas,
			handel_mca->record->name,
			i+1 );
	}

#if MXD_HANDEL_MCA_DEBUG_MCA_RECORD_ARRAY
	MX_DEBUG(-2,("%s: handel_mca = '%s', handel_mca->record = %p",
		fname, handel_mca->record->name, handel_mca->record));
	MX_DEBUG(-2,("%s: handel->mca_record_array = %p",
		fname, handel->mca_record_array));
	MX_DEBUG(-2,("%s: handel->mca_record_array[%lu] = %p",
		fname, i, handel->mca_record_array[i] ));
#endif

	/* Search for an empty slot in the modules array. */

	if ( ( handel_mca->module_number < 1 )
	  || ( handel_mca->module_number > handel->num_modules ) )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Module number %lu for Handel MCA '%s' is outside "
		"the allowed range of 1 to %lu",
			handel_mca->module_number,
			mca->record->name,
			handel->num_modules );
	}

	if ( handel->module_array == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The module_array pointer for Handel interface '%s' "
		"has not been initialized.", handel->record->name );
	}

	i = handel_mca->module_number - 1;

	for ( j = 0; j < handel->mcas_per_module; j++ ) {
		if ( handel->module_array[i][j] == NULL ) {
			handel->module_array[i][j] = mca->record;
			break;
		}
	}

	if ( j >= handel->mcas_per_module ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"Handel record '%s', module number %lu has too many "
		"records (%lu).  The maximum allowed is %ld.",
			handel->record->name,
			handel_mca->module_number, j,
			handel->mcas_per_module );
	}

#if MXD_HANDEL_MCA_DEBUG_MCA_RECORD_ARRAY
	MX_DEBUG(-2,("%s: handel_mca->module_number = %lu",
		fname, handel_mca->module_number));
	MX_DEBUG(-2,("%s: handel->module_array[%lu][%lu] = %p",
		fname, i, j, handel->module_array[i][j]));
#endif

	/* See how many bins are in the spectrum. */

	MX_XIA_SYNC( xiaGetRunData( handel_mca->detector_channel,
					"mca_length", &ulong_value ) );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Unable to get the number of spectrum bins for MCA '%s'.  "
		"Error code = %d, error status = '%s'",
			mca->record->name,
			xia_status, mxi_handel_strerror( xia_status ) );
	}

	handel_mca->num_spectrum_bins = (unsigned int) ulong_value;

	if ( handel_mca->debug_flag ) {
		MX_DEBUG(-2,("%s: MCA '%s': num_spectrum_bins = %u",
		fname, mca->record->name, handel_mca->num_spectrum_bins));
	}

	/* Do we need to allocate memory for a spectrum array or is
	 * mca->channel_array already the right size?
	 */

	if ( ( sizeof(unsigned long) == 4 )
	  && ( mca->maximum_num_channels >= handel_mca->num_spectrum_bins ) )
	{
		/* We do not need to allocate a spectrum array since
		 * mca->channel_array is already big enough and of the
		 * right data type to hold the data.  This allows us
		 * to skip the step of copying the data from the array
		 * handel_mca->spectrum_array to mca->channel_array.
		 */

		handel_mca->use_mca_channel_array = TRUE;

		handel_mca->spectrum_array = NULL;
	} else {
		/* We _do_ need to allocate a separate spectrum array.
		 * This will happen for one of two reasons:
		 *
		 * 1.  Either the value handel_mca->num_spectrum_bins
		 *     is bigger than mca->maximum_num_channels which
		 *     means that the spectrum returned by XIA would
		 *     not fit into mca->channel_array.
		 *
		 * 2.  We are running on a computer where unsigned longs
		 *     are not 32 bits, such as on an Alpha.
		 *
		 * Either way, we have to allocate a separate array
		 * and put up with the performance hit of copying the
		 * array each time.
		 */

		handel_mca->use_mca_channel_array = FALSE;

		handel_mca->spectrum_array = (unsigned long *) malloc(
		    handel_mca->num_spectrum_bins * sizeof(unsigned long) );

		if ( handel_mca->spectrum_array == (unsigned long *) NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocated a DXP spectrum array of %u unsigned longs",
			handel_mca->num_spectrum_bins );
		}
	}

	/* Allocate memory for the baseline array. */

	MX_XIA_SYNC( xiaGetRunData( handel_mca->detector_channel,
					"baseline_length", &ulong_value ) );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Unable to get the number of baseline bins for MCA '%s'.  "
		"Error code = %d, error status = '%s'",
			mca->record->name,
			xia_status, mxi_handel_strerror( xia_status ) );
	}

	handel_mca->baseline_length = (unsigned int) ulong_value;

	handel_mca->baseline_array = (unsigned long *)
	    malloc( handel_mca->baseline_length * sizeof(unsigned long) );

	if ( handel_mca->baseline_array == (unsigned long *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate a DXP baseline array of %lu unsigned longs",
			handel_mca->baseline_length );
	}

	if ( handel_mca->debug_flag ) {
		MX_DEBUG(-2,("%s complete for MCA '%s'",
			fname, mca->record->name));
	}

#if MXD_HANDEL_MCA_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, "for MCA '%s'", mca->record->name );
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_handel_mca_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_handel_mca_open()";

	MX_MCA *mca;
	MX_HANDEL_MCA *handel_mca = NULL;
	MX_HANDEL *handel = NULL;
	unsigned long i;
	double mapping_mode;
#if 0
	unsigned long codevar, coderev;
#endif
	int xia_status;
	mx_status_type mx_status;

#if MXD_HANDEL_MCA_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif
	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	mca = (MX_MCA *) (record->record_class_struct);

	mx_status = mxd_handel_mca_get_pointers( mca,
						&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( handel_mca->debug_flag ) {
		MX_DEBUG(-2,("%s: debugging enabled for MCA '%s'",
			fname, record->name ));
	}

	mca->trigger_mode = MXF_DEV_INTERNAL_TRIGGER;

	mx_status = mxd_handel_mca_handel_open(mca, handel_mca, handel->record);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0
	display_config = handel->handel_flags &
			MXF_HANDEL_DISPLAY_CONFIGURATION_AT_STARTUP;
#endif

	/* Does this MCA have mapping firmware installed.  We attempt to
	 * detect this at runtime by reading the value of 'mapping_mode'
	 * to see if it exists.
	 */

	mapping_mode = 0.0;

	MX_XIA_SYNC( xiaGetAcquisitionValues( handel_mca->detector_channel,
					"mapping_mode", &mapping_mode ) );

	switch( xia_status ) {
	case XIA_SUCCESS:
		handel_mca->has_mapping_firmware = TRUE;
		break;
	default:
		handel_mca->has_mapping_firmware = FALSE;
		break;
	}

#if 1
	MX_DEBUG(-2,("%s: MCA '%s' has_mapping_firmware = %d",
		fname, record->name, (int) handel_mca->has_mapping_firmware));
#endif

#if 0
	/* Find out what firmware variant and revision is used by this MCA. */

	mx_status = mxi_handel_read_parameter( mca, "CODEVAR", &codevar );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	handel_mca->firmware_code_variant = (int) codevar;

	mx_status = mxi_handel_read_parameter( mca, "CODEREV", &coderev );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	handel_mca->firmware_code_revision = (int) coderev;
#endif

	/* Verify that the firmware version in use supports hardware SCAs. */

	/* Use some heuristics based on the version of the firmware being run.*/

	if ( handel_mca->hardware_scas_are_enabled ) {
#if 0
		if ( ( codevar == 0 ) && ( coderev >= 108 ) ) {
			handel_mca->hardware_scas_are_enabled = TRUE;
		} else {
			handel_mca->hardware_scas_are_enabled = FALSE;
		}
#endif
	}

	/* If the MCA is controlled via Handel and hardware SCAs are enabled,
	 * try to set the number of SCAs to the value of mca->maximum_num_rois.
	 */

	if ( handel_mca->hardware_scas_are_enabled ) {

		double num_scas;

		num_scas = mca->maximum_num_rois;

#if MXD_HANDEL_MCA_DEBUG_TIMING
		MX_HRT_START( measurement );
#endif

		if ( handel_mca->debug_flag ) {
			MX_DEBUG(-2,
			("%s: About to set the number of hardware SCAs to %g",
				fname, num_scas));
		}

		MX_XIA_SYNC( xiaSetAcquisitionValues(
				handel_mca->detector_channel,
				"number_of_scas",
				(void *) &num_scas ) );

#if MXD_HANDEL_MCA_DEBUG_TIMING
		MX_HRT_END( measurement );

		MX_HRT_RESULTS( measurement, fname,
		  "xiaSetAcquisitionValues(..., \"number_of_scas\", ...)" );
#endif

		if ( xia_status != XIA_SUCCESS ) {
			(void) mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
	"The attempt to set the number of SCAs to 16 for MCA '%s' failed.  "
	"The use of hardware SCAs for this MCA will be disabled.  "
	"Error code = %d, '%s'", mca->record->name,
	xia_status, mxi_handel_strerror( xia_status ) );

			handel_mca->hardware_scas_are_enabled = FALSE;
		}

		/* Some Handel-supported XIA modules return ROI integrals
		 * as an array of 64-bit doubles.  We must check for this.
		 */

		if ( strcmp( handel_mca->module_type, "xmap" ) == 0 ) {
			handel_mca->use_double_roi_integral_array = TRUE;
		}

#if MXD_HANDEL_MCA_DEBUG_DOUBLE_ROIS
		MX_DEBUG(-2,("%s: MCA '%s' use_double_roi_integral_array = %d",
			fname, mca->record->name,
			(int) handel_mca->use_double_roi_integral_array ));
#endif

		if( handel_mca->use_double_roi_integral_array == TRUE ) {

			handel_mca->double_roi_integral_array = (double *)
			    malloc( mca->maximum_num_rois * sizeof(double) );

			if ( handel_mca->double_roi_integral_array == NULL ) {
				return mx_error( MXE_OUT_OF_MEMORY, fname,
				"Ran out of memory trying to allocate a "
				"%ld element array of 64-bit roi integrals "
				"for MCA '%s'.",
					mca->maximum_num_rois,
					mca->record->name );
			}
		}

#if MXD_HANDEL_MCA_DEBUG_DOUBLE_ROIS
		MX_DEBUG(-2,("%s: MCA '%s' double_roi_integral_array = %p",
			fname, mca->record->name,
			handel_mca->double_roi_integral_array));
#endif
	}

#if 0
	if ( display_config ) {
		mx_info(
	"MCA '%s': codevar = %#lx, coderev = %#lx, hardware SCAs = %d",
			handel_mca->record->name,
			handel_mca->firmware_code_variant,
			handel_mca->firmware_code_revision,
			(int) handel_mca->hardware_scas_are_enabled );
	}
#endif

	/* Initialize the range of bin numbers used by the MCA. */

	if ( 1 ) {
		double num_mx_channels;

		num_mx_channels = (double) mca->maximum_num_channels;

		MX_XIA_SYNC( xiaSetAcquisitionValues(
					handel_mca->detector_channel,
					"number_mca_channels",
					(void *) &num_mx_channels ) );

		if ( xia_status != XIA_SUCCESS ) {
			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"The attempt to set the number of bins for MCA '%s' "
			"failed.  Error code = %d, '%s'", mca->record->name,
			xia_status, mxi_handel_strerror( xia_status ) );
		}
	}

	mca->current_num_channels = mca->maximum_num_channels;
	mca->current_num_rois     = mca->maximum_num_rois;

	for ( i = 0; i < mca->maximum_num_rois; i++ ) {
		mca->roi_array[i][0] = 0;
		mca->roi_array[i][1] = 0;
	}

	/* Initialize the saved preset values to illegal values. */

	handel_mca->old_preset_type = (unsigned long) ULONG_MAX;
	handel_mca->old_preset_time = DBL_MAX;

#if MXD_HANDEL_MCA_DEBUG
	MX_DEBUG(-2,("%s: MCA '%s', handel_mca->hardware_scas_are_enabled = %d",
		fname, record->name,
		(int) handel_mca->hardware_scas_are_enabled));
#endif

	if ( handel_mca->debug_flag ) {
		MX_DEBUG(-2,("%s complete for MCA '%s'", fname, record->name ));
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_handel_mca_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_handel_mca_close()";

	MX_MCA *mca;
	MX_HANDEL_MCA *handel_mca = NULL;
	MX_HANDEL *handel = NULL;
	MX_RECORD **mca_record_array;
	unsigned long i, num_mcas;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	mca = (MX_MCA *) (record->record_class_struct);

	mx_status = mxd_handel_mca_get_pointers( mca,
						&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Delete our entry in mca_record_array. */

	num_mcas = handel->num_mcas;
	mca_record_array = handel->mca_record_array;

	if ( handel_mca->baseline_array != NULL ) {
		mx_free( handel_mca->baseline_array );
	}
	if ( handel_mca->spectrum_array != NULL ) {
		mx_free( handel_mca->spectrum_array );
	}

	for ( i = 0; i < num_mcas; i++ ) {

		if ( record == mca_record_array[i] ) {
			mca_record_array[i] = NULL;

			break;		/* Exit the for() loop. */
		}
	}

	/* Don't treat it as an error if we did not find an entry
	 * in mca_record_array.
	 */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_handel_mca_special_processing_setup( MX_RECORD *record )
{
	static const char fname[] = "mxd_handel_mca_special_processing_setup()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG(2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_HANDEL_MCA_ACQUISITION_VALUE:
		case MXLV_HANDEL_MCA_ACQUISITION_VALUE_TO_ALL:
		case MXLV_HANDEL_MCA_APPLY:
		case MXLV_HANDEL_MCA_APPLY_TO_ALL:
		case MXLV_HANDEL_MCA_ADC_TRACE_ARRAY:
		case MXLV_HANDEL_MCA_BASELINE_ARRAY:
		case MXLV_HANDEL_MCA_BASELINE_HISTORY_ARRAY:
		case MXLV_HANDEL_MCA_GAIN_CHANGE:
		case MXLV_HANDEL_MCA_GAIN_CALIBRATION:
		case MXLV_HANDEL_MCA_PARAMETER_VALUE:
		case MXLV_HANDEL_MCA_PARAM_VALUE_TO_ALL_CHANNELS:
		case MXLV_HANDEL_MCA_SHOW_PARAMETERS:
		case MXLV_HANDEL_MCA_SHOW_ACQUISITION_VALUES:
		case MXLV_HANDEL_MCA_STATISTICS:
			record_field->process_function
					    = mxd_handel_mca_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

/*---------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_handel_mca_arm( MX_MCA *mca )
{
	static const char fname[] = "mxd_handel_mca_arm()";

	MX_HANDEL_MCA *handel_mca = NULL;
	MX_HANDEL *handel = NULL;
	unsigned long preset_type;
	double preset_type_as_double;
	double preset_time;
	mx_status_type mx_status;

	mx_status = mxd_handel_mca_get_pointers( mca,
						&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If mapping-capable firmware is running, then we turn off
	 * mapping_mode.
	 */

	if ( handel_mca->has_mapping_firmware ) {
		double mapping_mode = 0.0;
		int xia_status;

		MX_XIA_SYNC( xiaSetAcquisitionValues( -1,
					"mapping_mode", &mapping_mode ) );

		if ( xia_status != XIA_SUCCESS ) {
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"The attempt to set 'mapping_mode' to %f "
			"for MCA '%s' failed.  Error code = %d, '%s'",
			mapping_mode, mca->record->name,
			xia_status, mxi_handel_strerror(xia_status) );
		}
	}

	/* Select the preset type. */

	switch( mca->preset_type ) {
	case MXF_MCA_PRESET_NONE:
		preset_type = MXF_HANDEL_MCA_PRESET_NONE;
		preset_time = 0.0;
		break;

	case MXF_MCA_PRESET_LIVE_TIME:
		preset_type = MXF_HANDEL_MCA_PRESET_LIVE_TIME;
		preset_time = mca->preset_live_time;
		break;

	case MXF_MCA_PRESET_REAL_TIME:
		preset_type = MXF_HANDEL_MCA_PRESET_REAL_TIME;
		preset_time = mca->preset_real_time;
		break;

	case MXF_MCA_PRESET_COUNT:
		preset_type = MXF_HANDEL_MCA_PRESET_OUTPUT_EVENTS;
		preset_time = mca->preset_count;
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Preset type %ld is not supported for record '%s'",
			mca->preset_type, mca->record->name );
	}

#if MXD_HANDEL_MCA_DEBUG
	MX_DEBUG(-2,("%s: preset_type = %lu", fname,
				(unsigned long) preset_type));
#endif
	/* Save the preset value. */

	handel->last_measurement_interval = preset_time;

	MX_DEBUG( 2,("%s: preset_time = %g", fname, preset_time));

	/* If all the preset values for this MCA start are the same as 
	 * the values from the last MCA start, then we do not need to
	 * send any change parameter requests and can go ahead and start
	 * the MCA.
	 */

	if ( ( preset_type == handel_mca->old_preset_type )
	  && ( preset_time == handel_mca->old_preset_time ) )
	{
		MX_DEBUG( 2,("%s: Presets will NOT be changed.", fname));

		mx_status = mxd_handel_mca_start_run( mca, TRUE );

		/* We are done, so return to the caller. */

		return mx_status;
	}

	/* One or more of the preset parameters are different than
	 * before, so we must set them all.
	 */

	MX_DEBUG( 2,("%s: Presets will be changed.", fname));

	/******* Set the preset type. *******/

	preset_type_as_double = preset_type;

	mx_status = mxi_handel_set_acquisition_values_for_all_channels( handel,
			"preset_type", &preset_type_as_double, TRUE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	handel_mca->old_preset_type = preset_type;

	/******* Set the timer preset. *******/

	mx_status = mxi_handel_set_acquisition_values_for_all_channels( handel,
			"preset_value", &preset_time, TRUE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	handel_mca->old_preset_time = preset_time;

	handel_mca->new_statistics_available = TRUE;

	HANDEL_MCA_DEBUG_STATISTICS( handel_mca );

	if ( mca->trigger_mode == MXF_DEV_EXTERNAL_TRIGGER ) {
		/* Start the MCA. */

		mx_status = mxd_handel_mca_start_run( mca, 1 );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_handel_mca_trigger( MX_MCA *mca )
{
	static const char fname[] = "mxd_handel_mca_trigger()";

	mx_status_type mx_status;

	mx_status = mxd_handel_mca_get_pointers( mca, NULL, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mca->trigger_mode == MXF_DEV_INTERNAL_TRIGGER ) {
		/* Start the MCA. */

		mx_status = mxd_handel_mca_start_run( mca, 1 );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_handel_mca_stop( MX_MCA *mca )
{
	static const char fname[] = "mxd_handel_mca_stop()";

	MX_HANDEL_MCA *handel_mca = NULL;
	mx_status_type mx_status;

	mx_status = mxd_handel_mca_get_pointers( mca,
						&handel_mca, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Stop the MCA. */

	mx_status = mxd_handel_mca_stop_run( mca );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_handel_mca_read( MX_MCA *mca )
{
	static const char fname[] = "mxd_handel_mca_read()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL *handel;
	int xia_status;
	unsigned long *array_ptr;
	mx_status_type mx_status;

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxd_handel_mca_get_pointers( mca,
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

	MX_XIA_SYNC( xiaGetRunData( handel_mca->detector_channel,
					"mca", array_ptr ) );

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
mxd_handel_mca_clear( MX_MCA *mca )
{
	static const char fname[] = "mxd_handel_mca_clear()";

	MX_HANDEL_MCA *handel_mca = NULL;
	size_t num_longs_to_zero, num_bytes_to_zero;
	mx_status_type mx_status;

	mx_status = mxd_handel_mca_get_pointers( mca,
						&handel_mca, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* There doesn't appear to be a way of doing this in the MCA
	 * using Handel without starting a run.  However, we _can_
	 * clear our local copy of the spectrum data.
	 */

	if ( mca->channel_array == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MCA channel array has not been allocated for MCA '%s'.",
			mca->record->name );
	}

	num_longs_to_zero = mca->current_num_channels;

	if ( num_longs_to_zero > mca->maximum_num_channels ) {
		num_longs_to_zero = mca->maximum_num_channels;
	}

	num_bytes_to_zero = num_longs_to_zero * sizeof( unsigned long );

	memset( mca->channel_array, 0, num_bytes_to_zero );

	mca->new_data_available = TRUE;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_handel_mca_busy( MX_MCA *mca )
{
	static const char fname[] = "mxd_handel_mca_busy()";

	MX_HANDEL_MCA *handel_mca = NULL;
	MX_HANDEL *handel = NULL;
	unsigned long run_active, mask;
	int xia_status;
	mx_status_type mx_status;

#if MXD_HANDEL_MCA_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxd_handel_mca_get_pointers( mca,
					&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_HANDEL_MCA_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	MX_XIA_SYNC( xiaGetRunData( handel_mca->detector_channel,
					"run_active", &run_active ) );

#if MXD_HANDEL_MCA_DEBUG_TIMING
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
		mca->busy = TRUE;
	} else {
		mca->busy = FALSE;
	}

	if ( mca->busy == FALSE ) {

		/* Send a stop run command so that the run enabled bit
		 * in the DXP is cleared.
		 */

		mx_status = mxd_handel_mca_stop_run( mca );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	MX_DEBUG( 2,("%s: mca->busy = %d", fname, (int) mca->busy));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_handel_mca_get_parameter( MX_MCA *mca )
{
	static const char fname[] = "mxd_handel_mca_get_parameter()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL *handel;
	char acquisition_value_name[MAXALIAS_LEN+1];
	double acquisition_value;
	long i, j, handel_preset_type;
	int xia_status;
	void *integral_array;
	double dbl_value;
	unsigned long roi[2];
	mx_status_type mx_status;

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxd_handel_mca_get_pointers( mca,
					&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_HANDEL_DEBUG
	MX_DEBUG(-2,("%s invoked for MCA '%s', parameter type '%s' (%ld).",
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

#if MXI_HANDEL_DEBUG
		MX_DEBUG(-2,("%s: mca->channel_number = %lu",
			fname, mca->channel_number));
#endif
		break;

	case MXLV_MCA_CURRENT_NUM_CHANNELS:
		mx_status = mxd_handel_mca_get_acquisition_values( mca,
				"number_mca_channels", &acquisition_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mca->current_num_channels = mx_round( acquisition_value );

#if MXI_HANDEL_DEBUG
		MX_DEBUG(-2,("%s: mca->current_num_channels = %ld",
			fname, mca->current_num_channels));
#endif
		break;

	case MXLV_MCA_PRESET_TYPE:
		mx_status = mxd_handel_mca_get_acquisition_values( mca,
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
			mca->preset_type = MXF_MCA_PRESET_COUNT;
			break;
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

#if MXI_HANDEL_DEBUG
		MX_DEBUG(-2,("%s: mca->preset_type = %ld",
			fname, mca->preset_type));
#endif
		break;

	case MXLV_MCA_PRESET_REAL_TIME:
		if ( mca->preset_type != MXF_MCA_PRESET_REAL_TIME ) {

			/* We want the preset value for a preset type
			 * that is not the _current_ preset type.
			 * This requires changing the preset type.
			 */

			mx_status = mxd_handel_mca_busy( mca );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( mca->busy ) {
				/* The MCA is currently acquiring a spectrum,
				 * so it is not possible to change the preset
				 * type right now.  Instead, we will return
				 * the cached value for this preset.
				 */
			} else {
				/* The MCA is _not_ acquiring a spectrum, so
				 * change the preset type.
				 */

				mca->preset_type    = MXF_MCA_PRESET_REAL_TIME;
				mca->parameter_type = MXLV_MCA_PRESET_TYPE;

				mx_status = mxd_handel_mca_set_parameter( mca );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				/* It is safe to read the acquisition value. */

				mx_status =
				    mxd_handel_mca_get_acquisition_values( mca,
				    "preset_value", &(mca->preset_real_time) );
			}
		} else {
			/* We are already set for the correct preset type,
			 * so just read the acquisition value.
			 */

			mx_status = mxd_handel_mca_get_acquisition_values( mca,
				"preset_value", &(mca->preset_real_time) );
		}

#if MXI_HANDEL_DEBUG_GET_PRESETS
		MX_DEBUG(-2,("%s: mca->preset_real_time = %g",
			fname, mca->preset_real_time));
#endif
		break;

	case MXLV_MCA_PRESET_LIVE_TIME:
		if ( mca->preset_type != MXF_MCA_PRESET_LIVE_TIME ) {

			/* We want the preset value for a preset type
			 * that is not the _current_ preset type.
			 * This requires changing the preset type.
			 */

			mx_status = mxd_handel_mca_busy( mca );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( mca->busy ) {
				/* The MCA is currently acquiring a spectrum,
				 * so it is not possible to change the preset
				 * type right now.  Instead, we will return
				 * the cached value for this preset.
				 */
			} else {
				/* The MCA is _not_ acquiring a spectrum, so
				 * change the preset type.
				 */

				mca->preset_type    = MXF_MCA_PRESET_LIVE_TIME;
				mca->parameter_type = MXLV_MCA_PRESET_TYPE;

				mx_status = mxd_handel_mca_set_parameter( mca );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				/* It is safe to read the acquisition value. */

				mx_status =
				    mxd_handel_mca_get_acquisition_values( mca,
				    "preset_value", &(mca->preset_live_time) );
			}
		} else {
			/* We are already set for the correct preset type,
			 * so just read the acquisition value.
			 */

			mx_status = mxd_handel_mca_get_acquisition_values( mca,
				"preset_value", &(mca->preset_live_time) );
		}

#if MXI_HANDEL_DEBUG_GET_PRESETS
		MX_DEBUG(-2,("%s: mca->preset_live_time = %g",
			fname, mca->preset_live_time));
#endif
		break;

	case MXLV_MCA_PRESET_COUNT:
		if ( mca->preset_type != MXF_MCA_PRESET_COUNT ) {

			/* We want the preset value for a preset type
			 * that is not the _current_ preset type.
			 * This requires changing the preset type.
			 */

			mx_status = mxd_handel_mca_busy( mca );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( mca->busy ) {
				/* The MCA is currently acquiring a spectrum,
				 * so it is not possible to change the preset
				 * type right now.  Instead, we will return
				 * the cached value for this preset.
				 */
			} else {
				/* The MCA is _not_ acquiring a spectrum, so
				 * change the preset type.
				 */

				mca->preset_type    = MXF_MCA_PRESET_COUNT;
				mca->parameter_type = MXLV_MCA_PRESET_TYPE;

				mx_status = mxd_handel_mca_set_parameter( mca );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				/* It is safe to read the acquisition value. */

				mx_status =
				    mxd_handel_mca_get_acquisition_values( mca,
				    "preset_value", &acquisition_value );

				mca->preset_count = mx_round(acquisition_value);
			}
		} else {
			/* We are already set for the correct preset type,
			 * so just read the acquisition value.
			 */

			mx_status = mxd_handel_mca_get_acquisition_values( mca,
				"preset_value", &acquisition_value );

			mca->preset_count = mx_round( acquisition_value );
		}

#if MXI_HANDEL_DEBUG_GET_PRESETS
		MX_DEBUG(-2,("%s: mca->preset_count = %g",
			fname, mca->preset_count));
#endif
		break;

	case MXLV_MCA_CHANNEL_VALUE:
		mca->channel_value = mca->channel_array[ mca->channel_number ];

#if MXI_HANDEL_DEBUG
		MX_DEBUG(-2,("%s: mca->channel_value = %lu",
			fname, mca->channel_value));
#endif
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
				snprintf( acquisition_value_name,
					sizeof(acquisition_value_name),
					"sca%ld_lo", i );
			} else {
				snprintf( acquisition_value_name,
					sizeof(acquisition_value_name),
					"SCA%ldLO", i );
			}
			
			MX_XIA_SYNC( xiaGetAcquisitionValues(
				handel_mca->detector_channel,
				acquisition_value_name,
				(void *) &acquisition_value ) );

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
				snprintf( acquisition_value_name,
					sizeof(acquisition_value_name),
						"sca%ld_hi", i );
			} else {
				snprintf( acquisition_value_name,
					sizeof(acquisition_value_name),
						"SCA%ldHI", i );
			}
			
			MX_XIA_SYNC( xiaGetAcquisitionValues(
				handel_mca->detector_channel,
				acquisition_value_name,
				(void *) &acquisition_value ) );

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
#if MXI_HANDEL_DEBUG
		    MX_DEBUG(-2,("%s: mca->roi_array[%lu][0] = %lu",
				fname, i, mca->roi_array[i][0]));
		    MX_DEBUG(-2,("%s: mca->roi_array[%lu][1] = %lu",
				fname, i, mca->roi_array[i][1]));
#endif
		}
		break;

	case MXLV_MCA_ROI_INTEGRAL_ARRAY:
	case MXLV_MCA_ROI_INTEGRAL:

#if MXI_HANDEL_DEBUG
		MX_DEBUG(-2,("%s: Reading roi_integral_array", fname));
#endif
		if ( handel_mca->hardware_scas_are_enabled ) {
			/* This system supports SCA integrals computed
			 * by the firmware.
			 */

#if MXI_HANDEL_DEBUG
			MX_DEBUG(-2,("%s: use_double_roi_integral_array = %d",
			  fname, (int) handel_mca->double_roi_integral_array));
#endif
			if ( handel_mca->use_double_roi_integral_array ) {
				integral_array =
					handel_mca->double_roi_integral_array;
			} else {
				integral_array = mca->roi_integral_array;
			}

			MX_XIA_SYNC( xiaGetRunData(
				handel_mca->detector_channel,
				"sca", integral_array ) );

#if 0
			{
				mx_bool_type valid;

				valid = mx_database_is_valid(
						handel_mca->record, 0 );

				if ( valid == FALSE ) {
					return mx_error(
						MXE_LIMIT_WAS_EXCEEDED, fname,
					"A call to xiaGetRunData( ,sca, ) "
					"has caused a buffer overrun.\n"
					"   Alas, all is lost...\n" );
				}
			}
#endif

			if ( xia_status != XIA_SUCCESS ) {
				return mx_error(
				    MXE_INTERFACE_ACTION_FAILED, fname,
			"Cannot read the region of interest integrals "
			"from MCA '%s'.  Error code = %d, '%s'",
					mca->record->name, xia_status,
					mxi_handel_strerror( xia_status ) );
			}

			if ( handel_mca->use_double_roi_integral_array ) {
				for ( i = 0; i < mca->maximum_num_rois; i++ ) {
					dbl_value =
				    handel_mca->double_roi_integral_array[i];

					if ( dbl_value >= LONG_MAX ) {
						mca->roi_integral_array[i] =
							LONG_MAX;
					} else {
						mca->roi_integral_array[i] =
							mx_round( dbl_value );
					}
				}
			}

			if ( mca->parameter_type == MXLV_MCA_ROI_INTEGRAL ) {
				mca->roi_integral =
				    mca->roi_integral_array[ mca->roi_number ];
#if MXI_HANDEL_DEBUG
				MX_DEBUG(-2,("%s: mca->roi_integral = %lu",
					fname, mca->roi_integral));
#endif
			}

		} else {
			/* Hardware SCA support is not used for this case. */

			if (mca->parameter_type == MXLV_MCA_ROI_INTEGRAL_ARRAY)
			{
				return
				    mx_mca_default_get_parameter_handler( mca );
			} else {
				i = mca->roi_number;
	
				mx_status = mx_mca_read( mca->record,
							NULL, NULL );
	
				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
	
				/* Make sure the ROI boundaries are correct. */
	
				mx_status = mx_mca_get_roi( mca->record,
								i, roi );
	
				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
	
				MX_DEBUG( 2,("%s: Integrating MCA '%s' ROI %lu "
					"integral from bin %lu to bin %lu",
					fname, mca->record->name,
					(unsigned long) i,
					roi[0], roi[1]));
	
				mca->roi_integral = 0;
	
				for ( j = roi[0]; j <= roi[1]; j++ ) {
				    mca->roi_integral += mca->channel_array[j];
				}

				MX_DEBUG( 2,("%s: ROI %lu integral = %lu",
				  fname, (unsigned long) i, mca->roi_integral));
			}
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
			snprintf( acquisition_value_name,
				sizeof(acquisition_value_name),
				"sca%ld_lo", i );
		    } else {
			snprintf( acquisition_value_name,
				sizeof(acquisition_value_name),
				"SCA%ldLO", i );
		    }
			
		    MX_XIA_SYNC( xiaGetAcquisitionValues(
				handel_mca->detector_channel,
				acquisition_value_name,
				(void *) &acquisition_value ) );

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
			snprintf( acquisition_value_name,
				sizeof(acquisition_value_name),
				"sca%ld_hi", i );
		    } else {
			snprintf( acquisition_value_name,
				sizeof(acquisition_value_name),
				"SCA%ldHI", i );
		    }
			
		    MX_XIA_SYNC( xiaGetAcquisitionValues(
				handel_mca->detector_channel,
				acquisition_value_name,
				(void *) &acquisition_value ) );

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

#if MXI_HANDEL_DEBUG
		MX_DEBUG(-2,("%s: mca->roi[0] = %lu", fname, mca->roi[0]));
		MX_DEBUG(-2,("%s: mca->roi[1] = %lu", fname, mca->roi[1]));
#endif
		break;
	
	case MXLV_MCA_REAL_TIME:
		MX_XIA_SYNC( xiaGetRunData( handel_mca->detector_channel,
			"runtime", (void *) &(mca->real_time) ) );

		if ( xia_status != XIA_SUCCESS ) {
			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"Cannot read the value '%s' from MCA '%s'.  "
			"Error code = %d, '%s'", acquisition_value_name,
					mca->record->name, xia_status,
					mxi_handel_strerror( xia_status ) );
		}
#if MXI_HANDEL_DEBUG
		MX_DEBUG(-2,("%s: mca->real_time = %g",
			fname, mca->real_time));
#endif
		break;

	case MXLV_MCA_LIVE_TIME:
		MX_XIA_SYNC( xiaGetRunData( handel_mca->detector_channel,
			"trigger_livetime", (void *) &(mca->live_time) ) );

		if ( xia_status != XIA_SUCCESS ) {
			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"Cannot read the value '%s' from MCA '%s'.  "
			"Error code = %d, '%s'", acquisition_value_name,
					mca->record->name, xia_status,
					mxi_handel_strerror( xia_status ) );
		}
#if MXI_HANDEL_DEBUG
		MX_DEBUG(-2,("%s: mca->live_time = %g",
			fname, mca->live_time));
#endif
		break;

	case MXLV_MCA_INPUT_COUNT_RATE:
		MX_XIA_SYNC( xiaGetRunData( handel_mca->detector_channel,
		    "input_count_rate", (void *) &(mca->input_count_rate) ) );

		if ( xia_status != XIA_SUCCESS ) {
			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"Cannot read the value '%s' from MCA '%s'.  "
			"Error code = %d, '%s'", acquisition_value_name,
					mca->record->name, xia_status,
					mxi_handel_strerror( xia_status ) );
		}
#if MXI_HANDEL_DEBUG
		MX_DEBUG(-2,("%s: $$$ mca->input_count_rate = %g",
			fname, mca->input_count_rate));
#endif
		break;

	case MXLV_MCA_OUTPUT_COUNT_RATE:
		MX_XIA_SYNC( xiaGetRunData( handel_mca->detector_channel,
		    "output_count_rate", (void *) &(mca->output_count_rate) ) );

		if ( xia_status != XIA_SUCCESS ) {
			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"Cannot read the value '%s' from MCA '%s'.  "
			"Error code = %d, '%s'", acquisition_value_name,
					mca->record->name, xia_status,
					mxi_handel_strerror( xia_status ) );
		}
#if MXI_HANDEL_DEBUG
		MX_DEBUG(-2,("%s: $$$ mca->output_count_rate = %g",
			fname, mca->output_count_rate));
#endif
		break;

	case MXLV_MCA_COUNTS:
		MX_XIA_SYNC( xiaGetRunData( handel_mca->detector_channel,
			"events_in_run", (void *) &(mca->counts) ) );

		if ( xia_status != XIA_SUCCESS ) {
			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"Cannot read the value '%s' from MCA '%s'.  "
			"Error code = %d, '%s'", acquisition_value_name,
					mca->record->name, xia_status,
					mxi_handel_strerror( xia_status ) );
		}
#if MXI_HANDEL_DEBUG
		MX_DEBUG(-2,("%s: mca->counts = %lu", fname, mca->counts));
#endif
		break;

	default:
		return mx_mca_default_get_parameter_handler( mca );
		break;
	}

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, "parameter type '%s' (%ld)",
		mx_get_field_label_string(mca->record, mca->parameter_type),
		mca->parameter_type );
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_handel_mca_set_parameter( MX_MCA *mca )
{
	static const char fname[] = "mxd_handel_mca_set_parameter()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL *handel;
	char acquisition_value_name[MAXALIAS_LEN+1];
	double acquisition_value;
	long i, handel_preset_type;
	mx_status_type mx_status;

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxd_handel_mca_get_pointers( mca,
					&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_HANDEL_DEBUG
	MX_DEBUG(-2,("%s invoked for MCA '%s', parameter type '%s' (%ld).",
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

		mx_status = mxd_handel_mca_set_acquisition_values( mca,
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
		case MXF_MCA_PRESET_COUNT:
			handel_preset_type =
					MXF_HANDEL_MCA_PRESET_OUTPUT_EVENTS;
			break;
		default:
			return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
				"MX preset type %ld is not yet implemented "
				"for MCA '%s'.",
				mca->preset_type, mca->record->name );
			break;
		}
		acquisition_value = (double) handel_preset_type;

		mx_status = mxd_handel_mca_set_acquisition_values( mca,
				"preset_type", &acquisition_value, TRUE );
		break;

	case MXLV_MCA_PRESET_REAL_TIME:
		if ( mca->preset_type != MXLV_MCA_PRESET_REAL_TIME ) {

			mca->preset_type    = MXF_MCA_PRESET_REAL_TIME;
			mca->parameter_type = MXLV_MCA_PRESET_TYPE;

			mx_status = mxd_handel_mca_set_parameter( mca );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		mx_status = mxd_handel_mca_set_acquisition_values( mca,
			"preset_value", &(mca->preset_real_time), TRUE );
		break;

	case MXLV_MCA_PRESET_LIVE_TIME:
		if ( mca->preset_type != MXLV_MCA_PRESET_LIVE_TIME ) {

			mca->preset_type    = MXF_MCA_PRESET_LIVE_TIME;
			mca->parameter_type = MXLV_MCA_PRESET_TYPE;

			mx_status = mxd_handel_mca_set_parameter( mca );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		mx_status = mxd_handel_mca_set_acquisition_values( mca,
			"preset_value", &(mca->preset_live_time), TRUE );
		break;

	case MXLV_MCA_PRESET_COUNT:
		if ( mca->preset_type != MXLV_MCA_PRESET_COUNT ) {

			mca->preset_type    = MXF_MCA_PRESET_COUNT;
			mca->parameter_type = MXLV_MCA_PRESET_TYPE;

			mx_status = mxd_handel_mca_set_parameter( mca );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		acquisition_value = (double) mca->preset_count;

		mx_status = mxd_handel_mca_set_acquisition_values( mca,
			"preset_value", &acquisition_value, TRUE );
		break;


	case MXLV_MCA_ROI_ARRAY:
		for ( i = 0; i < mca->current_num_rois; i++ ) {
			snprintf( acquisition_value_name,
				sizeof(acquisition_value_name),
				"sca%ld_lo", i );

			acquisition_value = (double) mca->roi_array[i][0];
			
			mx_status = mxd_handel_mca_set_acquisition_values( mca,
					acquisition_value_name,
					&acquisition_value, FALSE );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			snprintf( acquisition_value_name,
				sizeof(acquisition_value_name),
				"sca%ld_hi", i );
			
			acquisition_value = (double) mca->roi_array[i][1];
			
			mx_status = mxd_handel_mca_set_acquisition_values( mca,
					acquisition_value_name,
					&acquisition_value, FALSE );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			handel_mca->sca_has_been_initialized[i] = TRUE;
		}

		mx_status = mxi_handel_apply_to_all_channels( handel );

		break;

	case MXLV_MCA_ROI:
		i = mca->roi_number;

		snprintf( acquisition_value_name,
			sizeof(acquisition_value_name),
			"sca%ld_lo", i );

		acquisition_value = (double) mca->roi[0];

		MX_DEBUG( 2,
		("%s: acquisition_value_name = '%s', acquisition_value = %g",
			fname, acquisition_value_name, acquisition_value));

		mx_status = mxd_handel_mca_set_acquisition_values( mca,
					acquisition_value_name,
					&acquisition_value, FALSE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
			
		mca->roi_array[i][0] = mca->roi[0];

		/*---*/

		snprintf( acquisition_value_name,
			sizeof(acquisition_value_name),
			"sca%ld_hi", i );

		acquisition_value = (double) mca->roi[1];
			
		MX_DEBUG( 2,
		("%s: acquisition_value_name = '%s', acquisition_value = %g",
			fname, acquisition_value_name, acquisition_value));
			
		mx_status = mxd_handel_mca_set_acquisition_values( mca,
					acquisition_value_name,
					&acquisition_value, FALSE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
			
		mca->roi_array[i][1] = mca->roi[1];

		handel_mca->sca_has_been_initialized[i] = TRUE;

		mx_status = mxi_handel_apply_to_all_channels( handel );

		break;

	default:
		return mx_mca_default_set_parameter_handler( mca );
		break;
	}

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, "parameter type '%s' (%ld)",
		mx_get_field_label_string(mca->record, mca->parameter_type),
		mca->parameter_type );
#endif

	return mx_status;
}

/*---------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_handel_mca_get_mca_array( MX_RECORD *handel_record,
				unsigned long *num_mcas,
				MX_RECORD ***mca_record_array )
{
	static const char fname[] = "mxd_handel_mca_get_mca_array()";

	MX_HANDEL *handel;

	if ( handel_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The handel_record pointer passed was NULL." );
	}
	if ( num_mcas == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The num_mcas pointer passed was NULL." );
	}
	if ( mca_record_array == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The mca_record_array pointer passed was NULL." );
	}

	handel = (MX_HANDEL *) handel_record->record_type_struct;

	if ( handel == (MX_HANDEL *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_HANDEL pointer for record '%s' is NULL.",
			handel_record->name );
	}

	*num_mcas = handel->num_mcas;
	*mca_record_array = handel->mca_record_array;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_handel_mca_get_rate_corrected_roi_integral( MX_MCA *mca,
			unsigned long roi_number,
			double *corrected_roi_value )
{
	static const char fname[] =
		"mxd_handel_mca_get_rate_corrected_roi_integral()";

	MX_HANDEL_MCA *handel_mca = NULL;
	unsigned long mca_value;
	double corrected_value, multiplier;
	mx_status_type mx_status;

	mx_status = mxd_handel_mca_get_pointers( mca,
						&handel_mca, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( corrected_roi_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The corrected_roi_error pointer passed was NULL." );
	}

	mx_status = mx_mca_get_roi_integral( mca->record,
						roi_number, &mca_value );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	corrected_value = (double) mca_value;

	mx_status = mxd_handel_mca_read_statistics( mca );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	multiplier = mx_divide_safely( handel_mca->input_count_rate,
					handel_mca->output_count_rate );

	corrected_value = mx_multiply_safely( corrected_value, multiplier );

	*corrected_roi_value = corrected_value;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_handel_mca_get_livetime_corrected_roi_integral( MX_MCA *mca,
			unsigned long roi_number,
			double *corrected_roi_value )
{
	static const char fname[] =
		"mxd_handel_mca_get_livetime_corrected_roi_integral()";

	MX_HANDEL_MCA *handel_mca = NULL;
	unsigned long mca_value;
	double corrected_value, multiplier;
	mx_status_type mx_status;

	mx_status = mxd_handel_mca_get_pointers( mca,
						&handel_mca, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( corrected_roi_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The corrected_roi_error pointer passed was NULL." );
	}

	mx_status = mx_mca_get_roi_integral( mca->record,
						roi_number, &mca_value );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	corrected_value = (double) mca_value;

	mx_status = mxd_handel_mca_read_statistics( mca );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	multiplier = mx_divide_safely( handel_mca->input_count_rate,
					handel_mca->output_count_rate );

	corrected_value = mx_multiply_safely( corrected_value, multiplier );

	multiplier = mx_divide_safely( mca->real_time, mca->live_time );

	corrected_value = mx_multiply_safely( corrected_value, multiplier );

	*corrected_roi_value = corrected_value;

	return MX_SUCCESSFUL_RESULT;
}

#define MXU_HANDEL_MCA_NUM_STATISTICS	36    /* FIXME: 36 is for DXP-XMAP */

MX_EXPORT mx_status_type
mxd_handel_mca_read_statistics( MX_MCA *mca )
{
	static const char fname[] = "mxd_handel_mca_read_statistics()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL *handel;
	double module_statistics[MXU_HANDEL_MCA_NUM_STATISTICS];
	long channel_offset;
	mx_bool_type read_statistics;
	mx_status_type mx_status;

	mx_status = mxd_handel_mca_get_pointers( mca,
					&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mca->mca_flags & MXF_MCA_NO_READ_OPTIMIZATION ) {
		read_statistics = TRUE;

	} else if ( handel_mca->new_statistics_available ) {
		read_statistics = TRUE;

	} else {
		read_statistics = FALSE;
	}

	HANDEL_MCA_DEBUG_STATISTICS( handel_mca );

	if ( read_statistics == FALSE ) {

		/* Return now if we will be returning cached values. */

		HANDEL_MCA_DEBUG_STATISTICS( handel_mca );

		return MX_SUCCESSFUL_RESULT;
	}

	/* If we get here, we will be reading new statistics. */

	if ( handel->use_module_statistics_2 ) {
		mx_status = mxd_handel_mca_get_run_data( mca,
					"module_statistics_2",
					(void *) module_statistics );
	} else {
		mx_status = mxd_handel_mca_get_run_data( mca,
					"module_statistics",
					(void *) module_statistics );
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( strcmp( handel_mca->module_type, "xmap" ) == 0 ) {

		if ( handel_mca->debug_flag ) {
			int i;

			for ( i = 0; i < MXU_HANDEL_MCA_NUM_STATISTICS; i++ ) {
				MX_DEBUG(-2,
				("%s: statistics[%d] = %g",
				fname, i, module_statistics[i] ));
			}
		}

		channel_offset = handel_mca->detector_channel % 4;

		mca->real_time = module_statistics[ 9*channel_offset + 0 ];
		mca->live_time = module_statistics[ 9*channel_offset + 1 ];

		handel_mca->energy_live_time
				= module_statistics[ 9*channel_offset + 2 ];

		handel_mca->num_triggers = mx_round(
				module_statistics[ 9*channel_offset + 3 ] );
		handel_mca->num_events = mx_round(
				module_statistics[ 9*channel_offset + 4 ] );

		handel_mca->input_count_rate
				= module_statistics[ 9*channel_offset + 5 ];
		handel_mca->output_count_rate
				= module_statistics[ 9*channel_offset + 6 ];

		if ( handel->use_module_statistics_2 ) {
			handel_mca->num_underflows = mx_round(
				module_statistics[ 9*channel_offset + 7 ] );
			handel_mca->num_overflows = mx_round(
				module_statistics[ 9*channel_offset + 8 ] );
		} else {
			handel_mca->num_underflows = 0;
			handel_mca->num_overflows = 0;
		}
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"The reading of module statistics for module type '%s' "
		"of MCA '%s' is not currently supported.",
			handel_mca->module_type,
			mca->record->name );
	}

#if MXD_HANDEL_MCA_DEBUG_STATISTICS
	if ( 1 ) {
#else
	if ( handel_mca->debug_flag ) {
#endif
		MX_DEBUG(-2,(
	"%s: Record '%s', channel %ld, real_time = %g, live_time = %g, "
	"e_live_time = %g, #trig = %lu, #event = %lu, "
	"icr = %g, ocr = %g, #under = %lu, #over = %lu", fname,
			mca->record->name,
			handel_mca->detector_channel,
			mca->real_time,
			mca->live_time,
			handel_mca->energy_live_time,
			handel_mca->num_triggers,
			handel_mca->num_events,
			handel_mca->input_count_rate,
			handel_mca->output_count_rate,
			handel_mca->num_underflows,
			handel_mca->num_overflows));
	}

	if ( mca->busy == FALSE ) {
		handel_mca->new_statistics_available = FALSE;
	}

	HANDEL_MCA_DEBUG_STATISTICS( handel_mca );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_handel_mca_get_run_data( MX_MCA *mca,
			char *name,
			void *value_ptr )
{
	static const char fname[] = "mxd_handel_mca_get_run_data()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL *handel;
	int xia_status;
	mx_status_type mx_status;

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif
	mx_status = mxd_handel_mca_get_pointers( mca,
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

	MX_XIA_SYNC( xiaGetRunData( handel_mca->detector_channel,
					name, value_ptr ) );

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
mxd_handel_mca_get_acquisition_values( MX_MCA *mca,
				char *value_name,
				double *value_ptr )
{
	static const char fname[] = "mxd_handel_mca_get_acquisition_values()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL *handel;
	int xia_status;
	mx_status_type mx_status;

	mx_status = mxd_handel_mca_get_pointers( mca,
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

	if ( handel_mca->debug_flag ) {
		MX_DEBUG(-2,("%s: getting acquisition value '%s' for MCA '%s'.",
			fname, value_name, mca->record->name ));
	}

	MX_XIA_SYNC( xiaGetAcquisitionValues( handel_mca->detector_channel,
					value_name, (void *) value_ptr ) );

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

	if ( handel_mca->debug_flag ) {
		MX_DEBUG(-2,("%s: acquisition value '%s' for MCA '%s' = %g.",
			fname, value_name, mca->record->name, *value_ptr ));
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_handel_mca_set_acquisition_values( MX_MCA *mca,
				char *value_name,
				double *value_ptr,
				mx_bool_type apply_flag )
{
	static const char fname[] = "mxd_handel_mca_set_acquisition_values()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL *handel;
	int xia_status;
	mx_status_type mx_status;

	mx_status = mxd_handel_mca_get_pointers( mca,
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

	if ( handel_mca->debug_flag ) {
		MX_DEBUG(-2,
		("%s: setting acquisition value '%s' for MCA '%s' to %g.",
		fname, value_name, mca->record->name, *value_ptr ));
	}

	MX_XIA_SYNC( xiaSetAcquisitionValues( handel_mca->detector_channel,
					value_name, (void *) value_ptr ) );

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
					*value_ptr,
					xia_status,
					mxi_handel_strerror( xia_status ) );
	}

	if ( apply_flag ) {
		mx_status = mxi_handel_apply_to_all_channels( handel );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_handel_mca_apply( MX_MCA *mca )
{
	static const char fname[] = "mxd_handel_mca_apply()";

	MX_HANDEL_MCA *handel_mca = NULL;
	MX_HANDEL *handel = NULL;
	int xia_status, ignored;
	mx_status_type mx_status;

	mx_status = mxd_handel_mca_get_pointers( mca,
					&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( handel_mca->debug_flag ) {
		MX_DEBUG(-2,("%s for MCA '%s'", fname, mca->record->name ));
	}

	ignored = 0;

	MX_XIA_SYNC( xiaBoardOperation( handel_mca->detector_channel,
						"apply", (void *) &ignored ) );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Cannot apply acquisition values for MCA '%s'.  "
		"Error code = %d, '%s'", 
			mca->record->name,
			xia_status,
			mxi_handel_strerror( xia_status ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_handel_mca_start_run( MX_MCA *mca, mx_bool_type clear_flag )
{
	static const char fname[] = "mxd_handel_mca_start_run()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL *handel;
	unsigned short resume_flag;
	int xia_status;
	mx_status_type mx_status;

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxd_handel_mca_get_pointers( mca,
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

	MX_XIA_SYNC( xiaStartRun( handel_mca->detector_channel, resume_flag ) );

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
mxd_handel_mca_stop_run( MX_MCA *mca )
{
	static const char fname[] = "mxd_handel_mca_stop_run()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL *handel;
	int xia_status;
	mx_status_type mx_status;

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxd_handel_mca_get_pointers( mca,
					&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( handel_mca->debug_flag ) {
		MX_DEBUG(-2,("%s: Record '%s'.", fname, mca->record->name));
	}

#if MXI_HANDEL_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	MX_XIA_SYNC( xiaStopRun( handel_mca->detector_channel ) );

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
mxd_handel_mca_get_baseline_array( MX_MCA *mca )
{
	static const char fname[] =
			"mxd_handel_mca_get_baseline_array()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL *handel;
	MX_RECORD_FIELD *field;
	int xia_status;
	void *void_ptr;
	unsigned baseline_length;
	mx_status_type mx_status;

	mx_status = mxd_handel_mca_get_pointers( mca,
					&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for MCA '%s'.", fname, mca->record->name ));

	/* Get the current baseline length. */

	void_ptr = (void *) &baseline_length;

	MX_XIA_SYNC( xiaGetRunData( handel_mca->detector_channel,
					"baseline_length", void_ptr ) );

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

	MX_DEBUG(-2,("%s: baseline_length = %u", fname, baseline_length));

	/* Allocate or resize the baseline array as necessary. */

	if ( handel_mca->baseline_array == NULL ) {
	    handel_mca->baseline_array = (unsigned long *)
		malloc( baseline_length * sizeof(unsigned long) );

	    if ( handel_mca->baseline_array == (unsigned long *) NULL)
	    {
		handel_mca->baseline_length = 0;

		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory attempting to allocate a "
			"%u element baseline array for MCA '%s'.",
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
			"array to %u elements for MCA '%s'.",
				baseline_length, mca->record->name );
	    }

	    handel_mca->baseline_length = baseline_length;
	}

	MX_DEBUG(-2,("%s: getting baseline array for MCA '%s'.",
		fname, mca->record->name ));

	/* Now read the baseline array. */

	void_ptr = (void *) &(handel_mca->baseline_array[0]);

	MX_XIA_SYNC( xiaGetRunData( handel_mca->detector_channel,
					"baseline", void_ptr ) );

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
mxd_handel_mca_set_gain_change( MX_MCA *mca )
{
	static const char fname[] = "mxd_handel_mca_set_gain_change()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL *handel;
	int xia_status;
	mx_status_type mx_status;

	mx_status = mxd_handel_mca_get_pointers( mca,
					&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for MCA '%s', gain_change = %g",
		fname, mca->record->name, handel_mca->gain_change));

	MX_XIA_SYNC( xiaGainOperation( handel_mca->detector_channel,
					"calibrate",
					&(handel_mca->gain_change) ) );

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
mxd_handel_mca_set_gain_calibration( MX_MCA *mca )
{
	static const char fname[] = "mxd_handel_mca_set_gain_calibration()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL *handel;
	int xia_status;
	mx_status_type mx_status;

	mx_status = mxd_handel_mca_get_pointers( mca,
					&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for MCA '%s', gain_calibration = %g",
		fname, mca->record->name, handel_mca->gain_calibration));

	MX_XIA_SYNC( xiaGainCalibrate( handel_mca->detector_channel,
					handel_mca->gain_calibration ) );

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
mxd_handel_mca_get_adc_trace_array( MX_MCA *mca )
{
	static const char fname[] = "mxd_handel_mca_get_adc_trace_array()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL *handel;
	MX_RECORD_FIELD *field;
	int xia_status;
	void *void_ptr;
	unsigned adc_trace_length;
	mx_status_type mx_status;

	/* Tracewait of 5 microseconds (in nanoseconds). */

	double info[2];

	mx_status = mxd_handel_mca_get_pointers( mca,
					&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for MCA '%s'.", fname, mca->record->name ));

	/* Get the current ADC trace length. */

	void_ptr = (void *) &adc_trace_length;

	MX_XIA_SYNC( xiaGetSpecialRunData( handel_mca->detector_channel,
					"adc_trace_length", void_ptr ) );

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

	MX_DEBUG(-2,("%s: adc_trace_length = %u", fname, adc_trace_length));

	/* Allocate or resize the ADC trace array as necessary. */

	if ( handel_mca->adc_trace_array == NULL ) {
		handel_mca->adc_trace_array = (unsigned long *)
			malloc( adc_trace_length * sizeof(unsigned long) );

		if ( handel_mca->adc_trace_array == (unsigned long *) NULL ) {
			handel_mca->adc_trace_length = 0;

			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory attempting to allocate a "
			"%u element ADC trace array for MCA '%s'.",
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
			"array to %u elements for MCA '%s'.",
				adc_trace_length, mca->record->name );
		}

		handel_mca->adc_trace_length = adc_trace_length;
	}

	/* Start the special run. */

	info[0] = 1.0;
	info[1] = handel_mca->adc_trace_step_size;

	void_ptr = (void *) &(info[0]);

	MX_XIA_SYNC( xiaDoSpecialRun( handel_mca->detector_channel,
					"adc_trace", void_ptr ) );

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

		ulong_max = (double) ULONG_MAX;

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

	MX_XIA_SYNC( xiaGetSpecialRunData( handel_mca->detector_channel,
					"adc_trace", void_ptr ) );

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
mxd_handel_mca_get_baseline_history_array( MX_MCA *mca )
{
	static const char fname[] =
			"mxd_handel_mca_get_baseline_history_array()";

	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL *handel;
	MX_RECORD_FIELD *field;
	int xia_status;
	void *void_ptr;
	int info[1];
	unsigned baseline_history_length;
	mx_status_type mx_status;

	mx_status = mxd_handel_mca_get_pointers( mca,
					&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for MCA '%s'.", fname, mca->record->name ));

	/* Get the current baseline history length. */

	void_ptr = (void *) &baseline_history_length;

	MX_XIA_SYNC( xiaGetSpecialRunData( handel_mca->detector_channel,
					"baseline_history_length", void_ptr ) );

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

	MX_DEBUG(-2,("%s: baseline_history_length = %u",
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
			"%u element baseline history array for MCA '%s'.",
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
			"array to %u elements for MCA '%s'.",
				baseline_history_length, mca->record->name );
	    }

	    handel_mca->baseline_history_length = baseline_history_length;
	}

	/* Start the special run. */

	info[0] = 1;

	void_ptr = (void *) &(info[0]);

	MX_XIA_SYNC( xiaDoSpecialRun( handel_mca->detector_channel,
					"baseline_history", void_ptr ) );

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

	MX_XIA_SYNC( xiaGetSpecialRunData( handel_mca->detector_channel,
					"baseline_history", void_ptr ) );

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

/* mxp_string_sort() is used as an argument to qsort() */

#if 0
static int
mxp_string_sort( const void *ptr1, const void *ptr2 )
{
	int result;
	const char **string1 = (const char **) ptr1;
	const char **string2 = (const char **) ptr2;

	result = strcmp( *string1, *string2 );

	return result;
}
#else
static int
mxp_string_sort( const void *ptr1, const void *ptr2 )
{
	/* Sigh.  The unions are to get around GCC's warning about
	 * "cast discards 'const' qualifier".  All I can say is that
	 * I didn't get to choose the datatypes involved.
	 */

	union {
		const char *ptr1;
		const char **string1;
	} union1;
	const char **string1;

	union {
		const char *ptr2;
		const char **string2;
	} union2;
	const char **string2;

	int result;

	union1.ptr1 = ptr1;
	string1 = union1.string1;

	union2.ptr2 = ptr2;
	string2 = union2.string2;

	result = strcmp( *string1, *string2 );

	return result;
}
#endif

#define MXP_HANDEL_PARAMETER_NAME_LENGTH	40

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_handel_mca_show_parameters( MX_MCA *mca )
{
	static const char fname[] = "mxd_handel_mca_show_parameters()";

	MX_HANDEL_MCA *handel_mca = NULL;
	MX_HANDEL *handel = NULL;
	unsigned short num_parameters;
	char parameter_name[200];
	unsigned short *parameter_values = NULL;
	unsigned short i;
	char **string_array;
	long dimension_array[2];
	size_t size_array[2];
	int xia_status;
	mx_status_type mx_status;

	mx_status = mxd_handel_mca_get_pointers( mca,
					&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_XIA_SYNC( xiaGetNumParams( handel_mca->detector_channel,
					&num_parameters ) );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"Cannot get the number of parameters for MCA '%s'.  "
			"Error code = %d, '%s'", 
					mca->record->name,
					xia_status,
					mxi_handel_strerror( xia_status ) );
	}

	mx_info( "-------------------------------------------------------" );
	mx_info( "Parameters for MCA '%s'   (%hu parameters)",
		mca->record->name, num_parameters );
	mx_info( " " );

	/* Allocate memory for the string array. */

	dimension_array[0] = num_parameters;
	dimension_array[1] = MXP_HANDEL_PARAMETER_NAME_LENGTH+1;

	size_array[0] = sizeof(char);
	size_array[1] = sizeof(char *);

	string_array = (char **)
	    mx_allocate_array( MXFT_STRING, 2, dimension_array, size_array );

	if ( string_array == (char **) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate a %hu element array of strings for MCA '%s'.",
			num_parameters, mca->record->name );
	}

	/* Allocate memory for the value array. */

	parameter_values = (unsigned short *)
			malloc( num_parameters * sizeof(unsigned short) );

	if ( parameter_values == (unsigned short *) NULL ) {
		mx_free_array( string_array );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate a %hu element array "
		"of parameter values for MCA '%s'.",
		num_parameters, mca->record->name );
	}

	/* Read the values from Handel. */

	MX_XIA_SYNC( xiaGetParamData( handel_mca->detector_channel,
					"values", (void *) parameter_values ) );

	if ( xia_status != XIA_SUCCESS ) {
		mx_free_array( string_array );
		mx_free( parameter_values );

		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
			"Cannot get the parameter values for MCA '%s'.  "
			"Error code = %d, '%s'", 
					mca->record->name,
					xia_status,
					mxi_handel_strerror( xia_status ) );
	}

	/* Format the parameter information. */

	for ( i = 0; i < num_parameters; i++ ) {
		MX_XIA_SYNC( xiaGetParamName( handel_mca->detector_channel,
						i, parameter_name ) );

		if ( xia_status != XIA_SUCCESS ) {
			snprintf( parameter_name, sizeof(parameter_name),
				"< parameter %hu >", i );
		}

		snprintf( string_array[i], MXP_HANDEL_PARAMETER_NAME_LENGTH,
			"%-16s = %hu", parameter_name, parameter_values[i] );
	}

	/* Discard the parameter_values array, since we no longer need it. */

	mx_free( parameter_values );

	/* Sort the string array. */

	qsort( string_array, num_parameters, sizeof(char *), mxp_string_sort );

	/* Display the sorted parameter information. */

	for ( i = 0; i < num_parameters; i++ ) {
		mx_info( string_array[i] );
	}

	/* We are done, so discard the string array. */

	mx_free_array( string_array );

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

#define MXP_ACQ_VALUE_NAME_LENGTH	40

/* Note: The following list was created on August 9, 2012 for the DXP-XMAP. */

static char acquisition_value_name_array[][MXP_ACQ_VALUE_NAME_LENGTH+1] =
{
	"adc_percent_rule",
	"baseline_average",
	"baseline_threshold",
	"calibration_energy",
	"detector_polarity",
	"dynamic_range",
	"energy_threshold",
	"gap_time",
	"maxwidth",
	"mca_bin_width",
	"minimum_gap_time",
	"number_mca_channels",
	"number_of_scas",
	"peaking_time",
	"preamp_gain",
	"preset_type",
	"preset_value",
	"reset_delay",
	"trigger_gap_time",
	"trigger_peaking_time",
	"trigger_threshold",
};

static unsigned long num_acquisition_value_names
	= sizeof(acquisition_value_name_array)
	/ sizeof(acquisition_value_name_array[0]);

MX_EXPORT mx_status_type
mxd_handel_mca_show_acquisition_values( MX_MCA *mca )
{
	static const char fname[] = "mxd_handel_mca_show_acquisition_values()";

	MX_HANDEL_MCA *handel_mca = NULL;
	MX_HANDEL *handel = NULL;
	unsigned long i;
	double acquisition_value;
	int xia_status;
	mx_status_type mx_status;

	mx_status = mxd_handel_mca_get_pointers( mca,
					&handel_mca, &handel, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_info( "-------------------------------------------------------" );
	mx_info( "Acquisition values for MCA '%s'   (%lu parameters)",
		mca->record->name, num_acquisition_value_names );
	mx_info( " " );

	for ( i = 0; i < num_acquisition_value_names; i++ ) {
		MX_XIA_SYNC( xiaGetAcquisitionValues(
				handel_mca->detector_channel,
				acquisition_value_name_array[i],
				(void *) &acquisition_value ) );

		if ( xia_status != XIA_SUCCESS ) {
			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		    "Cannot read the acquisition value '%s' from MCA '%s'.  "
		    "Error code = %d, '%s'", acquisition_value_name_array[i],
				mca->record->name, xia_status,
				mxi_handel_strerror( xia_status ) );
		}

		mx_info( "%-20s = %g", acquisition_value_name_array[i],
					acquisition_value );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

#ifndef MX_PROCESS_GET

#define MX_PROCESS_GET	1
#define MX_PROCESS_PUT	2

#endif


static mx_status_type
mxd_handel_mca_process_function( void *record_ptr,
				void *record_field_ptr,
				void *socket_handler_ptr,
				int operation )
{
	static const char fname[] = "mxd_handel_mca_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_MCA *mca;
	MX_HANDEL_MCA *handel_mca = NULL;
	MX_HANDEL *handel = NULL;
	unsigned long parameter_value;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	mca = (MX_MCA *) record->record_class_struct;

	mx_status = mxd_handel_mca_get_pointers( mca,
					&handel_mca, &handel, fname );

	MX_DEBUG( 2,("**** %s invoked, operation = %d, label_value = %ld ****",
		fname, operation, record_field->label_value));

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_HANDEL_MCA_PARAMETER_VALUE:

			mx_status = mxi_handel_read_parameter( mca,
						handel_mca->parameter_name,
						&parameter_value );

			handel_mca->parameter_value = parameter_value;

			MX_DEBUG( 2,
		("%s: Read mca '%s' parameter name = '%s', value = %lu",
				fname, mca->record->name,
				handel_mca->parameter_name,
				handel_mca->parameter_value));

			break;
		case MXLV_HANDEL_MCA_STATISTICS:

			mx_status = mxd_handel_mca_read_statistics( mca );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			handel_mca->statistics[ MX_HANDEL_MCA_REAL_TIME ]
						= mca->real_time;

			handel_mca->statistics[ MX_HANDEL_MCA_LIVE_TIME ]
						= mca->live_time;

			handel_mca->statistics[ MX_HANDEL_MCA_ENERGY_LIVE_TIME ]
						= handel_mca->energy_live_time;

			handel_mca->statistics[ MX_HANDEL_MCA_NUM_TRIGGERS ]
					= (double) handel_mca->num_triggers;

			handel_mca->statistics[ MX_HANDEL_MCA_NUM_EVENTS ]
					= (double) handel_mca->num_events;

			handel_mca->statistics[ MX_HANDEL_MCA_INPUT_COUNT_RATE ]
						= handel_mca->input_count_rate;

			handel_mca->statistics[ MX_HANDEL_MCA_OUTPUT_COUNT_RATE]
					= handel_mca->output_count_rate;

			handel_mca->statistics[ MX_HANDEL_MCA_NUM_UNDERFLOWS ]
					= (double) handel_mca->num_underflows;

			handel_mca->statistics[ MX_HANDEL_MCA_NUM_OVERFLOWS ]
					= (double) handel_mca->num_overflows;

			break;
		case MXLV_HANDEL_MCA_BASELINE_ARRAY:

#if MXD_HANDEL_MCA_DEBUG
			MX_DEBUG(-2,
			("%s: getting the adc trace array for mca '%s'",
				fname, mca->record->name ));
#endif

			mx_status = mxd_handel_mca_get_baseline_array( mca );
			break;
		case MXLV_HANDEL_MCA_ACQUISITION_VALUE:

#if MXD_HANDEL_MCA_DEBUG
			MX_DEBUG(-2,
			("%s: get acquisition value '%s' for mca '%s'",
				fname, handel_mca->acquisition_value_name,
				mca->record->name ));
#endif

			mx_status = mxd_handel_mca_get_acquisition_values( mca,
					handel_mca->acquisition_value_name,
					&(handel_mca->acquisition_value) );

#if MXD_HANDEL_MCA_DEBUG
			MX_DEBUG(-2,
			("%s: acquisition value '%s' for mca '%s' = %g",
				fname, handel_mca->acquisition_value_name,
				mca->record->name,
				handel_mca->acquisition_value));
#endif
			break;
		case MXLV_HANDEL_MCA_ADC_TRACE_ARRAY:

#if MXD_HANDEL_MCA_DEBUG
			MX_DEBUG(-2,
			("%s: getting the adc trace array for mca '%s'",
				fname, mca->record->name ));
#endif

			mx_status = mxd_handel_mca_get_adc_trace_array( mca );
			break;
		case MXLV_HANDEL_MCA_BASELINE_HISTORY_ARRAY:

#if MXD_HANDEL_MCA_DEBUG
			MX_DEBUG(-2,
			("%s: getting the adc trace array for mca '%s'",
				fname, mca->record->name ));
#endif

			mx_status = 
			    mxd_handel_mca_get_baseline_history_array( mca );
			break;
		default:
			MX_DEBUG(-1,(
			    "%s: *** Unknown MX_PROCESS_GET label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		case MXLV_HANDEL_MCA_PARAMETER_VALUE:

			MX_DEBUG( 2,
		("%s: Write mca '%s' parameter name = '%s', value = %lu",
				fname, mca->record->name,
				handel_mca->parameter_name,
				handel_mca->parameter_value));

			mx_status = mxi_handel_write_parameter( mca,
				handel_mca->parameter_name,
				handel_mca->parameter_value );

			break;
		case MXLV_HANDEL_MCA_PARAM_VALUE_TO_ALL_CHANNELS:
			MX_DEBUG( 2,
("%s: Write to all channels for mca '%s' parameter name = '%s', value = %lu",
				fname, mca->record->name,
				handel_mca->parameter_name,
				handel_mca->param_value_to_all_channels));

			mx_status = mxi_handel_write_parameter_to_all_channels(
				handel,
				handel_mca->parameter_name,
				handel_mca->param_value_to_all_channels );

			break;
		case MXLV_HANDEL_MCA_GAIN_CHANGE:

#if MXD_HANDEL_MCA_DEBUG
			MX_DEBUG(-2,
			("%s: gain change for mca '%s' to %g",
				fname, mca->record->name,
				handel_mca->gain_change ));
#endif

			mx_status = mxd_handel_mca_set_gain_change( mca );
			break;
		case MXLV_HANDEL_MCA_GAIN_CALIBRATION:

#if MXD_HANDEL_MCA_DEBUG
			MX_DEBUG(-2,
			("%s: set gain calibration for mca '%s' to %g",
				fname, mca->record->name,
				handel_mca->gain_calibration ));
#endif
			mx_status = mxd_handel_mca_set_gain_calibration( mca );
			break;
		case MXLV_HANDEL_MCA_ACQUISITION_VALUE:

#if MXD_HANDEL_MCA_DEBUG
			MX_DEBUG(-2,
			("%s: set acquisition value '%s' for mca '%s' to %g",
				fname, handel_mca->acquisition_value_name,
				mca->record->name,
				handel_mca->acquisition_value ));
#endif
			mx_status = mxd_handel_mca_set_acquisition_values( mca,
					handel_mca->acquisition_value_name,
					&(handel_mca->acquisition_value),
					FALSE );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = mxd_handel_mca_apply( mca );
			break;
		case MXLV_HANDEL_MCA_ACQUISITION_VALUE_TO_ALL:

#if MXD_HANDEL_MCA_DEBUG
			MX_DEBUG(-2,
			("%s: set acquisition value '%s' for all mcas to %g",
				fname, handel_mca->acquisition_value_name,
				handel_mca->acquisition_value ));
#endif
			mx_status =
			    mxi_handel_set_acquisition_values_for_all_channels(
					handel,
					handel_mca->acquisition_value_name,
					&(handel_mca->acquisition_value),
					FALSE );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = mxi_handel_apply_to_all_channels( handel );
			break;
		case MXLV_HANDEL_MCA_APPLY:

#if MXD_HANDEL_MCA_DEBUG
			MX_DEBUG(-2,("%s: apply for mca '%s'",
				fname, mca->record->name ));
#endif
			mx_status = mxi_handel_apply_to_all_channels( handel );
			break;
		case MXLV_HANDEL_MCA_APPLY_TO_ALL:

#if MXD_HANDEL_MCA_DEBUG
			MX_DEBUG(-2,("%s: apply to all MCAs.", fname));
#endif
			mx_status = mxi_handel_apply_to_all_channels( handel );
			break;
		case MXLV_HANDEL_MCA_SHOW_PARAMETERS:

#if MXD_HANDEL_MCA_DEBUG
			MX_DEBUG(-2,("%s: show all parameters for mca '%s'.",
 				fname, mca->record->name));
#endif
			mx_status = mxd_handel_mca_show_parameters( mca );
			break;
		case MXLV_HANDEL_MCA_SHOW_ACQUISITION_VALUES:

#if MXD_HANDEL_MCA_DEBUG
			MX_DEBUG(-2,
			("%s: show all acquisition values for mca '%s'.",
 				fname, mca->record->name));
#endif
			mx_status = mxd_handel_mca_show_acquisition_values(mca);
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
	}

	return mx_status;
}

