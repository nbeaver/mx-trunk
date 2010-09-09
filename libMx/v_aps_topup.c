/*
 * Name:    v_aps_topup.c
 *
 * Purpose: Support for the Advanced Photon Source top-up interlock signals.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2003, 2005-2006, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mxconfig.h"

#if HAVE_EPICS

#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_epics.h"
#include "mx_variable.h"
#include "v_aps_topup.h"

MX_RECORD_FUNCTION_LIST mxv_aps_topup_record_function_list = {
	mxv_aps_topup_initialize_type,
	mxv_aps_topup_create_record_structures,
	NULL,
	NULL,
	NULL,
	mx_receive_variable
};

MX_VARIABLE_FUNCTION_LIST mxv_aps_topup_variable_function_list = {
	mxv_aps_topup_send_variable,
	mxv_aps_topup_receive_variable
};

/*---*/

MX_RECORD_FIELD_DEFAULTS mxv_aps_topup_interlock_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_APS_TOPUP_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_BOOL_VARIABLE_STANDARD_FIELDS
};

long mxv_aps_topup_interlock_num_record_fields
	= sizeof( mxv_aps_topup_interlock_record_field_defaults )
	/ sizeof( mxv_aps_topup_interlock_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_aps_topup_interlock_field_def_ptr
		= &mxv_aps_topup_interlock_record_field_defaults[0];

/*---*/

MX_RECORD_FIELD_DEFAULTS mxv_aps_topup_time_to_inject_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_APS_TOPUP_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_DOUBLE_VARIABLE_STANDARD_FIELDS
};

long mxv_aps_topup_time_to_inject_num_record_fields
	= sizeof( mxv_aps_topup_time_to_inject_record_field_defaults )
	/ sizeof( mxv_aps_topup_time_to_inject_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_aps_topup_time_to_inject_field_def_ptr
		= &mxv_aps_topup_time_to_inject_record_field_defaults[0];

/********************************************************************/

MX_EXPORT mx_status_type
mxv_aps_topup_initialize_type( long record_type )
{
	return mx_variable_initialize_type( record_type );
}

MX_EXPORT mx_status_type
mxv_aps_topup_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxv_aps_topup_create_record_structures()";

	MX_VARIABLE *variable_struct;
	MX_APS_TOPUP *aps_topup;

	if ( record->mx_type == MXV_CAL_APS_TOPUP_INTERLOCK ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"The topup interlock variable type is not yet implemented.  "
	"Use the topup time to inject variable type instead." );
	}

	/* Allocate memory for the necessary structures. */

	variable_struct = (MX_VARIABLE *) malloc( sizeof(MX_VARIABLE) );

	if ( variable_struct == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_VARIABLE structure." );
	}

	aps_topup = (MX_APS_TOPUP *)
				malloc( sizeof(MX_APS_TOPUP) );

	if ( aps_topup == (MX_APS_TOPUP *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_APS_TOPUP structure." );
	}

	/* Now set up the necessary pointers. */

	variable_struct->record = record;

	record->record_superclass_struct = variable_struct;
	record->record_class_struct = NULL;
	record->record_type_struct = aps_topup;

	record->superclass_specific_function_list =
				&mxv_aps_topup_variable_function_list;
	record->class_specific_function_list = NULL;

	mx_epics_pvname_init( &(aps_topup->topup_time_to_inject_pv),
					"Mt:TopUpTime2Inject" );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_aps_topup_send_variable( MX_VARIABLE *variable )
{
	const char fname[] = "mxv_aps_topup_send_variable()";

	return mx_error( MXE_PERMISSION_DENIED, fname,
	"The APS top-up status variables are read-only." );
}

MX_EXPORT mx_status_type
mxv_aps_topup_receive_variable( MX_VARIABLE *variable )
{
	MX_APS_TOPUP *aps_topup;
	MX_RECORD_FIELD *value_field;
	void *value_ptr;
	double topup_time_to_inject;
	double *double_ptr;
	mx_status_type mx_status;

	aps_topup = (MX_APS_TOPUP *) variable->record->record_type_struct;

	mx_status = mx_find_record_field(variable->record,
						"value", &value_field);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	value_ptr = mx_get_field_value_pointer( value_field );

	double_ptr = (double *) value_ptr;

	mx_status = mx_caget( &(aps_topup->topup_time_to_inject_pv),
				MX_CA_DOUBLE, 1, &topup_time_to_inject );

	*double_ptr = topup_time_to_inject + aps_topup->extra_delay_time;

	return mx_status;
}

#endif /* HAVE_EPICS */

