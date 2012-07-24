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
 * Copyright 2001-2006, 2008-2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_HANDEL_MCA_DEBUG			FALSE

#define MXD_HANDEL_MCA_DEBUG_STATISTICS		TRUE

#define MXD_HANDEL_MCA_DEBUG_TIMING		FALSE

#define MXD_HANDEL_MCA_DEBUG_DOUBLE_ROIS	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <float.h>

#include "mx_constants.h"
#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_hrt.h"
#include "mx_socket.h"
#include "mx_net.h"
#include "mx_mca.h"

#include <handel.h>
#include <handel_errors.h>
#include <handel_generic.h>

#include "i_handel.h"
#include "d_handel_mca.h"

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
	mxd_handel_mca_start,
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

#if MX_HANDEL_MCA_DEBUG_STATISTICS
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
						int operation );

/* A private function for the use of the driver. */

static mx_status_type
mxd_handel_mca_get_pointers( MX_MCA *mca,
			MX_HANDEL_MCA **handel_mca,
			const char *calling_fname )
{
	static const char fname[] = "mxd_handel_mca_get_pointers()";

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
	if ( mca->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_RECORD pointer for the MX_MCA pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*handel_mca = (MX_HANDEL_MCA *) mca->record->record_type_struct;

	if ( (*handel_mca)->handel_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The handel_record pointer for record '%s' is NULL.",
			mca->record->name );
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

	handel_mca->is_busy = NULL;
	handel_mca->get_run_data = NULL;
	handel_mca->get_acquisition_values = NULL;
	handel_mca->set_acquisition_values_for_all_channels = NULL;
	handel_mca->apply = NULL;
	handel_mca->read_parameter = NULL;
	handel_mca->write_parameter = NULL;
	handel_mca->write_parameter_to_all_channels = NULL;
	handel_mca->start_run = NULL;
	handel_mca->stop_run = NULL;
	handel_mca->read_spectrum = NULL;
	handel_mca->read_statistics = NULL;
	handel_mca->get_baseline_array = NULL;
	handel_mca->set_gain_change = NULL;
	handel_mca->set_gain_calibration = NULL;
	handel_mca->get_adc_trace_array = NULL;
	handel_mca->get_baseline_history_array = NULL;
	handel_mca->get_mx_parameter = NULL;
	handel_mca->set_mx_parameter = NULL;

	handel_mca->adc_trace_step_size = 5000.0;	/* in nanoseconds */
	handel_mca->adc_trace_length = 0;
	handel_mca->adc_trace_array = NULL;

	handel_mca->baseline_history_length = 0;
	handel_mca->baseline_history_array = NULL;

	handel_mca->mca_record_array_index = -1;

	handel_mca->firmware_code_variant = (unsigned long) -1;
	handel_mca->firmware_code_revision = (unsigned long) -1;
	handel_mca->hardware_scas_are_enabled = FALSE;

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
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	mca = (MX_MCA *) (record->record_class_struct);

	mx_status = mxd_handel_mca_get_pointers( mca, &handel_mca, fname );

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

	MX_HANDEL *handel;
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

	xia_status = xiaGetModuleItem( handel_mca->module_alias,
					"module_type",
					handel_mca->module_type );

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

	sprintf( item_name, "channel%ld_alias", module_channel );

	xia_status = xiaGetModuleItem( handel_mca->module_alias,
					item_name,
					&detector_channel_alt );

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

	xia_status = xiaStopRun( handel_mca->detector_channel );

	if ( xia_status != XIA_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot stop data acquisition for XIA DXP MCA '%s'.  "
		"Error code = %d, '%s'", handel_mca->record->name,
			xia_status, mxi_handel_strerror( xia_status ) );
	}

	/* Set up the interface function pointers. */

	handel_mca->is_busy = mxi_handel_is_busy;
	handel_mca->get_run_data = mxi_handel_get_run_data;
	handel_mca->get_acquisition_values =
					mxi_handel_get_acquisition_values;
	handel_mca->set_acquisition_values =
					mxi_handel_set_acquisition_values;
	handel_mca->set_acquisition_values_for_all_channels =
					mxi_handel_set_acq_for_all_channels;
	handel_mca->apply = mxi_handel_apply;
	handel_mca->read_parameter = mxi_handel_read_parameter;
	handel_mca->write_parameter = mxi_handel_write_parameter;
	handel_mca->write_parameter_to_all_channels
			= mxi_handel_write_parameter_to_all_channels;
	handel_mca->start_run = mxi_handel_start_run;
	handel_mca->stop_run = mxi_handel_stop_run;
	handel_mca->read_spectrum = mxi_handel_read_spectrum;
	handel_mca->read_statistics = mxi_handel_read_statistics;
	handel_mca->get_baseline_array = mxi_handel_get_baseline_array;

	handel_mca->set_gain_change = mxi_handel_set_gain_change;
	handel_mca->set_gain_calibration = mxi_handel_set_gain_calibration;
	handel_mca->get_adc_trace_array = mxi_handel_get_adc_trace_array;
	handel_mca->get_baseline_history_array =
				mxi_handel_get_baseline_history_array;

	handel_mca->get_mx_parameter = mxi_handel_get_mx_parameter;
	handel_mca->set_mx_parameter = mxi_handel_set_mx_parameter;

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

	/* See how many bins are in the spectrum. */

	xia_status = xiaGetRunData( handel_mca->detector_channel,
					"mca_length", &ulong_value );

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

	xia_status = xiaGetRunData( handel_mca->detector_channel,
					"baseline_length", &ulong_value );

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
	MX_RECORD *handel_record;
	MX_HANDEL *handel;
	unsigned long i, mca_number;
	int display_config = FALSE;
#if 0
	unsigned long codevar, coderev;
#endif
	mx_status_type mx_status;

#if MXD_HANDEL_MCA_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif
	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	mca = (MX_MCA *) (record->record_class_struct);

	mx_status = mxd_handel_mca_get_pointers( mca, &handel_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( handel_mca->debug_flag ) {
		MX_DEBUG(-2,("%s: debugging enabled for MCA '%s'",
			fname, record->name ));
	}

	handel_record = handel_mca->handel_record;

	if ( handel_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The handel_record pointer for record '%s' is NULL.",
			record->name );
	}

	/* Suppress GCC 'set but not used' warning. */
	display_config = display_config;

	mx_status = mxd_handel_mca_handel_open(mca, handel_mca, handel_record);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	handel = (MX_HANDEL *) handel_record->record_type_struct;

	display_config = handel->handel_flags &
			MXF_HANDEL_DISPLAY_CONFIGURATION_AT_STARTUP;

#if 0
	/* Find out what firmware variant and revision is used by this MCA. */

	mx_status = (handel_mca->read_parameter)( mca, "CODEVAR", &codevar );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	handel_mca->firmware_code_variant = (int) codevar;

	mx_status = (handel_mca->read_parameter)( mca, "CODEREV", &coderev );

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

		int xia_status;
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

		xia_status = xiaSetAcquisitionValues(
				handel_mca->detector_channel,
				"number_of_scas",
				(void *) &num_scas );

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

		int xia_status;
		double num_mx_channels;

		num_mx_channels = (double) mca->maximum_num_channels;

		xia_status = xiaSetAcquisitionValues(
					handel_mca->detector_channel,
					"number_mca_channels",
					(void *) &num_mx_channels );

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

	handel_mca->old_preset_type = (unsigned long) MX_ULONG_MAX;
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
	MX_RECORD *handel_record;
	MX_HANDEL *handel;
	MX_RECORD **mca_record_array;
	unsigned long i, num_mcas;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	mca = (MX_MCA *) (record->record_class_struct);

	mx_status = mxd_handel_mca_get_pointers( mca, &handel_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Delete our entry in mca_record_array. */

	handel_record = handel_mca->handel_record;

	handel = (MX_HANDEL *) handel_record->record_type_struct;

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
mxd_handel_mca_start( MX_MCA *mca )
{
	static const char fname[] = "mxd_handel_mca_start()";

	MX_HANDEL_MCA *handel_mca = NULL;
	unsigned long preset_type;
	double preset_type_as_double;
	double preset_time;
	mx_status_type mx_status;

	MX_RECORD *handel_record;
	MX_HANDEL *handel;

	mx_status = mxd_handel_mca_get_pointers( mca, &handel_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

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

	handel_record = handel_mca->handel_record;

	handel = (MX_HANDEL *) handel_record->record_type_struct;

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

		if ( handel_mca->start_run == NULL ) {
			return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"XIA interface record '%s' has not been initialized properly.",
				handel_mca->handel_record->name );
		}

		mx_status = (handel_mca->start_run)( mca, TRUE );

		/* We are done, so return to the caller. */

		return mx_status;
	}

	/* One or more of the preset parameters are different than
	 * before, so we must set them all.
	 */

	MX_DEBUG( 2,("%s: Presets will be changed.", fname));

	/******* Set the preset type. *******/

	if ( handel_mca->set_acquisition_values_for_all_channels == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
	    "Handel interface record '%s' has not been initialized properly.",
			handel_mca->handel_record->name );
	}

	preset_type_as_double = preset_type;

	mx_status = (handel_mca->set_acquisition_values_for_all_channels)( mca,
			"preset_type", &preset_type_as_double, TRUE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	handel_mca->old_preset_type = preset_type;

	/******* Set the timer preset. *******/

	mx_status = (handel_mca->set_acquisition_values_for_all_channels)( mca,
			"preset_value", &preset_time, TRUE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	handel_mca->old_preset_time = preset_time;

	handel_mca->new_statistics_available = TRUE;

	HANDEL_MCA_DEBUG_STATISTICS( handel_mca );

	/* Start the MCA. */

	if ( handel_mca->start_run == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"XIA interface record '%s' has not been initialized properly.",
			handel_mca->handel_record->name );
	}

	mx_status = (handel_mca->start_run)( mca, 1 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_handel_mca_stop( MX_MCA *mca )
{
	static const char fname[] = "mxd_handel_mca_stop()";

	MX_HANDEL_MCA *handel_mca = NULL;
	mx_status_type mx_status;

	mx_status = mxd_handel_mca_get_pointers( mca, &handel_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Stop the MCA. */

	if ( handel_mca->stop_run == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"XIA interface record '%s' has not been initialized properly.",
			handel_mca->handel_record->name );
	}

	mx_status = (handel_mca->stop_run)( mca );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_handel_mca_read( MX_MCA *mca )
{
	static const char fname[] = "mxd_handel_mca_read()";

	MX_HANDEL_MCA *handel_mca = NULL;
	mx_status_type mx_status;

	mx_status = mxd_handel_mca_get_pointers( mca, &handel_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( handel_mca->read_spectrum == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"XIA interface record '%s' has not been initialized properly.",
			handel_mca->handel_record->name );
	}

	mx_status = (handel_mca->read_spectrum)( mca );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_handel_mca_clear( MX_MCA *mca )
{
	static const char fname[] = "mxd_handel_mca_clear()";

	MX_HANDEL_MCA *handel_mca = NULL;
	mx_status_type mx_status;

	mx_status = mxd_handel_mca_get_pointers( mca, &handel_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* There doesn't appear to be a way of doing this without
	 * starting a run.
	 */

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_handel_mca_busy( MX_MCA *mca )
{
	static const char fname[] = "mxd_handel_mca_busy()";

	MX_HANDEL_MCA *handel_mca = NULL;
	mx_status_type mx_status;

	mx_status = mxd_handel_mca_get_pointers( mca, &handel_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( handel_mca->is_busy == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"XIA interface record '%s' has not been initialized properly.",
			handel_mca->handel_record->name );
	}

	mx_status = (handel_mca->is_busy)( mca, &(mca->busy) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mca->busy == FALSE ) {

		/* Send a stop run command so that the run enabled bit
		 * in the DXP is cleared.
		 */

		if ( handel_mca->stop_run == NULL ) {
			return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"XIA interface record '%s' has not been initialized properly.",
				handel_mca->handel_record->name );
		}

		mx_status = (handel_mca->stop_run)( mca );

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

	MX_HANDEL_MCA *handel_mca = NULL;
	mx_status_type mx_status;

	mx_status = mxd_handel_mca_get_pointers( mca, &handel_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s', parameter type '%s' (%ld).",
		fname, mca->record->name,
		mx_get_field_label_string(mca->record,mca->parameter_type),
		mca->parameter_type));

	if ( handel_mca->get_mx_parameter != NULL ) {
		mx_status = (handel_mca->get_mx_parameter)( mca );
	} else {
		mx_status = mxd_handel_mca_default_get_mx_parameter( mca );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_handel_mca_set_parameter( MX_MCA *mca )
{
	static const char fname[] = "mxd_handel_mca_set_parameter()";

	MX_HANDEL_MCA *handel_mca = NULL;
	mx_status_type mx_status;

	mx_status = mxd_handel_mca_get_pointers( mca, &handel_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s', parameter type '%s' (%ld).",
		fname, mca->record->name,
		mx_get_field_label_string(mca->record,mca->parameter_type),
		mca->parameter_type));

	if ( handel_mca->set_mx_parameter != NULL ) {
		mx_status = (handel_mca->set_mx_parameter)( mca );
	} else {
		mx_status = mxd_handel_mca_default_set_mx_parameter( mca );
	}

	return mx_status;
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

/* === */

MX_EXPORT mx_status_type
mxd_handel_mca_default_get_mx_parameter( MX_MCA *mca )
{
	static const char fname[] = "mxd_handel_mca_default_get_mx_parameter()";

	MX_HANDEL_MCA *handel_mca = NULL;
	unsigned long i, j;
	unsigned long roi[2];
	char name[20];
	double double_value;
	mx_status_type mx_status;

	mx_status = mxd_handel_mca_get_pointers( mca, &handel_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s', parameter type '%s' (%ld).",
		fname, mca->record->name,
		mx_get_field_label_string(mca->record,mca->parameter_type),
		mca->parameter_type));

	switch( mca->parameter_type ) {
	case MXLV_MCA_CURRENT_NUM_CHANNELS:

		if ( handel_mca->get_acquisition_values == NULL ) {
			return mx_error( MXE_INITIALIZATION_ERROR, fname,
	    "Handel interface record '%s' has not been initialized properly.",
				handel_mca->handel_record->name );
		}

		mx_status = (handel_mca->get_acquisition_values)( mca,
				"number_mca_channels", &double_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mca->current_num_channels = (long) double_value;

		if ( mca->current_num_channels > mca->maximum_num_channels ) {
			mx_warning(
"MCA '%s' reports that it currently has %ld channels.  "
"The MX record is currently configured for a maximum of %ld channels, "
"so channels %ld to %ld will be discarded.  "
"This probably means that the MX database is not correctly configured.",
				mca->record->name,
				mca->current_num_channels,
				mca->maximum_num_channels,
				mca->current_num_channels,
				mca->maximum_num_channels - 1 );

			mca->current_num_channels = mca->maximum_num_channels;
		}
		break;
	case MXLV_MCA_ROI:
		i = mca->roi_number;

		sprintf( name, "sca%lu_lo", (unsigned long) i );

		if ( handel_mca->get_acquisition_values == NULL ) {
			return mx_error( MXE_INITIALIZATION_ERROR, fname,
	    "Handel interface record '%s' has not been initialized properly.",
				handel_mca->handel_record->name );
		}

		mx_status = (handel_mca->get_acquisition_values)( mca,
						name, &double_value );

		mca->roi[0] = mx_round( double_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		sprintf( name, "sca%lu_hi", (unsigned long) i );

		mx_status = (handel_mca->get_acquisition_values)( mca,
						name, &double_value );

		mca->roi[1] = mx_round( double_value );

		break;
	case MXLV_MCA_ROI_INTEGRAL:
		i = mca->roi_number;

		/* Hardware SCA support is not used for this case. */

		mx_status = mx_mca_read( mca->record, NULL, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Make sure the ROI boundaries are correct. */

		mx_status = mx_mca_get_roi( mca->record, i, roi );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		MX_DEBUG( 2,
	("%s: Integrating MCA '%s' ROI %lu integral from bin %lu to bin %lu",
				fname, mca->record->name, (unsigned long) i,
				roi[0], roi[1]));

		mca->roi_integral = 0;

		for ( j = roi[0]; j <= roi[1]; j++ ) {
			mca->roi_integral += mca->channel_array[j];
		}

		MX_DEBUG( 2,("%s: ROI %lu integral = %lu",
			fname, (unsigned long) i, mca->roi_integral));

		break;
	case MXLV_MCA_LIVE_TIME:
	case MXLV_MCA_REAL_TIME:
	case MXLV_MCA_INPUT_COUNT_RATE:
	case MXLV_MCA_OUTPUT_COUNT_RATE:
		mx_status = mxd_handel_mca_read_statistics( mca, handel_mca );

		HANDEL_MCA_DEBUG_STATISTICS( handel_mca );

		MX_DEBUG( 2,
		("%s: new_statistics_available = %d, mca->busy = %d",
			fname, (int) handel_mca->new_statistics_available,
			(int) mca->busy ));

		break;
	default:
		return mx_mca_default_get_parameter_handler( mca );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_handel_mca_default_set_mx_parameter( MX_MCA *mca )
{
	static const char fname[] = "mxd_handel_mca_default_set_mx_parameter()";

	MX_HANDEL_MCA *handel_mca = NULL;
	unsigned long i;
	char name[20];
	double double_value;
	mx_status_type mx_status;

	mx_status = mxd_handel_mca_get_pointers( mca, &handel_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for MCA '%s', parameter type '%s' (%ld).",
		fname, mca->record->name,
		mx_get_field_label_string(mca->record,mca->parameter_type),
		mca->parameter_type));

	if ( handel_mca->set_acquisition_values == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
	    "Handel interface record '%s' has not been initialized properly.",
			handel_mca->handel_record->name );
	}

	if ( handel_mca->set_acquisition_values_for_all_channels == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
	    "Handel interface record '%s' has not been initialized properly.",
			handel_mca->handel_record->name );
	}

	switch( mca->parameter_type ) {
	case MXLV_MCA_CURRENT_NUM_CHANNELS:
		double_value = mca->current_num_channels;

		mx_status = 
		    (handel_mca->set_acquisition_values_for_all_channels)( mca,
				"number_mca_channels", &double_value, TRUE );

	case MXLV_MCA_ROI:
		i = mca->roi_number;

		sprintf( name, "sca%lu_lo", (unsigned long) i );

		double_value = mca->roi[0];

		mx_status = (handel_mca->set_acquisition_values)( mca,
					name, &double_value, FALSE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		sprintf( name, "sca%lu_hi", (unsigned long) i );

		double_value = mca->roi[1];

		mx_status = (handel_mca->set_acquisition_values)( mca,
					name, &double_value, FALSE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = (handel_mca->apply)( mca, -1 );
		break;

	case MXLV_MCA_ROI_ARRAY:
		for ( i = 0; i < mca->current_num_rois; i++ ) {

			sprintf( name, "sca%lu_lo", (unsigned long) i );

			double_value = mca->roi_array[i][0];

			mx_status = (handel_mca->set_acquisition_values)(
					mca, name, &double_value, FALSE );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			sprintf( name, "sca%lu_hi", (unsigned long) i );

			double_value = mca->roi_array[i][1];

			mx_status = (handel_mca->set_acquisition_values)(
					mca, name, &double_value, FALSE );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
		break;
	default:
		return mx_mca_default_set_parameter_handler( mca );
	}

	return MX_SUCCESSFUL_RESULT;
}

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

	mx_status = mxd_handel_mca_get_pointers( mca, &handel_mca, fname );

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

	mx_status = mxd_handel_mca_read_statistics( mca, handel_mca );

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

	mx_status = mxd_handel_mca_get_pointers( mca, &handel_mca, fname );

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

	mx_status = mxd_handel_mca_read_statistics( mca, handel_mca );

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

MX_EXPORT mx_status_type
mxd_handel_mca_read_statistics( MX_MCA *mca,
				MX_HANDEL_MCA *handel_mca )
{
	static const char fname[] = "mxd_handel_mca_read_statistics()";

	int read_statistics;
	mx_status_type mx_status;

	if ( mca->mca_flags & MXF_MCA_NO_READ_OPTIMIZATION ) {
		read_statistics = TRUE;

	} else if ( handel_mca->new_statistics_available ) {
		read_statistics = TRUE;

	} else {
		read_statistics = FALSE;
	}

	HANDEL_MCA_DEBUG_STATISTICS( handel_mca );

#if MXD_HANDEL_DEBUG_STATISTICS
	MX_DEBUG(-2,("%s: read_statistics = %d", fname, read_statistics));
#endif

	if ( read_statistics ) {

		if ( handel_mca->read_statistics == NULL ) {
			return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"XIA interface record '%s' has not been initialized properly.",
				handel_mca->handel_record->name );
		}

		mx_status = (handel_mca->read_statistics)( mca );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( mca->busy == FALSE ) {
			handel_mca->new_statistics_available = FALSE;
		}
	}

	HANDEL_MCA_DEBUG_STATISTICS( handel_mca );

	return MX_SUCCESSFUL_RESULT;
}

#ifndef MX_PROCESS_GET

#define MX_PROCESS_GET	1
#define MX_PROCESS_PUT	2

#endif


static mx_status_type
mxd_handel_mca_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mxd_handel_mca_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_MCA *mca;
	MX_HANDEL_MCA *handel_mca = NULL;
	unsigned long parameter_value;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	mca = (MX_MCA *) record->record_class_struct;
	handel_mca = (MX_HANDEL_MCA *) record->record_type_struct;

	MX_DEBUG( 2,("**** %s invoked, operation = %d, label_value = %ld ****",
		fname, operation, record_field->label_value));

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_HANDEL_MCA_PARAMETER_VALUE:
			if ( handel_mca->read_parameter == NULL ) {
				return mx_error( MXE_INITIALIZATION_ERROR,
					fname,
		"XIA interface record '%s' has not been initialized properly.",
					handel_mca->handel_record->name );
			}

			mx_status = (handel_mca->read_parameter)( mca,
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

			mx_status = mxd_handel_mca_read_statistics(
							mca, handel_mca );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			handel_mca->statistics[ MX_HANDEL_MCA_REAL_TIME ]
						= mca->real_time;

			handel_mca->statistics[ MX_HANDEL_MCA_LIVE_TIME ]
						= mca->live_time;

			handel_mca->statistics[ MX_HANDEL_MCA_INPUT_COUNT_RATE ]
						= handel_mca->input_count_rate;

			handel_mca->statistics[ MX_HANDEL_MCA_OUTPUT_COUNT_RATE]
					= handel_mca->output_count_rate;

			handel_mca->statistics[ MX_HANDEL_MCA_NUM_EVENTS ]
					= (double) handel_mca->num_events;

			break;
		case MXLV_HANDEL_MCA_BASELINE_ARRAY:

#if MXD_HANDEL_MCA_DEBUG
			MX_DEBUG(-2,
			("%s: getting the adc trace array for mca '%s'",
				fname, mca->record->name ));
#endif

			if ( handel_mca->get_baseline_array == NULL ) {
				return mx_error( MXE_UNSUPPORTED, fname,
		"Getting the baseline array is not supported for MCA '%s'.",
					mca->record->name );
			}

			mx_status = (handel_mca->get_baseline_array)( mca );
			break;
		case MXLV_HANDEL_MCA_ACQUISITION_VALUE:

#if MXD_HANDEL_MCA_DEBUG
			MX_DEBUG(-2,
			("%s: get acquisition value '%s' for mca '%s'",
				fname, handel_mca->acquisition_value_name,
				mca->record->name ));
#endif

			if ( handel_mca->get_acquisition_values == NULL ) {
				return mx_error( MXE_UNSUPPORTED, fname,
		"Getting acquisition values is not supported for MCA '%s'.",
					mca->record->name );
			}

			mx_status = (handel_mca->get_acquisition_values)( mca,
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

			if ( handel_mca->get_adc_trace_array == NULL ) {
				return mx_error( MXE_UNSUPPORTED, fname,
		"Getting the adc trace array is not supported for MCA '%s'.",
					mca->record->name );
			}

			mx_status = (handel_mca->get_adc_trace_array)( mca );
			break;
		case MXLV_HANDEL_MCA_BASELINE_HISTORY_ARRAY:

#if MXD_HANDEL_MCA_DEBUG
			MX_DEBUG(-2,
			("%s: getting the adc trace array for mca '%s'",
				fname, mca->record->name ));
#endif

			if ( handel_mca->get_baseline_history_array == NULL ) {
				return mx_error( MXE_UNSUPPORTED, fname,
	"Getting the baseline history array is not supported for MCA '%s'.",
					mca->record->name );
			}

			mx_status = 
				(handel_mca->get_baseline_history_array)( mca );
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

			if ( handel_mca->write_parameter == NULL ) {
				return mx_error( MXE_INITIALIZATION_ERROR,
					fname,
		"XIA interface record '%s' has not been initialized properly.",
					handel_mca->handel_record->name );
			}

			mx_status = (handel_mca->write_parameter)( mca,
				handel_mca->parameter_name,
				handel_mca->parameter_value );

			break;
		case MXLV_HANDEL_MCA_PARAM_VALUE_TO_ALL_CHANNELS:
			MX_DEBUG( 2,
("%s: Write to all channels for mca '%s' parameter name = '%s', value = %lu",
				fname, mca->record->name,
				handel_mca->parameter_name,
				handel_mca->param_value_to_all_channels));

			if ( handel_mca->write_parameter_to_all_channels
				== NULL )
			{
				return mx_error( MXE_INITIALIZATION_ERROR,
					fname,
		"XIA interface record '%s' has not been initialized properly.",
					handel_mca->handel_record->name );
			}

			mx_status =
			    (handel_mca->write_parameter_to_all_channels)(
				mca,
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

			if ( handel_mca->set_gain_change == NULL ) {
				return mx_error( MXE_UNSUPPORTED, fname,
			"Changing the gain is not supported for MCA '%s'.",
					mca->record->name );
			}

			mx_status = (handel_mca->set_gain_change)( mca );
			break;
		case MXLV_HANDEL_MCA_GAIN_CALIBRATION:

#if MXD_HANDEL_MCA_DEBUG
			MX_DEBUG(-2,
			("%s: set gain calibration for mca '%s' to %g",
				fname, mca->record->name,
				handel_mca->gain_calibration ));
#endif

			if ( handel_mca->set_gain_calibration == NULL ) {
				return mx_error( MXE_UNSUPPORTED, fname,
		"Setting the gain calibration is not supported for MCA '%s'.",
					mca->record->name );
			}

			mx_status = (handel_mca->set_gain_calibration)( mca );
			break;
		case MXLV_HANDEL_MCA_ACQUISITION_VALUE:

#if MXD_HANDEL_MCA_DEBUG
			MX_DEBUG(-2,
			("%s: set acquisition value '%s' for mca '%s' to %g",
				fname, handel_mca->acquisition_value_name,
				mca->record->name,
				handel_mca->acquisition_value ));
#endif

			if ( handel_mca->set_acquisition_values == NULL ) {
				return mx_error( MXE_UNSUPPORTED, fname,
		"Setting acquisition values is not supported for MCA '%s'.",
					mca->record->name );
			}

			mx_status = (handel_mca->set_acquisition_values)( mca,
					handel_mca->acquisition_value_name,
					&(handel_mca->acquisition_value),
					FALSE );
			break;
		case MXLV_HANDEL_MCA_ACQUISITION_VALUE_TO_ALL:

#if MXD_HANDEL_MCA_DEBUG
			MX_DEBUG(-2,
			("%s: set acquisition value '%s' for all mcas to %g",
				fname, handel_mca->acquisition_value_name,
				handel_mca->acquisition_value ));
#endif

			if ( handel_mca->set_acquisition_values == NULL ) {
				return mx_error( MXE_UNSUPPORTED, fname,
		"Setting acquisition values is not supported for MCA '%s'.",
					mca->record->name );
			}

			mx_status = 
			  (handel_mca->set_acquisition_values_for_all_channels)(
					mca,
					handel_mca->acquisition_value_name,
					&(handel_mca->acquisition_value),
					FALSE );
			break;
		case MXLV_HANDEL_MCA_APPLY:

#if MXD_HANDEL_MCA_DEBUG
			MX_DEBUG(-2,("%s: apply for mca '%s'",
				fname, mca->record->name ));
#endif

			if ( handel_mca->apply == NULL ) {
				return mx_error( MXE_UNSUPPORTED, fname,
				"Apply is not supported for MCA '%s'.",
					mca->record->name );
			}

			mx_status = (handel_mca->apply)( mca, FALSE );
			break;
		case MXLV_HANDEL_MCA_APPLY_TO_ALL:

#if MXD_HANDEL_MCA_DEBUG
			MX_DEBUG(-2,("%s: apply to all MCAs.", fname));
#endif

			if ( handel_mca->apply == NULL ) {
				return mx_error( MXE_UNSUPPORTED, fname,
				"Apply is not supported for MCA '%s'.",
					mca->record->name );
			}

			mx_status = (handel_mca->apply)( mca, TRUE );
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

