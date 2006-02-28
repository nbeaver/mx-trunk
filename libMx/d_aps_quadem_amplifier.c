/*
 * Name:    d_aps_quadem_amplifier.c
 *
 * Purpose: MX driver for the QUADEM electrometer system designed by
 *          Steve Ross of the Advanced Photon Source using the quadEM
 *          EPICS support written by Mark Rivers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003-2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mxconfig.h"

#if HAVE_EPICS

#include <stdlib.h>
#include <math.h>
#include <float.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_epics.h"
#include "mx_amplifier.h"

#include "d_aps_quadem_amplifier.h"

/* Initialize the amplifier driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_aps_quadem_record_function_list = {
	NULL,
	mxd_aps_quadem_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_aps_quadem_open
};

MX_AMPLIFIER_FUNCTION_LIST mxd_aps_quadem_amplifier_function_list = {
	mxd_aps_quadem_get_gain,
	mxd_aps_quadem_set_gain,
	NULL,
	NULL,
	mxd_aps_quadem_get_time_constant,
	mxd_aps_quadem_set_time_constant,
	mxd_aps_quadem_get_parameter,
	mxd_aps_quadem_set_parameter
};

/* APS_QUADEM amplifier data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_aps_quadem_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AMPLIFIER_STANDARD_FIELDS,
	MXD_APS_QUADEM_STANDARD_FIELDS
};

long mxd_aps_quadem_num_record_fields
		= sizeof( mxd_aps_quadem_record_field_defaults )
		  / sizeof( mxd_aps_quadem_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_aps_quadem_rfield_def_ptr
			= &mxd_aps_quadem_record_field_defaults[0];

/* Private functions for the use of the driver. */

