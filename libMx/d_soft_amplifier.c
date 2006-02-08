/*
 * Name:    d_soft_amplifier.c
 *
 * Purpose: MX driver to control software emulated amplifiers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_amplifier.h"
#include "d_soft_amplifier.h"

/* Initialize the amplifier driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_soft_amplifier_record_function_list = {
	mxd_soft_amplifier_initialize_type,
	mxd_soft_amplifier_create_record_structures,
	mxd_soft_amplifier_finish_record_initialization,
	mxd_soft_amplifier_delete_record,
	NULL,
	mxd_soft_amplifier_read_parms_from_hardware,
	mxd_soft_amplifier_write_parms_to_hardware,
	mxd_soft_amplifier_open,
	mxd_soft_amplifier_close
};

MX_AMPLIFIER_FUNCTION_LIST mxd_soft_amplifier_amplifier_function_list = {
	mxd_soft_amplifier_get_gain,
	mxd_soft_amplifier_set_gain,
	mxd_soft_amplifier_get_offset,
	mxd_soft_amplifier_set_offset,
	mxd_soft_amplifier_get_time_constant,
	mxd_soft_amplifier_set_time_constant,
	mx_amplifier_default_get_parameter_handler,
	mx_amplifier_default_set_parameter_handler
};

/* Soft amplifier data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_soft_amplifier_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AMPLIFIER_STANDARD_FIELDS
};

mx_length_type mxd_soft_amplifier_num_record_fields
		= sizeof( mxd_soft_amplifier_record_field_defaults )
		  / sizeof( mxd_soft_amplifier_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_soft_amplifier_rfield_def_ptr
			= &mxd_soft_amplifier_record_field_defaults[0];

/* === */

MX_EXPORT mx_status_type
mxd_soft_amplifier_initialize_type( long type )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_amplifier_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxd_soft_amplifier_create_record_structures()";

	MX_AMPLIFIER *amplifier;
	MX_SOFT_AMPLIFIER *soft_amplifier;

	/* Allocate memory for the necessary structures. */

	amplifier = (MX_AMPLIFIER *) malloc( sizeof(MX_AMPLIFIER) );

	if ( amplifier == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_AMPLIFIER structure." );
	}

	soft_amplifier = (MX_SOFT_AMPLIFIER *)
				malloc( sizeof(MX_SOFT_AMPLIFIER) );

	if ( soft_amplifier == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SOFT_AMPLIFIER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = amplifier;
	record->record_type_struct = soft_amplifier;
	record->class_specific_function_list
			= &mxd_soft_amplifier_amplifier_function_list;

	amplifier->record = record;

	/* We arbitrarily choose the gain range for a soft amplifier
	 * to be from 1 to 1e10.
	 */

	amplifier->gain_range[0] = 1.0;
	amplifier->gain_range[1] = 1.0e10;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_amplifier_finish_record_initialization( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_amplifier_delete_record( MX_RECORD *record )
{
	MX_SOFT_AMPLIFIER *soft_amplifier;

	if ( record == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	soft_amplifier = (MX_SOFT_AMPLIFIER *) record->record_type_struct;

	if ( soft_amplifier != NULL ) {
		free( soft_amplifier );

		record->record_type_struct = NULL;
	}

	if ( record->record_class_struct != NULL ) {
		free( record->record_class_struct );

		record->record_class_struct = NULL;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_amplifier_read_parms_from_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_amplifier_write_parms_to_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_amplifier_open( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_amplifier_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_amplifier_get_gain( MX_AMPLIFIER *amplifier )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_amplifier_set_gain( MX_AMPLIFIER *amplifier )
{
	const char fname[] = "mxd_soft_amplifier_set_gain()";

	double gain;

	if ( amplifier == (MX_AMPLIFIER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AMPLIFIER pointer passed was NULL." );
	}

	gain = amplifier->gain;

	if ( ( gain < amplifier->gain_range[0] )
	  || ( gain > amplifier->gain_range[1] ) )
	{
		amplifier->gain = amplifier->gain_range[0];

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The requested amplifier gain of %g is outside the allowed range of %g to %g.",
			gain, amplifier->gain_range[0],
			amplifier->gain_range[1] );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_amplifier_get_offset( MX_AMPLIFIER *amplifier )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_amplifier_set_offset( MX_AMPLIFIER *amplifier )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_amplifier_get_time_constant( MX_AMPLIFIER *amplifier )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_amplifier_set_time_constant( MX_AMPLIFIER *amplifier )
{
	return MX_SUCCESSFUL_RESULT;
}

