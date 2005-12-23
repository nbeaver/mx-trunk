/*
 * Name:    v_mathop.c
 *
 * Purpose: Support for simple mathematical operations on a list of records.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2001-2002, 2004-2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_types.h"
#include "mx_variable.h"
#include "v_mathop.h"

#include "mx_analog_input.h"
#include "mx_analog_output.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "mx_motor.h"
#include "mx_encoder.h"
#include "mx_scaler.h"
#include "mx_timer.h"
#include "mx_amplifier.h"
#include "mx_relay.h"

MX_RECORD_FUNCTION_LIST mxv_mathop_record_function_list = {
	mxv_mathop_initialize_type,
	mxv_mathop_create_record_structures,
	mxv_mathop_finish_record_initialization,
	mx_default_delete_record_handler,
	NULL,
	mxv_mathop_dummy_function,
	mxv_mathop_dummy_function,
	mxv_mathop_dummy_function,
	mxv_mathop_dummy_function
};

MX_VARIABLE_FUNCTION_LIST mxv_mathop_variable_function_list = {
	mxv_mathop_send_variable,
	mxv_mathop_receive_variable
};

MX_RECORD_FIELD_DEFAULTS mxv_mathop_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_MATHOP_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_DOUBLE_VARIABLE_STANDARD_FIELDS
};

long mxv_mathop_num_record_fields
	= sizeof( mxv_mathop_record_field_defaults )
	/ sizeof( mxv_mathop_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_mathop_rfield_def_ptr
		= &mxv_mathop_record_field_defaults[0];

#define MXV_MATHOP_DEBUG	FALSE

/********************************************************************/

static struct {
	int type;
	char name[ MXU_MATHOP_NAME_LENGTH + 1 ];
} mxv_mathop_operation_type [] = {
	{ MXF_MATHOP_ADD,		"add" },
	{ MXF_MATHOP_SUBTRACT,		"subtract" },
	{ MXF_MATHOP_MULTIPLY,		"multiply" },
	{ MXF_MATHOP_DIVIDE,		"divide" },
	{ MXF_MATHOP_NEGATE,		"negate" },

	{ MXF_MATHOP_SQUARE,		"square" },
	{ MXF_MATHOP_SQUARE_ROOT,	"square_root" },

	{ MXF_MATHOP_SINE,		"sine" },
	{ MXF_MATHOP_COSINE,		"cosine" },
	{ MXF_MATHOP_TANGENT,		"tangent" },

	{ MXF_MATHOP_ARC_SINE,		"arc_sine" },
	{ MXF_MATHOP_ARC_COSINE,	"arc_cosine" },
	{ MXF_MATHOP_ARC_TANGENT,	"arc_tangent" },

	{ MXF_MATHOP_EXPONENTIAL,	"exponential" },
	{ MXF_MATHOP_LOGARITHM,		"logarithm" },
	{ MXF_MATHOP_LOG10,		"log10" },

	/* Common abbreviations. */

	{ MXF_MATHOP_SQUARE_ROOT,	"sqrt" },

	{ MXF_MATHOP_SINE,		"sin" },
	{ MXF_MATHOP_COSINE,		"cos" },
	{ MXF_MATHOP_TANGENT,		"tan" },

	{ MXF_MATHOP_ARC_SINE,		"asin" },
	{ MXF_MATHOP_ARC_COSINE,	"acos" },
	{ MXF_MATHOP_ARC_TANGENT,	"atan" },

	{ MXF_MATHOP_EXPONENTIAL,	"exp" },
	{ MXF_MATHOP_LOGARITHM,		"log" },

	/* Alternate spellings. */

	{ MXF_MATHOP_ARC_SINE,		"arcsine" },
	{ MXF_MATHOP_ARC_COSINE,	"arccosine" },
	{ MXF_MATHOP_ARC_TANGENT,	"arctangent" },
};

static int mxv_mathop_num_operation_types =
		sizeof( mxv_mathop_operation_type )
		/ sizeof( mxv_mathop_operation_type[0] );

static mx_status_type mxv_mathop_get_value( MX_RECORD *record, double *value );

static mx_status_type mxv_mathop_put_value( MX_RECORD *record, double value,
						unsigned long mathop_flags );

