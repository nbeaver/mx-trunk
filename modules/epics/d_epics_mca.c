/*
 * Name:    d_epics_mca.c
 *
 * Purpose: MX multichannel analyzer driver for EPICS multichannel analyzers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002-2003, 2006, 2008-2012, 2014-2015
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_EPICS_MCA_DEBUG				FALSE

#define MXD_EPICS_MCA_ENABLE_ACQUIRING_PV_CALLBACK	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_epics.h"
#include "mx_mca.h"
#include "d_epics_mca.h"

/* Initialize the MCA driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_epics_mca_record_function_list = {
	mxd_epics_mca_initialize_driver,
	mxd_epics_mca_create_record_structures,
	mxd_epics_mca_finish_record_initialization,
	mxd_epics_mca_delete_record,
	mxd_epics_mca_print_structure,
	mxd_epics_mca_open,
	NULL
};

MX_MCA_FUNCTION_LIST mxd_epics_mca_mca_function_list = {
	mxd_epics_mca_start,
	mxd_epics_mca_stop,
	mxd_epics_mca_read,
	mxd_epics_mca_clear,
	mxd_epics_mca_busy,
	mxd_epics_mca_get_parameter,
	mxd_epics_mca_set_parameter,
};

MX_RECORD_FIELD_DEFAULTS mxd_epics_mca_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_MCA_STANDARD_FIELDS,
	MXD_EPICS_MCA_STANDARD_FIELDS
};

long mxd_epics_mca_num_record_fields
		= sizeof( mxd_epics_mca_record_field_defaults )
		  / sizeof( mxd_epics_mca_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_epics_mca_rfield_def_ptr
			= &mxd_epics_mca_record_field_defaults[0];

/* Private functions for the use of the driver. */