static mx_status_type
mxd_aps_quadem_get_pointers( MX_AMPLIFIER *amplifier,
			MX_APS_QUADEM_AMPLIFIER **aps_quadem_amplifier,
			const char *calling_fname )
{
	static const char fname[] = "mxd_aps_quadem_get_pointers()";

	MX_RECORD *record;

	if ( amplifier == (MX_AMPLIFIER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_AMPLIFIER pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( aps_quadem_amplifier == (MX_APS_QUADEM_AMPLIFIER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_APS_QUADEM_AMPLIFIER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	record = amplifier->record;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the MX_AMPLIFIER structure "
		"passed by '%s' is NULL.", calling_fname );
	}

	*aps_quadem_amplifier = (MX_APS_QUADEM_AMPLIFIER *)
						record->record_type_struct;

	if ( *aps_quadem_amplifier == (MX_APS_QUADEM_AMPLIFIER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_APS_QUADEM_AMPLIFIER pointer for record '%s' is NULL.",
			record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_aps_quadem_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_aps_quadem_create_record_structures()";

	MX_AMPLIFIER *amplifier;
	MX_APS_QUADEM_AMPLIFIER *aps_quadem_amplifier;

	/* Allocate memory for the necessary structures. */

	amplifier = (MX_AMPLIFIER *) malloc( sizeof(MX_AMPLIFIER) );

	if ( amplifier == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_AMPLIFIER structure." );
	}

	aps_quadem_amplifier = (MX_APS_QUADEM_AMPLIFIER *)
				malloc( sizeof(MX_APS_QUADEM_AMPLIFIER) );

	if ( aps_quadem_amplifier == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_APS_QUADEM_AMPLIFIER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = amplifier;
	record->record_type_struct = aps_quadem_amplifier;
	record->class_specific_function_list
			= &mxd_aps_quadem_amplifier_function_list;

	amplifier->record = record;

#if 0
	/* The gain range for an APS_QUADEM is from 1.76e11 to 1e12. */

	amplifier->gain_range[0] = 1.76e11;
	amplifier->gain_range[1] = 1.00e12;
#else
	amplifier->gain_range[0] = -DBL_MAX;
	amplifier->gain_range[1] = DBL_MAX;
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aps_quadem_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_aps_quadem_open()";

	MX_AMPLIFIER *amplifier;
	MX_APS_QUADEM_AMPLIFIER *aps_quadem_amplifier;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	amplifier = (MX_AMPLIFIER *) record->record_class_struct;

	mx_status = mxd_aps_quadem_get_pointers( amplifier,
						&aps_quadem_amplifier, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_epics_pvname_init( &(aps_quadem_amplifier->gain_pv),
			"%sGain", aps_quadem_amplifier->epics_record_namestem );

	mx_epics_pvname_init( &(aps_quadem_amplifier->conversion_time_pv),
			"%sConversionTime",
				aps_quadem_amplifier->epics_record_namestem );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aps_quadem_get_gain( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_aps_quadem_get_gain()";

	MX_APS_QUADEM_AMPLIFIER *aps_quadem_amplifier;
	short selected_gain_range;
	mx_status_type mx_status;

	mx_status = mxd_aps_quadem_get_pointers( amplifier,
						&aps_quadem_amplifier, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_caget( &(aps_quadem_amplifier->gain_pv),
				MX_CA_SHORT, 1, &selected_gain_range );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( selected_gain_range ) {
	case 0:
		amplifier->gain = 1.0e12;
		break;
	case 1:
		amplifier->gain = 2.51e12;
		break;
	case 2:
		amplifier->gain = 2.93e12;
		break;
	case 3:
		amplifier->gain = 3.52e12;
		break;
	case 4:
		amplifier->gain = 4.40e12;
		break;
	case 5:
		amplifier->gain = 5.87e12;
		break;
	case 6:
		amplifier->gain = 8.80e12;
		break;
	case 7:
		amplifier->gain = 17.60e12;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unexpected gain range %hd seen for amplifier '%s'.",
			selected_gain_range, amplifier->record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aps_quadem_set_gain( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_aps_quadem_set_gain()";

	MX_APS_QUADEM_AMPLIFIER *aps_quadem_amplifier;
	short gain_range;
	mx_status_type mx_status;

	mx_status = mxd_aps_quadem_get_pointers( amplifier,
						&aps_quadem_amplifier, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( amplifier->gain < 1.58e12 ) {
		gain_range = 0;
	} else
	if ( amplifier->gain < 2.71e12 ) {
		gain_range = 1;
	} else
	if ( amplifier->gain < 3.21e12 ) {
		gain_range = 2;
	} else
	if ( amplifier->gain < 3.94e12 ) {
		gain_range = 3;
	} else
	if ( amplifier->gain < 5.08e12 ) {
		gain_range = 4;
	} else
	if ( amplifier->gain < 7.19e12 ) {
		gain_range = 5;
	} else
	if ( amplifier->gain < 12.45e12 ) {
		gain_range = 6;
	} else {
		gain_range = 7;
	}

	mx_status = mx_caput( &(aps_quadem_amplifier->gain_pv),
				MX_CA_SHORT, 1, &gain_range );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_aps_quadem_get_time_constant( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_aps_quadem_get_time_constant()";

	MX_APS_QUADEM_AMPLIFIER *aps_quadem_amplifier;
	double raw_time_constant;
	mx_status_type mx_status;

	mx_status = mxd_aps_quadem_get_pointers( amplifier,
						&aps_quadem_amplifier, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_caget( &(aps_quadem_amplifier->conversion_time_pv),
				MX_CA_DOUBLE, 1, &raw_time_constant );

	amplifier->time_constant = 1.0e-6 * raw_time_constant;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_aps_quadem_set_time_constant( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_aps_quadem_set_time_constant()";

	MX_APS_QUADEM_AMPLIFIER *aps_quadem_amplifier;
	double raw_time_constant;
	mx_status_type mx_status;

	mx_status = mxd_aps_quadem_get_pointers( amplifier,
						&aps_quadem_amplifier, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	raw_time_constant = 1.0e6 * amplifier->time_constant;

	mx_status = mx_caput( &(aps_quadem_amplifier->conversion_time_pv),
				MX_CA_DOUBLE, 1, &raw_time_constant );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_aps_quadem_get_parameter( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_aps_quadem_get_parameter()";

	MX_APS_QUADEM_AMPLIFIER *aps_quadem_amplifier;
	mx_status_type mx_status;

	mx_status = mxd_aps_quadem_get_pointers( amplifier,
						&aps_quadem_amplifier, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,
	("%s invoked for amplifier '%s' for parameter type '%s' (%ld).",
		fname, amplifier->record->name,
		mx_get_field_label_string( amplifier->record,
			amplifier->parameter_type ),
		amplifier->parameter_type));

	switch( amplifier->parameter_type ) {
	default:
		return mx_amplifier_default_get_parameter_handler( amplifier );
	}
}

MX_EXPORT mx_status_type
mxd_aps_quadem_set_parameter( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_aps_quadem_set_parameter()";

	MX_APS_QUADEM_AMPLIFIER *aps_quadem_amplifier;
	mx_status_type mx_status;

	mx_status = mxd_aps_quadem_get_pointers( amplifier,
						&aps_quadem_amplifier, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,
	("%s invoked for amplifier '%s' for parameter type '%s' (%ld).",
		fname, amplifier->record->name,
		mx_get_field_label_string( amplifier->record,
			amplifier->parameter_type ),
		amplifier->parameter_type));

	switch( amplifier->parameter_type ) {
	default:
		mx_status = mx_amplifier_default_set_parameter_handler(
				amplifier );
	}

	return mx_status;
}

#endif /* HAVE_EPICS */