static mx_status_type mxv_mathop_compute_value( MX_VARIABLE *variable,
						double *value );

static mx_status_type mxv_mathop_change_value( MX_VARIABLE *variable,
						double new_value );

MX_EXPORT mx_status_type
mxv_mathop_initialize_type( long record_type )
{
	const char fname[] = "mxv_mathop_initialize_type()";

	MX_DRIVER *driver;
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
	MX_RECORD_FIELD_DEFAULTS **record_field_defaults_ptr;
	MX_RECORD_FIELD_DEFAULTS *field;
	long num_record_fields, referenced_field_index;
	long num_items_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_variable_initialize_type( record_type );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	driver = mx_get_driver_by_type( record_type );

	if ( driver == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Record type %ld not found.",
			record_type );
	}

	record_field_defaults_ptr = driver->record_field_defaults_ptr;

	if (record_field_defaults_ptr == (MX_RECORD_FIELD_DEFAULTS **) NULL) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'record_field_defaults_ptr' for record type '%s' is NULL.",
			driver->name );
	}

	record_field_defaults = *record_field_defaults_ptr;

	if ( record_field_defaults == (MX_RECORD_FIELD_DEFAULTS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'record_field_defaults_ptr' for record type '%s' is NULL.",
			driver->name );
	}

	if ( driver->num_record_fields == (long *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'num_record_fields' pointer for record type '%s' is NULL.",
			driver->name );
	}

	num_record_fields = *(driver->num_record_fields);

	/* Construct a varargs cookie for 'num_items'. */

	mx_status = mx_find_record_field_defaults_index(
			record_field_defaults, num_record_fields,
			"num_items", &referenced_field_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_construct_varargs_cookie( referenced_field_index, 0,
					&num_items_varargs_cookie );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize all the varargs fields that depend on 'num_items'. */

	mx_status = mx_find_record_field_defaults( record_field_defaults,
				num_record_fields, "item_array", &field);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = num_items_varargs_cookie;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_mathop_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxv_mathop_create_record_structures()";

	MX_VARIABLE *variable_struct;
	MX_MATHOP_VARIABLE *mathop_variable;

	/* Allocate memory for the necessary structures. */

	variable_struct = (MX_VARIABLE *) malloc( sizeof(MX_VARIABLE) );

	if ( variable_struct == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_VARIABLE structure." );
	}

	mathop_variable = (MX_MATHOP_VARIABLE *)
				malloc( sizeof(MX_MATHOP_VARIABLE) );

	if ( mathop_variable == (MX_MATHOP_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MATHOP_VARIABLE structure." );
	}

	/* Now set up the necessary pointers. */

	variable_struct->record = record;

	record->record_superclass_struct = variable_struct;
	record->record_class_struct = NULL;
	record->record_type_struct = mathop_variable;

	record->superclass_specific_function_list =
				&mxv_mathop_variable_function_list;
	record->class_specific_function_list = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_mathop_finish_record_initialization( MX_RECORD *record )
{
	const char fname[] = "mxv_mathop_finish_record_initialization()";

	MX_MATHOP_VARIABLE *mathop_variable;
	MX_RECORD *dependent_record;
	char *name, *endptr;
	long i, num_items;
	double value;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	mathop_variable = (MX_MATHOP_VARIABLE *) record->record_type_struct;

	if ( mathop_variable == (MX_MATHOP_VARIABLE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MATHOP_VARIABLE pointer for record '%s' is NULL.",
			record->name );
	}

	/* Initialize the operation_type from the operation_name string. */

	name = mathop_variable->operation_name;

	for ( i = 0; i < mxv_mathop_num_operation_types; i++ ) {

		if ( strcmp(name, mxv_mathop_operation_type[i].name) == 0 ) {

			mathop_variable->operation_type =
				mxv_mathop_operation_type[i].type;

			break;		/* Exit the for() loop. */
		}
	}

	if ( i >= mxv_mathop_num_operation_types ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The operation type '%s' for mathop record '%s' is not recognized.",
			name, record->name );
	}

	MX_DEBUG( 2,("%s: mathop '%s' operation '%s' type = %d",
		fname, record->name, name, mathop_variable->operation_type));

	/* Allocate memory for 'record_array' and 'value_array'. */

	num_items = mathop_variable->num_items;

	mathop_variable->record_array = (MX_RECORD **) malloc(
				num_items * sizeof( MX_RECORD * ) );

	if ( mathop_variable->record_array == (MX_RECORD **) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Ran out of memory trying to allocate a %ld element array of "
	"MX_RECORD pointers.", num_items );
	}

	mathop_variable->value_array = (double *) malloc(
				num_items * sizeof( double ) );

	if ( mathop_variable->value_array == (double *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Ran out of memory trying to allocate a %ld element array of doubles.",
			num_items );
	}

	/* Initialize the contents of 'record_array' and 'value_array'. */

	for ( i = 0; i < num_items; i++ ) {

		name = (mathop_variable->item_array)[i];

		dependent_record = mx_get_record( record, name );

		(mathop_variable->record_array)[i] = dependent_record;

		if ( dependent_record != NULL ) {
			value = 0.0;
		} else {
			/* If the contents of the item_array element
			 * does not correspond to a record name, we assume
			 * that it contains a numerical constant instead.
			 *
			 * We can verify this by trying to parse the
			 * string for the numerical constant.
			 */

			value = strtod( name, &endptr );

			if ( *endptr != '\0' ) {
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Elements of the item_array for mathop record '%s' must be "
	"either MX record names or numerical constants.  The string '%s' "
	"is neither of these things.", record->name, name );
			}
		}
		(mathop_variable->value_array)[i] = value;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_mathop_dummy_function( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_mathop_send_variable( MX_VARIABLE *variable )
{
	void *value_ptr;
	double *double_ptr;
	double new_value;
	mx_status_type mx_status;

	mx_status = mx_get_variable_pointer( variable->record, &value_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	double_ptr = (double *) value_ptr;

	new_value = *double_ptr;

	mx_status = mxv_mathop_change_value( variable, new_value );

	return mx_status;
}

MX_EXPORT mx_status_type
mxv_mathop_receive_variable( MX_VARIABLE *variable )
{
	void *value_ptr;
	double *double_ptr;
	double value;
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warning. */

	value = 0.0;

	mx_status = mxv_mathop_compute_value( variable, &value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get_variable_pointer( variable->record, &value_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	double_ptr = (double *) value_ptr;

	*double_ptr = value;

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

static mx_status_type
mxv_mathop_get_value( MX_RECORD *record, double *value )
{
	const char fname[] = "mxv_mathop_get_value()";

	void *pointer_to_value;
	long num_dimensions, field_type;
	unsigned long ulong_value;
	long long_value;
	int int_value;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}
	if ( value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The value pointer passed was NULL." );
	}

	*value = 0.0;

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( record->mx_superclass ) {
	case MXR_DEVICE:

		switch( record->mx_class ) {
		case MXC_ANALOG_INPUT:
			mx_status = mx_analog_input_read( record, value );
			break;
		case MXC_ANALOG_OUTPUT:
			mx_status = mx_analog_output_read( record, value );
			break;
		case MXC_DIGITAL_INPUT:
			mx_status = mx_digital_input_read( record,
							&ulong_value );

			*value = (double) ulong_value;
			break;
		case MXC_DIGITAL_OUTPUT:
			mx_status = mx_digital_output_read( record,
							&ulong_value );

			*value = (double) ulong_value;
			break;
		case MXC_MOTOR:
			mx_status = mx_motor_get_position( record, value );
			break;
		case MXC_ENCODER:
			mx_status = mx_encoder_read( record, &long_value );

			*value = (double) long_value;
			break;
		case MXC_SCALER:
			mx_status = mx_scaler_read( record, &long_value );

			*value = (double) long_value;
			break;
		case MXC_TIMER:
			mx_status = mx_timer_read( record, value );
			break;
		case MXC_AMPLIFIER:
			mx_status = mx_amplifier_get_gain( record, value );
			break;
		case MXC_RELAY:
			mx_status = mx_get_relay_status( record, &int_value );
			break;
		default:
			return mx_error( MXE_TYPE_MISMATCH, fname,
		"Device '%s' may not be used by a mathop variable record.",
				record->name );
			break;
		}
		break;

	case MXR_VARIABLE:
		/* Find out the size and datatype of the variable. */

		mx_status = mx_get_variable_parameters( record,
					&num_dimensions, NULL,
					&field_type, &pointer_to_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Only 1-dimensional variables are currently supported. */

		if ( num_dimensions != 1 ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"%ld dimensional variable '%s' may not be used by a "
			"mathop variable record.  Only 1-dimensional variables "
			"are supported.", num_dimensions, record->name );
		}

		/* Receive the current value of the variable. */

		mx_status = mx_receive_variable( record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Save the current value of the first element of the
		 * 1-dimensional array.  Any elements after the first
		 * one are ignored.
		 */

		switch( field_type ) {
		case MXFT_CHAR:
			*value = (double) *(char *) pointer_to_value;
			break;
		case MXFT_UCHAR:
			*value = (double) *(unsigned char *) pointer_to_value;
			break;
		case MXFT_SHORT:
			*value = (double) *(short *) pointer_to_value;
			break;
		case MXFT_USHORT:
			*value = (double) *(unsigned short *) pointer_to_value;
			break;
		case MXFT_INT:
			*value = (double) *(int *) pointer_to_value;
			break;
		case MXFT_UINT:
			*value = (double) *(unsigned int *) pointer_to_value;
			break;
		case MXFT_LONG:
			*value = (double) *(long *) pointer_to_value;
			break;
		case MXFT_ULONG:
		case MXFT_HEX:
			*value = (double) *(unsigned long *) pointer_to_value;
			break;
		case MXFT_FLOAT:
			*value = (double) *(float *) pointer_to_value;
			break;
		case MXFT_DOUBLE:
			*value = *(double *) pointer_to_value;
			break;
		default:
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"Variable '%s' is of a type that is not supported "
			"by mathop variable records.",
				record->name );
			break;
		}
		break;

	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Record '%s' may not be used by a mathop variable record.  "
		"Only device and variable records may be used.",
			record->name );
		break;
	}

	return mx_status;
}

static mx_status_type
mxv_mathop_put_value( MX_RECORD *record, double new_value, unsigned long flags )
{
	const char fname[] = "mxv_mathop_put_value()";

	void *pointer_to_value;
	long num_dimensions, field_type;
	char *char_ptr;
	unsigned char *uchar_ptr;
	short *short_ptr;
	unsigned short *ushort_ptr;
	int *int_ptr;
	unsigned int *uint_ptr;
	long *long_ptr;
	unsigned long *ulong_ptr;
	float *float_ptr;
	double *double_ptr;
	mx_status_type mx_status;


	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( record->mx_superclass ) {
	case MXR_DEVICE:

		switch( record->mx_class ) {
		case MXC_ANALOG_INPUT:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Cannot write value %g to analog input '%s'.  "
			"Analog inputs are read only.",
				new_value, record->name );
			break;
		case MXC_ANALOG_OUTPUT:
			mx_status = mx_analog_output_write( record, new_value );
			break;
		case MXC_DIGITAL_INPUT:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Cannot write value %g to digital input '%s'.  "
			"Digital inputs are read only.",
				new_value, record->name );
			break;
		case MXC_DIGITAL_OUTPUT:
			mx_status = mx_digital_output_write( record,
							mx_round( new_value ) );
			break;
		case MXC_MOTOR:
			if ( flags & MXF_MATHOP_WRITE_ONLY_REDEFINES_VALUE ) {
				mx_status = mx_motor_set_position( record,
								new_value );
			} else {
				mx_status = mx_motor_move_absolute( record,
						new_value, MXF_MTR_NOWAIT );
			}
			break;
		case MXC_ENCODER:
			mx_status = mx_encoder_write( record,
						mx_round( new_value ) );
			break;
		case MXC_SCALER:
			mx_status = mx_scaler_start( record,
						mx_round( new_value ) );
			break;
		case MXC_TIMER:
			mx_status = mx_timer_start( record, new_value );
			break;
		case MXC_AMPLIFIER:
			mx_status = mx_amplifier_set_gain( record, new_value );
			break;
		case MXC_RELAY:
			mx_status = mx_relay_command( record,
						(int) mx_round( new_value ) );
			break;
		default:
			return mx_error( MXE_TYPE_MISMATCH, fname,
		"Device '%s' may not be used by a mathop variable record.",
				record->name );
			break;
		}
		break;

	case MXR_VARIABLE:
		/* Find out the size and datatype of the variable. */

		mx_status = mx_get_variable_parameters( record,
					&num_dimensions, NULL,
					&field_type, &pointer_to_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Only 1-dimensional variables are currently supported. */

		if ( num_dimensions != 1 ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"%ld dimensional variable '%s' may not be used by a "
			"mathop variable record.  Only 1-dimensional variables "
			"are supported.", num_dimensions, record->name );
		}

		/* Assign the new value to the first element of the
		 * 1-dimensional array.  Any elements after the first
		 * one are ignored.
		 */

		switch( field_type ) {
		case MXFT_CHAR:
			char_ptr = (char *) pointer_to_value;

			*char_ptr = (char) mx_round( new_value );
			break;
		case MXFT_UCHAR:
			uchar_ptr = (unsigned char *) pointer_to_value;

			*uchar_ptr = (unsigned char) mx_round( new_value );
			break;
		case MXFT_SHORT:
			short_ptr = (short *) pointer_to_value;

			*short_ptr = (short) mx_round( new_value );
			break;
		case MXFT_USHORT:
			ushort_ptr = (unsigned short *) pointer_to_value;

			*ushort_ptr = (unsigned short) mx_round( new_value );
			break;
		case MXFT_INT:
			int_ptr = (int *) pointer_to_value;

			*int_ptr = (int) mx_round( new_value );
			break;
		case MXFT_UINT:
			uint_ptr = (unsigned int *) pointer_to_value;

			*uint_ptr = (unsigned int) mx_round( new_value );
			break;
		case MXFT_LONG:
			long_ptr = (long *) pointer_to_value;

			*long_ptr = mx_round( new_value );
			break;
		case MXFT_ULONG:
			ulong_ptr = (unsigned long *) pointer_to_value;

			*ulong_ptr = (unsigned long) mx_round( new_value );
			break;
		case MXFT_FLOAT:
			float_ptr = (float *) pointer_to_value;

			*float_ptr = (float) new_value;
			break;
		case MXFT_DOUBLE:
			double_ptr = (double *) pointer_to_value;

			*double_ptr = new_value;
			break;
		default:
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"Variable '%s' is of a type that is not supported "
			"by mathop variable records",
				record->name );
			break;
		}

		/* Send the current value of the variable. */

		mx_status = mx_send_variable( record );
		break;

	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Record '%s' may not be used by a variable record.  "
		"Only device and variable records may be used.",
			record->name );
		break;
	}

	return mx_status;
}