static mx_status_type
mxd_epics_mca_get_pointers( MX_MCA *mca,
			MX_EPICS_MCA **epics_mca,
			const char *calling_fname )
{
	static const char fname[] = "mxd_epics_mca_get_pointers()";

	if ( mca == (MX_MCA *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The mca pointer passed by '%s' was NULL",
			calling_fname );
	}

	if ( mca->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_RECORD pointer for mca pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( epics_mca != (MX_EPICS_MCA **) NULL ) {

		*epics_mca = (MX_EPICS_MCA *)
				(mca->record->record_type_struct);

		if ( *epics_mca == (MX_EPICS_MCA *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_EPICS_MCA pointer for mca record '%s' passed by '%s' is NULL",
				mca->record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

#if MXD_EPICS_MCA_ENABLE_ACQUIRING_PV_CALLBACK

static mx_status_type
mxd_epics_mca_acquiring_pv_callback( MX_EPICS_CALLBACK *cb, void *args )
{
	static const char fname[] = "mxd_epics_mca_acquiring_pv_callback()";

	MX_RECORD *record;
	MX_MCA *mca;
	MX_EPICS_MCA *epics_mca;
	const long *value_ptr;
	long value;

	record = (MX_RECORD *) args;

	value_ptr = cb->value_ptr;

	value = *value_ptr;

	MX_DEBUG(-2,("%s invoked for MCA '%s', value = %ld",
		fname, record->name, value ));

	mca = (MX_MCA *) record->record_class_struct;

	epics_mca = (MX_EPICS_MCA *) record->record_type_struct;

	if ( value != 0 ) {
		mca->new_data_available = TRUE;
	}

	return MX_SUCCESSFUL_RESULT;
}

#endif   /* MXD_EPICS_MCA_ENABLE_ACQUIRING_PV_CALLBACK */

/* === */

MX_EXPORT mx_status_type
mxd_epics_mca_initialize_driver( MX_DRIVER *driver )
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
mxd_epics_mca_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_epics_mca_create_record_structures()";

	MX_MCA *mca;
	MX_EPICS_MCA *epics_mca = NULL;

	/* Allocate memory for the necessary structures. */

	mca = (MX_MCA *) malloc( sizeof(MX_MCA) );

	if ( mca == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MCA structure." );
	}

	epics_mca = (MX_EPICS_MCA *) malloc( sizeof(MX_EPICS_MCA) );

	if ( epics_mca == (MX_EPICS_MCA *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_EPICS_MCA structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = mca;
	record->record_type_struct = epics_mca;
	record->class_specific_function_list
		= &mxd_epics_mca_mca_function_list;

	mca->record = record;
	epics_mca->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_mca_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_epics_mca_finish_record_initialization()";

	MX_MCA *mca;
	MX_RECORD *list_head_record, *current_record;
	MX_EPICS_MCA *epics_mca, *current_epics_mca;
	MX_RECORD_FIELD *field;
	MX_DRIVER *our_driver;
	long i, num_mcas, our_mx_type;
	char *detector_name, *current_detector_name;
	mx_status_type mx_status;

	mca = (MX_MCA *) record->record_class_struct;

	mca->channel_array = ( unsigned long * )
		malloc( mca->maximum_num_channels * sizeof( unsigned long ) );

	if ( mca->channel_array == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory allocating an %lu channel data array.",
			mca->maximum_num_channels );
	}

	mx_status = mx_mca_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	strlcpy( record->network_type_name, "epics",
				MXU_NETWORK_TYPE_NAME_LENGTH );

	/* Now we must find all of the MCA records that are part of the
	 * same multi-element MCA system.
	 */

	epics_mca = record->record_type_struct;

	detector_name = epics_mca->epics_detector_name;

	list_head_record = record->list_head;

	/* Get the driver type for our driver. */

	our_driver = mx_get_driver_for_record( record );

	if ( our_driver == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Could not find the driver for record '%s'.", record->name );
	}

	our_mx_type = our_driver->mx_type;

	/* On our first pass, we find out how many matching MCAs are present. */

	num_mcas = 0;

	current_record = list_head_record->next_record;

	while ( current_record != list_head_record ) {
		if ( current_record->mx_type == our_mx_type ) {
			current_epics_mca = current_record->record_type_struct;

			current_detector_name =
				current_epics_mca->epics_detector_name;

			if ( strcmp(detector_name, current_detector_name) == 0 )
			{
				num_mcas++;
			}
		}

		current_record = current_record->next_record;
	}

	epics_mca->num_associated_mcas = num_mcas;

	/* Setup the associated_mca_record_array field. */

	epics_mca->associated_mca_record_array = (MX_RECORD **)
				malloc( num_mcas * sizeof(MX_RECORD *) );

	if ( epics_mca->associated_mca_record_array == (MX_RECORD **) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %ld element "
		"array of MX_RECORD pointers.", num_mcas );
	}

	mx_status = mx_find_record_field( record,
			"associated_mca_record_array", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = num_mcas;

	/* On our second pass, we add the matching MCAs to our array. */

	i = 0;

	current_record = list_head_record->next_record;

	while ( current_record != list_head_record ) {
		if ( current_record->mx_type == our_mx_type ) {
			current_epics_mca = current_record->record_type_struct;

			current_detector_name =
				current_epics_mca->epics_detector_name;

			if ( strcmp(detector_name, current_detector_name) == 0 )
			{
				if ( i >= num_mcas ) {
					return mx_error(
					MXE_CORRUPT_DATA_STRUCTURE, fname,
					"On our second pass through the "
					"record list we found more matching "
					"MCAs for MCA '%s' than we did during "
					"our first pass.  This should never "
					"happen.", record->name );
				}

				epics_mca->associated_mca_record_array[i]
					= current_record;

				i++;
			}
		}

		current_record = current_record->next_record;
	}

#if MXD_EPICS_MCA_DEBUG
	MX_DEBUG(-2,("%s: num_mcas = %ld", fname, num_mcas));

	for ( i = 0; i < num_mcas; i++ ) {
		MX_DEBUG(-2,("%s:   MCA[%ld] = '%s'", fname, i, 
			epics_mca->associated_mca_record_array[i]->name));
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_mca_delete_record( MX_RECORD *record )
{
	MX_MCA *mca;

	if ( record == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}
	if ( record->record_type_struct != NULL ) {

		free( record->record_type_struct );

		record->record_type_struct = NULL;
	}
	mca = (MX_MCA *) record->record_class_struct;

	if ( mca != NULL ) {

		if ( mca->channel_array != NULL ) {
			free( mca->channel_array );

			mca->channel_array = NULL;
		}
		free( record->record_class_struct );

		record->record_class_struct = NULL;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_mca_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_epics_mca_print_structure()";

	MX_MCA *mca;
	MX_EPICS_MCA *epics_mca = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	mca = (MX_MCA *) (record->record_class_struct);

	if ( mca == (MX_MCA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_MCA pointer for record '%s' is NULL.", record->name);
	}

	epics_mca = (MX_EPICS_MCA *) (record->record_type_struct);

	if ( epics_mca == (MX_EPICS_MCA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_EPICS_MCA pointer for record '%s' is NULL.", record->name);
	}

	fprintf(file, "MCA parameters for record '%s':\n", record->name);

	fprintf(file, "  MCA type              = EPICS_MCA.\n\n");
	fprintf(file, "  EPICS detector name   = %s\n",
					epics_mca->epics_detector_name);
	fprintf(file, "  EPICS mca name        = %s\n",
					epics_mca->epics_mca_name);
	fprintf(file, "  maximum # of channels = %lu\n",
					mca->maximum_num_channels);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_mca_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_epics_mca_open()";

	MX_MCA *mca;
	MX_EPICS_MCA *epics_mca = NULL;
	unsigned long i, flags, connect_flags;
	double version_number;
	char pvname[50];
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	mca = (MX_MCA *) record->record_class_struct;

	mx_status = mxd_epics_mca_get_pointers(mca, &epics_mca, fname);

	if ( mx_status.code != MXE_SUCCESS ) {
		mca->busy = FALSE;

		return mx_status;
	}

	snprintf( pvname, sizeof(pvname), "%s%s.VERS",
			epics_mca->epics_detector_name,
			epics_mca->epics_mca_name );

	mx_status = mx_caget_by_name(pvname, MX_CA_DOUBLE, 1, &version_number);

	if ( mx_status.code != MXE_SUCCESS ) {
		epics_mca->epics_record_version = 0.0;
	} else {
		epics_mca->epics_record_version = version_number;
	}

	/* Set some reasonable defaults. */

	mca->current_num_channels = mca->maximum_num_channels;
	mca->current_num_rois = mca->maximum_num_rois;

	for ( i = 0; i < mca->maximum_num_rois; i++ ) {
		mca->roi_array[i][0] = 0;
		mca->roi_array[i][1] = 0;
	}

	mca->preset_type = MXF_MCA_PRESET_LIVE_TIME;
	mca->parameter_type = 0;

	mca->roi[0] = 0;
	mca->roi[1] = 0;
	mca->roi_integral = 0;

	mca->channel_number = 0;
	mca->channel_value = 0;

	mca->real_time = 0.0;
	mca->live_time = 0.0;

	mca->preset_real_time = 0.0;
	mca->preset_live_time = 0.0;
	mca->preset_count = 0;

	/* Initialize MX EPICS data structures for all of the process
	 * variables we might use.
	 *
	 * Please note that the following initialization steps do not
	 * invoke Channel Access and cause _no_ network I/O.  All they
	 * do is create the string representation of the process variable
	 * names for later use.
	 */

	flags = epics_mca->epics_mca_flags;

	if ( flags & MXF_EPICS_MCA_DISABLE_READ_OPTIMIZATION ) {
		mca->mca_flags |= MXF_MCA_NO_READ_OPTIMIZATION;
	}

	if ( flags & MXF_EPICS_MCA_MULTIELEMENT_DETECTOR ) {

		mx_epics_pvname_init( &(epics_mca->acquiring_pv),
			"%sAcquiring.VAL", epics_mca->epics_detector_name );

		mx_epics_pvname_init( &(epics_mca->erase_pv),
			"%sEraseAll.VAL", epics_mca->epics_detector_name );

		mx_epics_pvname_init( &(epics_mca->erase_start_pv),
			"%sEraseStart.VAL", epics_mca->epics_detector_name);

		mx_epics_pvname_init( &(epics_mca->preset_live_pv),
			"%sPresetLive.VAL", epics_mca->epics_detector_name );

		mx_epics_pvname_init( &(epics_mca->preset_real_pv),
			"%sPresetReal.VAL", epics_mca->epics_detector_name );

		mx_epics_pvname_init( &(epics_mca->stop_pv),
			"%sStopAll.VAL", epics_mca->epics_detector_name );

		mx_epics_pvname_init( &(epics_mca->start_pv),
			"%sStartAll.VAL", epics_mca->epics_detector_name );
	} else {
		mx_epics_pvname_init( &(epics_mca->acquiring_pv),
			"%s%s.ACQG", epics_mca->epics_detector_name,
					epics_mca->epics_mca_name );

		mx_epics_pvname_init( &(epics_mca->erase_pv),
			"%s%s.ERAS", epics_mca->epics_detector_name,
					epics_mca->epics_mca_name );

		mx_epics_pvname_init( &(epics_mca->erase_start_pv),
			"%s%s.ERST", epics_mca->epics_detector_name,
					epics_mca->epics_mca_name );

		mx_epics_pvname_init( &(epics_mca->preset_live_pv),
			"%s%s.PLTM", epics_mca->epics_detector_name,
					epics_mca->epics_mca_name );

		mx_epics_pvname_init( &(epics_mca->preset_real_pv),
			"%s%s.PRTM", epics_mca->epics_detector_name,
					epics_mca->epics_mca_name );

		mx_epics_pvname_init( &(epics_mca->stop_pv),
			"%s%s.STOP", epics_mca->epics_detector_name,
					epics_mca->epics_mca_name );

		mx_epics_pvname_init( &(epics_mca->start_pv),
			"%s%s.STRT", epics_mca->epics_detector_name,
					epics_mca->epics_mca_name );
	}

	mx_epics_pvname_init( &(epics_mca->act_pv),
			"%s%s.ACT", epics_mca->epics_detector_name,
					epics_mca->epics_mca_name );

	mx_epics_pvname_init( &(epics_mca->eltm_pv),
			"%s%s.ELTM", epics_mca->epics_detector_name,
					epics_mca->epics_mca_name );

	mx_epics_pvname_init( &(epics_mca->ertm_pv),
			"%s%s.ERTM", epics_mca->epics_detector_name,
					epics_mca->epics_mca_name );

	mx_epics_pvname_init( &(epics_mca->nuse_pv),
			"%s%s.NUSE", epics_mca->epics_detector_name,
					epics_mca->epics_mca_name );

	mx_epics_pvname_init( &(epics_mca->pct_pv),
			"%s%s.PCT", epics_mca->epics_detector_name,
					epics_mca->epics_mca_name );

	mx_epics_pvname_init( &(epics_mca->pcth_pv),
			"%s%s.PCTH", epics_mca->epics_detector_name,
					epics_mca->epics_mca_name );

	mx_epics_pvname_init( &(epics_mca->pctl_pv),
			"%s%s.PCTL", epics_mca->epics_detector_name,
					epics_mca->epics_mca_name );

	mx_epics_pvname_init( &(epics_mca->val_pv),
			"%s%s.VAL", epics_mca->epics_detector_name,
					epics_mca->epics_mca_name );

	epics_mca->num_roi_pvs = mca->maximum_num_rois;

	epics_mca->roi_low_pv_array = (MX_EPICS_PV *)
		malloc( epics_mca->num_roi_pvs * sizeof(MX_EPICS_PV) );

	if ( epics_mca->roi_low_pv_array == (MX_EPICS_PV *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Ran out of memory allocating %ld elements for roi_low_pv_array "
	"used by MCA record '%s'.", epics_mca->num_roi_pvs, record->name );
	}

	epics_mca->roi_high_pv_array = (MX_EPICS_PV *)
		malloc( epics_mca->num_roi_pvs * sizeof(MX_EPICS_PV) );

	if ( epics_mca->roi_high_pv_array == (MX_EPICS_PV *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Ran out of memory allocating %ld elements for roi_high_pv_array "
	"used by MCA record '%s'.", epics_mca->num_roi_pvs, record->name );
	}

	epics_mca->roi_integral_pv_array = (MX_EPICS_PV *)
		malloc( epics_mca->num_roi_pvs * sizeof(MX_EPICS_PV) );

	if ( epics_mca->roi_integral_pv_array == (MX_EPICS_PV *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Ran out of memory allocating %ld elements for roi_integral_pv_array "
	"used by MCA record '%s'.", epics_mca->num_roi_pvs, record->name );
	}

	epics_mca->roi_background_pv_array = (MX_EPICS_PV *)
		malloc( epics_mca->num_roi_pvs * sizeof(MX_EPICS_PV) );

	if ( epics_mca->roi_background_pv_array == (MX_EPICS_PV *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Ran out of memory allocating %ld elements for roi_background_pv_array "
	"used by MCA record '%s'.", epics_mca->num_roi_pvs, record->name );
	}

	for ( i = 0; i < epics_mca->num_roi_pvs; i++ ) {
		mx_epics_pvname_init( &(epics_mca->roi_low_pv_array[i]),
			"%s%s.R%ldLO", epics_mca->epics_detector_name,
					epics_mca->epics_mca_name, i );

		mx_epics_pvname_init( &(epics_mca->roi_high_pv_array[i]),
			"%s%s.R%ldHI", epics_mca->epics_detector_name,
					epics_mca->epics_mca_name, i );

		mx_epics_pvname_init( &(epics_mca->roi_integral_pv_array[i]),
			"%s%s.R%ld", epics_mca->epics_detector_name,
					epics_mca->epics_mca_name, i );

		mx_epics_pvname_init( &(epics_mca->roi_background_pv_array[i]),
			"%s%s.R%ldBG", epics_mca->epics_detector_name,
					epics_mca->epics_mca_name, i );
	}

	/* Test to see if this MCA is an XIA DXP MCA. */

	if ( strlen( epics_mca->epics_dxp_name ) == 0 ) {
		epics_mca->have_dxp_record = FALSE;
	} else {
		if ( flags & MXF_EPICS_MCA_DXP_RECORD_IS_NOT_USED ) {
			epics_mca->have_dxp_record = FALSE;
		} else
		if ( flags & MXF_EPICS_MCA_DXP_RECORD_IS_USED ) {
			epics_mca->have_dxp_record = TRUE;
		} else {
			MX_EPICS_PV pv;

			mx_epics_pvname_init( &pv, "%s%s:InputCountRate.VAL",
					epics_mca->epics_detector_name,
					epics_mca->epics_dxp_name );

			connect_flags = MXF_EPVC_WAIT_FOR_CONNECTION
							| MXF_EPVC_QUIET;

			mx_status = mx_epics_pv_connect( &pv, connect_flags );

			if ( mx_status.code == MXE_SUCCESS ) {
				epics_mca->have_dxp_record = FALSE;
			} else {
				epics_mca->have_dxp_record = TRUE;
			}

			(void) mx_epics_pv_disconnect( &pv );
		}
	}

#if 0
	MX_DEBUG(-2,("%s: mca '%s' have_dxp_record = %d",
		fname, epics_mca->record->name,
		(int) epics_mca->have_dxp_record ));
#endif

	if ( epics_mca->have_dxp_record == FALSE ) {
		mx_epics_pvname_init( &(epics_mca->icr_pv),
			"%s%s:InputCountRate.VAL",
				epics_mca->epics_detector_name,
				epics_mca->epics_dxp_name );

		mx_epics_pvname_init( &(epics_mca->ocr_pv),
			"%s%s:OutputCountRate.VAL",
				epics_mca->epics_detector_name,
				epics_mca->epics_dxp_name );
	} else {
		mx_epics_pvname_init( &(epics_mca->icr_pv),
			"%s%s.ICR", epics_mca->epics_detector_name,
				epics_mca->epics_dxp_name );

		mx_epics_pvname_init( &(epics_mca->ocr_pv),
			"%s%s.OCR", epics_mca->epics_detector_name,
				epics_mca->epics_dxp_name );
	}

#if MXD_EPICS_MCA_ENABLE_ACQUIRING_PV_CALLBACK
	{
		MX_EPICS_CALLBACK *callback;

		/* Establish an EPICS callback for the Acquiring PV. */

		mx_status = mx_epics_add_callback( &(epics_mca->acquiring_pv),
					MX_CA_EVENT_VALUE,
					mxd_epics_mca_acquiring_pv_callback,
					record,
					&callback );
	}
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_mca_start( MX_MCA *mca )
{
	static const char fname[] = "mxd_epics_mca_start()";

	MX_EPICS_MCA *epics_mca = NULL;
	MX_EPICS_GROUP epics_group;
	MX_RECORD *current_record;
	MX_MCA *current_mca;
	int32_t start, preset_counts;
	double preset_real_time, preset_live_time;
	long i;
	unsigned long flags;
	mx_status_type mx_status;

	mx_status = mxd_epics_mca_get_pointers( mca, &epics_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the 'new_data_available' flag for all of the associated MCAs. */

	for ( i = 0; i < epics_mca->num_associated_mcas; i++ ) {
		current_record = epics_mca->associated_mca_record_array[i];

		if ( current_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_RECORD pointer for associated MCA %ld was "
			"NULL for MCA '%s'.",
				i, mca->record->name );
		}

		current_mca = (MX_MCA *) current_record->record_class_struct;

		if ( current_mca == (MX_MCA *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_MCA pointer for associated MCA '%s' was "
			"NULL for MCA '%s'.",
				current_record->name,
				mca->record->name );
		}

		current_mca->new_data_available = TRUE;
	}

	/* Set the preset interval appropriately depending on the
	 * preset type.
	 */

	switch( mca->preset_type ) {
	case MXF_MCA_PRESET_NONE:
		preset_live_time = 0.0;
		preset_real_time = 0.0;
		preset_counts = 0;
		break;
	case MXF_MCA_PRESET_LIVE_TIME:
		preset_live_time = mca->preset_live_time;
		preset_real_time = 0.0;
		preset_counts = 0;
		break;
	case MXF_MCA_PRESET_REAL_TIME:
		preset_live_time = 0.0;
		preset_real_time = mca->preset_real_time;
		preset_counts = 0;
		break;
	case MXF_MCA_PRESET_COUNT:
		preset_live_time = 0.0;
		preset_real_time = 0.0;
		preset_counts = 0;
		break;
		
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unsupported preset type %ld for MCA '%s'.",
			mca->preset_type, mca->record->name );
		break;
	}

	/* Save a group of EPICS commands to be started all at once. */

	mx_status = mx_epics_start_group( &epics_group );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Do the actual preset changes. */

	mx_status = mx_group_caput( &epics_group, &(epics_mca->preset_live_pv),
				MX_CA_DOUBLE, 1, &preset_live_time );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_epics_delete_group( &epics_group );

		return mx_status;
	}

	mx_status = mx_group_caput( &epics_group, &(epics_mca->preset_real_pv),
				MX_CA_DOUBLE, 1, &preset_real_time );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_epics_delete_group( &epics_group );

		return mx_status;
	}

	mx_status = mx_group_caput( &epics_group, &(epics_mca->pct_pv),
				MX_CA_LONG, 1, &preset_counts );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_epics_delete_group( &epics_group );

		return mx_status;
	}

	/* Send all of the commands in the group. */

	mx_status = mx_epics_end_group( &epics_group );

	/* Tell the counting to start. */

	flags = epics_mca->epics_mca_flags;

	start = 1;

	if ( flags & MXF_EPICS_MCA_WAIT_ON_START ) {

		if ( flags & MXF_EPICS_MCA_NO_ERASE_ON_START ) {

			mx_status = mx_caput_with_callback(
						&(epics_mca->start_pv),
						MX_CA_LONG, 1, &start,
						NULL, NULL );
		} else {
			mx_status = mx_caput_with_callback(
						&(epics_mca->erase_start_pv),
						MX_CA_LONG, 1, &start,
						NULL, NULL );
		}
	} else {
		if ( flags & MXF_EPICS_MCA_NO_ERASE_ON_START ) {

			mx_status = mx_caput_nowait( &(epics_mca->start_pv),
						MX_CA_LONG, 1, &start );
		} else {
			mx_status = mx_caput_nowait(
						&(epics_mca->erase_start_pv),
						MX_CA_LONG, 1, &start );
		}
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_mca_stop( MX_MCA *mca )
{
	static const char fname[] = "mxd_epics_mca_stop()";

	MX_EPICS_MCA *epics_mca = NULL;
	int32_t stop;
	mx_status_type mx_status;

	mx_status = mxd_epics_mca_get_pointers( mca, &epics_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	stop = 1;

	mx_status = mx_caput( &(epics_mca->stop_pv), MX_CA_LONG, 1, &stop );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_mca_read( MX_MCA *mca )
{
	static const char fname[] = "mxd_epics_mca_read()";

	MX_EPICS_MCA *epics_mca = NULL;
	unsigned long num_channels;
	mx_bool_type use_malloc = TRUE;
	mx_status_type mx_status;

	mx_status = mxd_epics_mca_get_pointers( mca, &epics_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_mca_get_num_channels( mca->record, &num_channels );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( sizeof(int32_t) == sizeof(long) ) {
		mx_status = mx_caget( &(epics_mca->val_pv),
			MX_CA_LONG, (int) num_channels, mca->channel_array );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	} else
	if ( use_malloc == TRUE ) {
		int32_t *int32_channel_array;
		unsigned long i;

		int32_channel_array = malloc( num_channels * sizeof(int32_t) );

		if ( int32_channel_array == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate a %lu element of "
			"32-bit integers for an EPICS channel array.",
				num_channels );
		}

		mx_status = mx_caget( &(epics_mca->val_pv),
			MX_CA_LONG, (int) num_channels, int32_channel_array );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_free( int32_channel_array );
			return mx_status;
		}

		for ( i = 0; i < num_channels; i++ ) {
			mca->channel_array[i] = int32_channel_array[i];
		}

		mx_free( int32_channel_array );
	} else {
		/* 64-bit longs without using malloc(). */

		int32_t *int32_channel_array;
		unsigned long i;

		mx_status = mx_caget( &(epics_mca->val_pv),
			MX_CA_LONG, (int) num_channels, mca->channel_array );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Since an EPICS 'long' is only 32-bits, then only the
		 * first half of the array mca->channel_array has been used.
		 * We must now transfer the int32_t channel array values to
		 * their proper 64-bit long location.  We must work our way
		 * from the end of the array to the beginning to avoid
		 * overwriting values.
		 * 
		 * FIXME?  This is a hack and may not work on all machine
		 * architectures.
		 */

		int32_channel_array = (int32_t *) mca->channel_array;

		for ( i = num_channels-1; i >= 0; i-- ) {
			mca->channel_array[i] = int32_channel_array[i];
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_mca_clear( MX_MCA *mca )
{
	static const char fname[] = "mxd_epics_mca_clear()";

	MX_EPICS_MCA *epics_mca = NULL;
	int32_t erase;
	mx_status_type mx_status;

	mx_status = mxd_epics_mca_get_pointers( mca, &epics_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	erase = 1;

	mx_status = mx_caput( &(epics_mca->erase_pv), MX_CA_LONG, 1, &erase );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_mca_busy( MX_MCA *mca )
{
	static const char fname[] = "mxd_epics_mca_busy()";

	MX_EPICS_MCA *epics_mca = NULL;
	MX_EPICS_PV *pv;
	int32_t busy;
	unsigned long flags;
	mx_status_type mx_status;

	mx_status = mxd_epics_mca_get_pointers( mca, &epics_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* See if the start operation is still waiting for its callback
	 * to complete.
	 */

	flags = epics_mca->epics_mca_flags;

	if ( flags & MXF_EPICS_MCA_NO_ERASE_ON_START ) {
		pv = &(epics_mca->start_pv);
	} else {
		pv = &(epics_mca->erase_start_pv);
	}

	if ( pv->put_callback_status == MXF_EPVH_CALLBACK_IN_PROGRESS ) {
		mca->busy = TRUE;

#if MXD_EPICS_MCA_DEBUG
		MX_DEBUG(-2,("%s: MCA '%s' start callback in progress.",
			fname, mca->record->name));
#endif

		return MX_SUCCESSFUL_RESULT;
	}

	/* Get the current acquisition status. */

	mx_status = mx_caget( &(epics_mca->acquiring_pv),
				MX_CA_LONG, 1, &busy );

	if ( busy ) {
		mca->busy = TRUE;
	} else {
		mca->busy = FALSE;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_mca_get_parameter( MX_MCA *mca )
{
	static const char fname[] = "mxd_epics_mca_get_parameter()";

	MX_EPICS_MCA *epics_mca = NULL;
	double preset_live_time, preset_real_time;
	int32_t current_num_channels, preset_counts;
	int32_t preset_count_region_low, preset_count_region_high;
	int32_t roi_array_low, roi_array_high, roi_integral, roi_low, roi_high;
	long i;
	mx_status_type mx_status;

	mx_status = mxd_epics_mca_get_pointers( mca, &epics_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mca->parameter_type == MXLV_MCA_CURRENT_NUM_CHANNELS ) {

		mx_status = mx_caget( &(epics_mca->nuse_pv),
				MX_CA_LONG, 1, &current_num_channels );

		if ( mca->current_num_channels > mca->maximum_num_channels ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The EPICS MCA '%s%s' is reported to have %lu "
			"channels, but the record '%s' is only configured "
			"to support up to %lu channels.",
				epics_mca->epics_detector_name,
				epics_mca->epics_mca_name,
				mca->current_num_channels,
				mca->record->name,
				mca->maximum_num_channels );
		}

		mca->current_num_channels = current_num_channels;
	} else
	if ( mca->parameter_type == MXLV_MCA_PRESET_TYPE ) {

		/* Read all of the EPICS variables that affect 
		 * the preset type.
		 */

		mx_status = mx_caget( &(epics_mca->preset_live_pv),
					MX_CA_DOUBLE, 1, &preset_live_time );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_caget( &(epics_mca->preset_real_pv),
					MX_CA_DOUBLE, 1, &preset_real_time );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_caget( &(epics_mca->pct_pv),
					MX_CA_LONG, 1, &preset_counts );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Figure out which type of preset this is. */

		if ( preset_real_time != 0.0 ) {
			mca->preset_type = MXF_MCA_PRESET_REAL_TIME;
		} else
		if ( preset_live_time != 0.0 ) {
			mca->preset_type = MXF_MCA_PRESET_LIVE_TIME;
		} else
		if ( preset_counts != 0 ) {
			mca->preset_type = MXF_MCA_PRESET_COUNT;
		} else {
			mca->preset_type = MXF_MCA_PRESET_NONE;
		}

	} else
	if ( mca->parameter_type == MXLV_MCA_PRESET_COUNT_REGION ) {

		mx_status = mx_caget( &(epics_mca->pctl_pv),
			MX_CA_LONG, 1, &preset_count_region_low );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mca->preset_count_region[0] = preset_count_region_low;

		mx_status = mx_caget( &(epics_mca->pcth_pv),
			MX_CA_LONG, 1, &preset_count_region_high );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mca->preset_count_region[1] = preset_count_region_high;
	} else
	if ( mca->parameter_type == MXLV_MCA_ROI_ARRAY ) {

		for ( i = 0; i < mca->current_num_rois; i++ ) {

			mx_status = mx_caget( 
				&(epics_mca->roi_low_pv_array[i]),
				MX_CA_LONG, 1, &roi_array_low );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = mx_caget(
				&(epics_mca->roi_high_pv_array[i]),
				MX_CA_LONG, 1, &roi_array_high );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mca->roi_array[i][0] = roi_array_low;
			mca->roi_array[i][1] = roi_array_high;
		}
	} else
	if ( mca->parameter_type == MXLV_MCA_ROI_INTEGRAL_ARRAY ) {

		for ( i = 0; i < mca->current_num_rois; i++ ) {

			mx_status = mx_caget(
				&(epics_mca->roi_integral_pv_array[i]),
				MX_CA_LONG, 1, &roi_integral );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mca->roi_integral_array[i] = roi_integral;
		}
	} else
	if ( mca->parameter_type == MXLV_MCA_ROI ) {

		mx_status = mx_caget(
			&(epics_mca->roi_low_pv_array[ mca->roi_number ]),
				MX_CA_LONG, 1, &roi_low );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_caget(
			&(epics_mca->roi_high_pv_array[ mca->roi_number ]),
				MX_CA_LONG, 1, &roi_high );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mca->roi[0] = roi_low;
		mca->roi[1] = roi_high;
	} else
	if ( mca->parameter_type == MXLV_MCA_ROI_INTEGRAL ) {

		mx_status = mx_caget( 
			&(epics_mca->roi_integral_pv_array[ mca->roi_number ]),
				MX_CA_LONG, 1, &roi_integral );

		mca->roi_integral = roi_integral;
	} else
	if ( mca->parameter_type == MXLV_MCA_REAL_TIME ) {

		mx_status = mx_caget( &(epics_mca->ertm_pv),
					MX_CA_DOUBLE, 1, &(mca->real_time) );

	} else
	if ( mca->parameter_type == MXLV_MCA_LIVE_TIME ) {

		mx_status = mx_caget( &(epics_mca->eltm_pv),
					MX_CA_DOUBLE, 1, &(mca->live_time) );

	} else
	if ( mca->parameter_type == MXLV_MCA_COUNTS ) {

		mx_status = mx_caget( &(epics_mca->act_pv),
					MX_CA_DOUBLE, 1, &(mca->counts) );

	} else
	if ( mca->parameter_type == MXLV_MCA_INPUT_COUNT_RATE ) {

		mx_status = mx_caget( &(epics_mca->icr_pv),
				MX_CA_DOUBLE, 1, &(mca->input_count_rate) );

	} else
	if ( mca->parameter_type == MXLV_MCA_OUTPUT_COUNT_RATE ) {

		mx_status = mx_caget( &(epics_mca->ocr_pv),
				MX_CA_DOUBLE, 1, &(mca->output_count_rate) );

	} else {
		return mx_mca_default_get_parameter_handler( mca );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_mca_set_parameter( MX_MCA *mca )
{
	static const char fname[] = "mxd_epics_mca_set_parameter()";

	MX_EPICS_MCA *epics_mca = NULL;
	int32_t preset_count_region_low, preset_count_region_high;
	int32_t roi_array_low, roi_array_high, roi_low, roi_high;
	int32_t current_num_channels;
	long i;
	short no_background;
	mx_status_type mx_status;

	mx_status = mxd_epics_mca_get_pointers( mca, &epics_mca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mca->parameter_type == MXLV_MCA_CURRENT_NUM_CHANNELS ) {

		current_num_channels = mca->current_num_channels;

		mx_status = mx_caput( &(epics_mca->nuse_pv),
				MX_CA_LONG, 1, &current_num_channels );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( mca->current_num_channels > mca->maximum_num_channels ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The EPICS MCA '%s%s' is reported to have %lu "
			"channels, but the record '%s' is only configured "
			"to support up to %lu channels.",
				epics_mca->epics_detector_name,
				epics_mca->epics_mca_name,
				mca->current_num_channels,
				mca->record->name,
				mca->maximum_num_channels );
		}

	} else
	if ( mca->parameter_type == MXLV_MCA_PRESET_COUNT_REGION ) {

		preset_count_region_low  = mca->preset_count_region[0];
		preset_count_region_high = mca->preset_count_region[1];

		mx_status = mx_caput( &(epics_mca->pctl_pv),
			MX_CA_LONG, 1, &preset_count_region_low );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_caput( &(epics_mca->pcth_pv),
			MX_CA_LONG, 1, &preset_count_region_high );

	} else
	if ( mca->parameter_type == MXLV_MCA_ROI_ARRAY ) {

		for ( i = 0; i < mca->current_num_rois; i++ ) {

			/* Set the region boundaries for region i. */

			roi_array_low  = mca->roi_array[i][0];
			roi_array_high = mca->roi_array[i][1];

			mx_status = mx_caput(
				&(epics_mca->roi_low_pv_array[i]),
					MX_CA_LONG, 1, &roi_array_low );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = mx_caput(
				&(epics_mca->roi_high_pv_array[i]),
					MX_CA_LONG, 1, &roi_array_high );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* At present, we are not using the background
			 * calculation.
			 */

			no_background = -1;

			mx_status = mx_caput(
				&(epics_mca->roi_background_pv_array[i]),
					MX_CA_SHORT, 1, &no_background );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	} else
	if ( mca->parameter_type == MXLV_MCA_ROI ) {

		/* Set the region boundaries. */

		roi_low  = mca->roi[0];
		roi_high = mca->roi[1];

		mx_status = mx_caput_nowait(
			&(epics_mca->roi_low_pv_array[ mca->roi_number ]),
				MX_CA_LONG, 1, &roi_low );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_caput_nowait(
			&(epics_mca->roi_high_pv_array[ mca->roi_number ]),
				MX_CA_LONG, 1, &roi_high );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* At present, we are not using the background calculation. */

		no_background = -1;

		mx_status = mx_caput_nowait(
		    &(epics_mca->roi_background_pv_array[ mca->roi_number ]),
				MX_CA_SHORT, 1, &no_background );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	} else {
		return mx_mca_default_set_parameter_handler( mca );
	}

	return mx_status;
}