static mx_status_type
mxv_mathop_compute_value( MX_VARIABLE *variable, double *value )
{
	const char fname[] = "mxv_mathop_compute_value()";

	MX_RECORD *variable_record;
	MX_MATHOP_VARIABLE *mathop_variable;
	MX_RECORD **record_array;
	double *value_array;
	double local_value, temp_value;
	unsigned long i;
	mx_status_type mx_status;

	mx_status = MX_SUCCESSFUL_RESULT;

	if ( variable == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VARIABLE pointer passed was NULL." );
	}

	variable_record = variable->record;

	if ( variable_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_RECORD pointer for the MX_VARIABLE structure passed is NULL." );
	}

	mathop_variable = (MX_MATHOP_VARIABLE *)
				variable_record->record_type_struct;

	if ( mathop_variable == (MX_MATHOP_VARIABLE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MATHOP_VARIABLE pointer for record '%s' is NULL.",
			variable_record->name );
	}

	/* Initialize the computed value, if possible. */

	if ( value != NULL ) {
		*value = 0.0;
	}

	/* If there are no items to compute with, return zero. */

	if ( mathop_variable->num_items <= 0 )
		return MX_SUCCESSFUL_RESULT;

	record_array = mathop_variable->record_array;
	value_array = mathop_variable->value_array;

	if ( mathop_variable->num_items > 1 ) {

	    if ( (mathop_variable->operation_type != MXF_MATHOP_ADD)
	      && (mathop_variable->operation_type != MXF_MATHOP_SUBTRACT)
	      && (mathop_variable->operation_type != MXF_MATHOP_MULTIPLY)
	      && (mathop_variable->operation_type != MXF_MATHOP_DIVIDE) )
	    {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Multiple record math operations are allowed only for "
		"addition, subtraction, multiplication, and division." );
	    } else {

		/* Initialize the value to the first record's value. */

		if ( record_array[0] == (MX_RECORD *) NULL ) {
			local_value = value_array[0];
		} else {
			mx_status = mxv_mathop_get_value( record_array[0],
							&local_value );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			value_array[0] = local_value;
		}

		for ( i = 1; i < mathop_variable->num_items; i++ ) {

			if ( record_array[i] == (MX_RECORD *) NULL ) {
				temp_value = value_array[i];
			} else {
				mx_status = mxv_mathop_get_value(
							record_array[i],
							&temp_value );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				value_array[i] = temp_value;
			}

			switch( mathop_variable->operation_type ) {
			case MXF_MATHOP_ADD:
				local_value += temp_value;
				break;
			case MXF_MATHOP_SUBTRACT:
				local_value -= temp_value;
				break;
			case MXF_MATHOP_MULTIPLY:
				local_value *= temp_value;
				break;
			case MXF_MATHOP_DIVIDE:
				local_value = mx_divide_safely( local_value,
								temp_value );
				break;
			}
		}
	    }
	} else {
	    if ( record_array[0] == (MX_RECORD *) NULL ) {
		local_value = value_array[0];
	    } else {
		mx_status = mxv_mathop_get_value( record_array[0],
							&local_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		value_array[0] = local_value;
	    }

	    switch( mathop_variable->operation_type ) {
	    case MXF_MATHOP_ADD:
	    case MXF_MATHOP_SUBTRACT:
	    case MXF_MATHOP_MULTIPLY:
	    case MXF_MATHOP_DIVIDE:
		break;

	    case MXF_MATHOP_NEGATE:
		local_value = -local_value;
		break;

	    case MXF_MATHOP_SQUARE:
		local_value *= local_value;
		break;

	    case MXF_MATHOP_SQUARE_ROOT:
		if ( local_value < 0.0 ) {
			local_value = 0.0;
		} else {
			local_value = sqrt( local_value );
		}
		break;

	    case MXF_MATHOP_SINE:
		local_value = sin( local_value );
		break;

	    case MXF_MATHOP_COSINE:
		local_value = cos( local_value );
		break;

	    case MXF_MATHOP_TANGENT:
		local_value = tan( local_value );
		break;

	    case MXF_MATHOP_ARC_SINE:
		if ( local_value > 1.0 ) {
			local_value = 1.0;
		} else if ( local_value < -1.0 ) {
			local_value = -1.0;
		}
		local_value = asin( local_value );
		break;

	    case MXF_MATHOP_ARC_COSINE:
		if ( local_value > 1.0 ) {
			local_value = 1.0;
		} else if ( local_value < -1.0 ) {
			local_value = -1.0;
		}
		local_value = acos( local_value );
		break;

	    case MXF_MATHOP_ARC_TANGENT:
		local_value = atan( local_value );
		break;

	    case MXF_MATHOP_EXPONENTIAL:
		local_value = exp( local_value );
		break;

	    case MXF_MATHOP_LOGARITHM:
		if ( local_value <= 0.0 ) {
			local_value = - DBL_MAX;
		} else {
			local_value = log( local_value );
		}
		break;

	    case MXF_MATHOP_LOG10:
		if ( local_value <= 0.0 ) {
			local_value = - DBL_MAX;
		} else {
			local_value = log10( local_value );
		}
		break;

	    default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Mathop operation type %d is unsupported.",
			mathop_variable->operation_type );
		break;
	    }
	}

	if ( value != NULL ) {
		*value = local_value;
	}

	return mx_status;
}

static mx_status_type
mxv_mathop_change_value( MX_VARIABLE *variable, double new_value )
{
	const char fname[] = "mxv_mathop_change_value()";

	MX_RECORD *variable_record;
	MX_MATHOP_VARIABLE *mathop_variable;
	MX_RECORD **record_array;
	double *value_array;
	double old_partial_value, new_item_value, temp_value;
	unsigned long i;
	mx_status_type mx_status;

	if ( variable == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VARIABLE pointer passed was NULL." );
	}

	variable_record = variable->record;

	if ( variable_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_RECORD pointer for the MX_VARIABLE structure passed is NULL." );
	}

	mathop_variable = (MX_MATHOP_VARIABLE *)
				variable_record->record_type_struct;

	if ( mathop_variable == (MX_MATHOP_VARIABLE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MATHOP_VARIABLE pointer for record '%s' is NULL.",
			variable_record->name );
	}

	record_array = mathop_variable->record_array;
	value_array = mathop_variable->value_array;

	new_item_value = 0.0;

	MX_DEBUG( 2,("%s invoked for variable '%s', new_value = %g",
		fname, variable_record->name, new_value));

	if ( mathop_variable->mathop_flags & MXF_MATHOP_READ_ONLY ) {
		return mx_error( MXE_PERMISSION_DENIED, fname,
			"The mathop variable '%s' is read only.",
			variable_record->name );
	}

	/* If there are no items to compute with, then return. */

	if ( mathop_variable->num_items <= 0 )
		return MX_SUCCESSFUL_RESULT;

	/* If item_to_change is outside the allowed range of item numbers,
	 * then return without changing anything.
	 */

	if ( ( mathop_variable->item_to_change < 0 )
	  || ( mathop_variable->item_to_change >= mathop_variable->num_items ) )
	{
		return MX_SUCCESSFUL_RESULT;
	}

	/* If item_to_change is a static value without an associated record,
	 * then return with an error message.
	 */

	i = mathop_variable->item_to_change;

	if ( record_array[i] == (MX_RECORD *) NULL ) {

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Mathop variable '%s' is configured with item_to_change == %ld, "
	"but the corresponding item '%s' is an unmodifiable constant.  "
	"You must specify an item_to_change that is a record name.",
			variable_record->name,
			mathop_variable->item_to_change,
			mathop_variable->item_array[i] );
	}

	if ( mathop_variable->num_items > 1 ) {

	    if ( (mathop_variable->operation_type != MXF_MATHOP_ADD)
	      && (mathop_variable->operation_type != MXF_MATHOP_SUBTRACT)
	      && (mathop_variable->operation_type != MXF_MATHOP_MULTIPLY)
	      && (mathop_variable->operation_type != MXF_MATHOP_DIVIDE) )
	    {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Multiple record math operations are allowed only for "
		"addition, subtraction, multiplication, and division." );
	    } else {

		/* Compute the old partial value, not including the
		 * value of the item to change.
		 */

		if ( mathop_variable->item_to_change == 0 ) {
			switch( mathop_variable->operation_type ) {
			case MXF_MATHOP_ADD:
			case MXF_MATHOP_SUBTRACT:
				old_partial_value = 0.0;
				break;
			case MXF_MATHOP_MULTIPLY:
			case MXF_MATHOP_DIVIDE:
				old_partial_value = 1.0;
				break;
			}
		} else {
			if ( record_array[0] == (MX_RECORD *) NULL ) {
				old_partial_value = value_array[0];
			} else {
				mx_status = mxv_mathop_get_value(
							record_array[0],
							&old_partial_value );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				value_array[0] = old_partial_value;
			}
		}

		for ( i = 1; i < mathop_variable->num_items; i++ ) {

			if ( mathop_variable->item_to_change != i ) {

				if ( record_array[i] == (MX_RECORD *) NULL ) {
					temp_value = value_array[i];
				} else {
					mx_status = mxv_mathop_get_value(
							record_array[i],
							&temp_value );

					if ( mx_status.code != MXE_SUCCESS )
						return mx_status;

					value_array[i] = temp_value;
				}

				switch( mathop_variable->operation_type ) {
				case MXF_MATHOP_ADD:
					old_partial_value += temp_value;
					break;
				case MXF_MATHOP_SUBTRACT:
					old_partial_value -= temp_value;
					break;
				case MXF_MATHOP_MULTIPLY:
					old_partial_value *= temp_value;
					break;
				case MXF_MATHOP_DIVIDE:
					old_partial_value = mx_divide_safely(
							old_partial_value,
							temp_value );
					break;
				}
			}
		}
	
		/* Now that we have the new value and the old partial value,
		 * we may compute the new item value.  The algorithm is
		 *
		 * If item_to_change == 0,
		 *
		 * Add or Subtract:
		 *     new_value = new_item_value + old_partial_value;
		 *
		 * Multiply or Divide:
		 *     new_value = new_item_value * old_partial_value;
		 *
		 * If item_to_change > 0,
		 *
		 * Add:
		 *     new_value = old_partial_value + new_item_value;
		 *
		 * Subtract:
		 *     new_value = old_partial_value - new_item_value;
		 *
		 * Multiply:
		 *     new_value = old_partial_value * new_item_value;
		 *
		 * Divide:
		 *     new_value = old_partial_value / new_item_value;
		 */

		if ( mathop_variable->item_to_change == 0 ) {

			switch( mathop_variable->operation_type ) {
			case MXF_MATHOP_ADD:
			case MXF_MATHOP_SUBTRACT:
				new_item_value = new_value - old_partial_value;
				break;
			case MXF_MATHOP_MULTIPLY:
			case MXF_MATHOP_DIVIDE:
				new_item_value = mx_divide_safely( new_value,
							old_partial_value );
				break;
			}
		} else {
			switch( mathop_variable->operation_type ) {
			case MXF_MATHOP_ADD:
				new_item_value = new_value - old_partial_value;
				break;
			case MXF_MATHOP_SUBTRACT:
				new_item_value = old_partial_value - new_value;
				break;
			case MXF_MATHOP_MULTIPLY:
				new_item_value = mx_divide_safely( new_value,
							old_partial_value );
				break;
			case MXF_MATHOP_DIVIDE:
				new_item_value = mx_divide_safely(
							old_partial_value,
							new_value );
				break;
			}
		}

		MX_DEBUG( 2,("%s: old_partial_value = %g",
					fname, old_partial_value ));
	    }
	} else {
	    switch( mathop_variable->operation_type ) {
	    case MXF_MATHOP_ADD:
	    case MXF_MATHOP_SUBTRACT:
	    case MXF_MATHOP_MULTIPLY:
	    case MXF_MATHOP_DIVIDE:
		new_item_value = new_value;
		break;

	    case MXF_MATHOP_NEGATE:
		new_item_value = -new_value;
		break;

	    case MXF_MATHOP_SQUARE:
		if ( new_value < 0.0 ) {
			new_item_value = 0.0;
		} else {
			new_item_value = sqrt( new_value );
		}
		break;

	    case MXF_MATHOP_SQUARE_ROOT:
		new_item_value = new_value * new_value;
		break;

	    case MXF_MATHOP_SINE:
		if ( new_value > 1.0 ) {
			new_value = 1.0;
		} else if ( new_value < -1.0 ) {
			new_value = 1.0;
		}
		new_item_value = asin( new_value );
		break;

	    case MXF_MATHOP_COSINE:
		if ( new_value > 1.0 ) {
			new_value = 1.0;
		} else if ( new_value < -1.0 ) {
			new_value = 1.0;
		}
		new_item_value = acos( new_value );
		break;

	    case MXF_MATHOP_TANGENT:
		new_item_value = atan( new_value );
		break;

	    case MXF_MATHOP_ARC_SINE:
		new_item_value = sin( new_value );
		break;

	    case MXF_MATHOP_ARC_COSINE:
		new_item_value = cos( new_value );
		break;

	    case MXF_MATHOP_ARC_TANGENT:
		new_item_value = tan( new_value );
		break;

	    case MXF_MATHOP_EXPONENTIAL:
		if ( new_value <= 0.0 ) {
			new_item_value = - DBL_MAX;
		} else {
			new_item_value = log( new_value );
		}
		break;

	    case MXF_MATHOP_LOGARITHM:
		new_item_value = exp( new_value );
		break;

	    case MXF_MATHOP_LOG10:
		new_item_value = pow( new_value, 10.0 );
		break;

	    default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Mathop operation type %d is unsupported.",
			mathop_variable->operation_type );
		break;
	    }
	}

	/* Now that we have the new value for the item to change,
	 * assign the new value to the item.
	 */

	MX_DEBUG( 2,("%s: new_item_value = %g", fname, new_item_value));

	mx_status = mxv_mathop_put_value(
			record_array[ mathop_variable->item_to_change ],
			new_item_value,
			mathop_variable->mathop_flags );

	return mx_status;
}

