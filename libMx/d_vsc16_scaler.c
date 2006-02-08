/*
 * Name:    d_vsc16_scaler.c
 *
 * Purpose: MX driver for a Joerger VSC-16 counter channel used as a scaler.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2002, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_measurement.h"
#include "mx_stdint.h"
#include "mx_vme.h"
#include "mx_scaler.h"
#include "i_vsc16.h"
#include "d_vsc16_scaler.h"

MX_RECORD_FUNCTION_LIST mxd_vsc16_scaler_record_function_list = {
	NULL,
	mxd_vsc16_scaler_create_record_structures,
	mxd_vsc16_scaler_finish_record_initialization,
	NULL,
	mxd_vsc16_scaler_print_structure,
	NULL,
	NULL,
	mxd_vsc16_scaler_open,
	NULL
};

MX_SCALER_FUNCTION_LIST mxd_vsc16_scaler_scaler_function_list = {
	mxd_vsc16_scaler_clear,
	mxd_vsc16_scaler_overflow_set,
	mxd_vsc16_scaler_read,
	NULL,
	mxd_vsc16_scaler_is_busy,
	mxd_vsc16_scaler_start,
	mxd_vsc16_scaler_stop,
	mxd_vsc16_scaler_get_parameter,
	mxd_vsc16_scaler_set_parameter,
	mxd_vsc16_scaler_set_modes_of_associated_counters
};

/* EPICS scaler data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_vsc16_scaler_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCALER_STANDARD_FIELDS,
	MXD_VSC16_SCALER_STANDARD_FIELDS
};

mx_length_type mxd_vsc16_scaler_num_record_fields
		= sizeof( mxd_vsc16_scaler_record_field_defaults )
		  / sizeof( mxd_vsc16_scaler_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_vsc16_scaler_rfield_def_ptr
			= &mxd_vsc16_scaler_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_vsc16_scaler_get_pointers( MX_SCALER *scaler,
			MX_VSC16_SCALER **vsc16_scaler,
			MX_VSC16 **vsc16,
			const char *calling_fname )
{
	static const char fname[] = "mxd_vsc16_scaler_get_pointers()";

	MX_RECORD *vsc16_record;

	if ( scaler == (MX_SCALER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_SCALER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( vsc16_scaler == (MX_VSC16_SCALER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VSC16_SCALER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( vsc16 == (MX_VSC16 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VSC16 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*vsc16_scaler = (MX_VSC16_SCALER *) scaler->record->record_type_struct;

	if ( *vsc16_scaler == (MX_VSC16_SCALER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_VSC16_SCALER pointer for record '%s' is NULL.",
			scaler->record->name );
	}

	vsc16_record = (*vsc16_scaler)->vsc16_record;

	if ( vsc16_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The vsc16_record pointer for record '%s' is NULL.",
			scaler->record->name );
	}

	*vsc16 = (MX_VSC16 *) vsc16_record->record_type_struct;

	if ( *vsc16 == (MX_VSC16 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_VSC16 pointer for record '%s' is NULL.",
			scaler->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*======*/

MX_EXPORT mx_status_type
mxd_vsc16_scaler_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_vsc16_scaler_create_record_structures()";

	MX_SCALER *scaler;
	MX_VSC16_SCALER *vsc16_scaler;

	/* Allocate memory for the necessary structures. */

	scaler = (MX_SCALER *) malloc( sizeof(MX_SCALER) );

	if ( scaler == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SCALER structure." );
	}

	vsc16_scaler = (MX_VSC16_SCALER *)
				malloc( sizeof(MX_VSC16_SCALER) );

	if ( vsc16_scaler == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_VSC16_SCALER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = scaler;
	record->record_type_struct = vsc16_scaler;
	record->class_specific_function_list
			= &mxd_vsc16_scaler_scaler_function_list;

	scaler->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_vsc16_scaler_finish_record_initialization( MX_RECORD *record )
{
	mx_status_type mx_status;

	mx_status = mx_scaler_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_vsc16_scaler_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_vsc16_scaler_print_structure()";

	MX_SCALER *scaler;
	MX_VSC16_SCALER *vsc16_scaler;
	MX_VSC16 *vsc16;
	int32_t current_value;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	scaler = (MX_SCALER *) (record->record_class_struct);

	mx_status = mxd_vsc16_scaler_get_pointers( scaler,
					&vsc16_scaler, &vsc16, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "SCALER parameters for scaler '%s':\n", record->name);

	fprintf(file, "  Scaler type           = VSC16_SCALER.\n\n");
	fprintf(file, "  VSC16 record          = %s\n", vsc16->record->name);
	fprintf(file, "  counter number        = %d\n",
					vsc16_scaler->counter_number);

	mx_status = mx_scaler_read( record, &current_value );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read value of scaler '%s'",
			record->name );
	}

	fprintf(file, "  present value         = %ld\n", (long) current_value);

	mx_status = mx_scaler_get_dark_current( record, NULL );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read the dark current for scaler '%s'",
			record->name );
	}

	fprintf(file, "  dark current          = %g counts per second.\n",
						scaler->dark_current);
	fprintf(file, "  scaler flags          = %#lx\n",
					(unsigned long) scaler->scaler_flags);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_vsc16_scaler_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_vsc16_scaler_open()";

	MX_SCALER *scaler;
	MX_VSC16_SCALER *vsc16_scaler;
	MX_VSC16 *vsc16;
	int counter_number;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	scaler = (MX_SCALER *) (record->record_class_struct);

	mx_status = mxd_vsc16_scaler_get_pointers( scaler,
					&vsc16_scaler, &vsc16, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

        /* Check that the counter number is valid. */

        counter_number = vsc16_scaler->counter_number;

        if ( counter_number < 1 || counter_number > vsc16->num_counters ) {
                return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
        "VSC16 counter number %d for scaler '%s' is out of allowed range 1-%lu",
                        counter_number, record->name, vsc16->num_counters );
        }

	vsc16->counter_record[ counter_number - 1 ] = record;

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_vsc16_scaler_clear( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_vsc16_scaler_clear()";

	MX_VSC16_SCALER *vsc16_scaler;
	MX_VSC16 *vsc16;
	unsigned long preset_address;
	mx_status_type mx_status;

	mx_status = mxd_vsc16_scaler_get_pointers( scaler,
					&vsc16_scaler, &vsc16, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Clear the counter by overwriting its preset. */

	preset_address = vsc16->base_address + MX_VSC16_PRESET_BASE
				+ 4 * ( vsc16_scaler->counter_number - 1 );

	mx_status = mx_vme_out32( vsc16->vme_record,
				vsc16->crate_number,
				MXF_VME_A32,
				preset_address,
				0x0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	scaler->value = 0.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_vsc16_scaler_overflow_set( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_vsc16_scaler_overflow_set()";

	MX_VSC16_SCALER *vsc16_scaler;
	MX_VSC16 *vsc16;
	mx_status_type mx_status;

	mx_status = mxd_vsc16_scaler_get_pointers( scaler,
					&vsc16_scaler, &vsc16, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The VSC16 will disarm itself and stop counting when an overflow
	 * occurs.  Check to see if the VSC16 is still counting.  If so,
	 * then there has been no overflow.
	 */

	mx_status = mxd_vsc16_scaler_is_busy( scaler );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scaler->busy == TRUE ) {
		scaler->overflow_set = FALSE;

		return MX_SUCCESSFUL_RESULT;
	}

	/* FIXME: UNTESTED ASSUMPTION
	 *
	 * If the VSC16 has stopped with this channel at its maximum
	 * possible value, then we assume that the counter has overflowed.
	 */

	mx_status = mxd_vsc16_scaler_read( scaler );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( scaler->raw_value == 0xffffffff ) {
		scaler->overflow_set = TRUE;
	} else {
		scaler->overflow_set = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_vsc16_scaler_read( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_vsc16_scaler_read()";

	MX_VSC16_SCALER *vsc16_scaler;
	MX_VSC16 *vsc16;
	uint32_t counter_value;
	unsigned long read_address;
	mx_status_type mx_status;

	mx_status = mxd_vsc16_scaler_get_pointers( scaler,
					&vsc16_scaler, &vsc16, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read the counter. */

	read_address = vsc16->base_address + MX_VSC16_READ_BASE
				+ 4 * ( vsc16_scaler->counter_number - 1 );

	mx_status = mx_vme_in32( vsc16->vme_record,
				vsc16->crate_number,
				MXF_VME_A32,
				read_address,
				&counter_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	scaler->raw_value = (long) counter_value;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_vsc16_scaler_is_busy( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_vsc16_scaler_is_busy()";

	MX_VSC16_SCALER *vsc16_scaler;
	MX_VSC16 *vsc16;
	uint16_t control_register;
	mx_status_type mx_status;

	mx_status = mxd_vsc16_scaler_get_pointers( scaler,
					&vsc16_scaler, &vsc16, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_vme_in16( vsc16->vme_record,
				vsc16->crate_number,
				MXF_VME_A32,
				vsc16->base_address + MX_VSC16_CONTROL_REGISTER,
				&control_register );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* See if the ARM bit is set.  The counter will disarm at the 
	 * end of the counting cycle.
	 */

	if ( control_register & 0x1 ) {
		scaler->busy = TRUE;
	} else {
		scaler->busy = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_vsc16_scaler_start( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_vsc16_scaler_start()";

	MX_VSC16_SCALER *vsc16_scaler;
	MX_VSC16 *vsc16;
	MX_RECORD *vme_record;
	uint32_t preset;
	unsigned long crate, base, preset_address;
	int counter_index;
	mx_status_type mx_status;

	mx_status = mxd_vsc16_scaler_get_pointers( scaler,
					&vsc16_scaler, &vsc16, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	counter_index = vsc16_scaler->counter_number - 1;

	vme_record = vsc16->vme_record;
	crate      = vsc16->crate_number;
	base       = vsc16->base_address;

	/* Set the scaler preset. */

	preset = scaler->value;

	preset_address = base + MX_VSC16_PRESET_BASE + 4 * counter_index;

	mx_status = mx_vme_out32( vme_record, crate, MXF_VME_A32,
					preset_address,
					preset );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Start the scaler counting:
	 *
	 *    Configure the counter to disarm on internal IRQ and 
	 *    set the arm bit.
	 */

	mx_status = mx_vme_out16( vme_record, crate, MXF_VME_A32,
					base + MX_VSC16_CONTROL_REGISTER,
					0x1 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_vsc16_scaler_stop( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_vsc16_scaler_stop()";

	MX_VSC16_SCALER *vsc16_scaler;
	MX_VSC16 *vsc16;
	mx_status_type mx_status;

	mx_status = mxd_vsc16_scaler_get_pointers( scaler,
					&vsc16_scaler, &vsc16, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Stop the scaler from counting by disarming it. */

	mx_status = mx_vme_out16( vsc16->vme_record,
				vsc16->crate_number,
				MXF_VME_A32,
				vsc16->base_address + MX_VSC16_CONTROL_REGISTER,
				0x0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	scaler->value = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_vsc16_scaler_get_parameter( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_vsc16_timer_get_parameter()";

	MX_VSC16_SCALER *vsc16_scaler;
	MX_VSC16 *vsc16;
	uint16_t direction_register, mask;
	mx_status_type mx_status;

	mx_status = mxd_vsc16_scaler_get_pointers( scaler,
					&vsc16_scaler, &vsc16, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( scaler->parameter_type ) {
	case MXLV_SCL_MODE:
		/* The counter mode may be found by reading
		 * the direction register.
		 */

		mx_status = mx_vme_in16( vsc16->vme_record,
				vsc16->crate_number,
				MXF_VME_A32,
			vsc16->base_address + MX_VSC16_DIRECTION_REGISTER,
				&direction_register );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mask = 1 << ( vsc16_scaler->counter_number - 1 );

		/* If the mask bit is set, the counter is counting down and is
		 * serving as a preset.  Otherwise, it is counting up.
		 */

		if ( direction_register & mask ) {
			scaler->mode = MXCM_PRESET_MODE;
		} else {
			scaler->mode = MXCM_COUNTER_MODE;
		}
		break;

	default:
		mx_status = mx_scaler_default_get_parameter_handler( scaler );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_vsc16_scaler_set_parameter( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_vsc16_scaler_set_parameter()";

	MX_VSC16_SCALER *vsc16_scaler;
	MX_VSC16 *vsc16;
	MX_RECORD *vme_record;
	uint16_t direction_register, mask;
	unsigned long crate, base;
	int existing_mode;
	mx_status_type mx_status;

	mx_status = mxd_vsc16_scaler_get_pointers( scaler,
					&vsc16_scaler, &vsc16, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	vme_record = vsc16->vme_record;
	crate      = vsc16->crate_number;
	base       = vsc16->base_address;

	switch( scaler->parameter_type ) {
	case MXLV_SCL_MODE:
		/* See if the counter is already in the correct mode by reading
		 * the direction register.
		 */

		mx_status = mx_vme_in16( vme_record, crate, MXF_VME_A32,
					base + MX_VSC16_DIRECTION_REGISTER,
					&direction_register );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mask = 1 << ( vsc16_scaler->counter_number - 1 );

		/* If the mask bit is set, the counter is counting down and is
		 * serving as a preset.  Otherwise, it is counting up.
		 */

		if ( direction_register & mask ) {
			existing_mode = MXCM_PRESET_MODE;
		} else {
			existing_mode = MXCM_COUNTER_MODE;
		}

		/* If we are already in the correct mode, then we can exit. */

		if ( existing_mode == scaler->mode )
			return MX_SUCCESSFUL_RESULT;

		/* Otherwise, we must reprogram the direction register. */

		switch( scaler->mode ) {
		case MXCM_PRESET_MODE:
			direction_register |= mask;
			break;

		case MXCM_COUNTER_MODE:
			direction_register &= ~mask;
			break;

		default:
			return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported preset mode %d requested for VSC16 scaler '%s'.  "
		"Only preset mode and counter mode are supported.",
				scaler->mode, scaler->record->name );
			break;
		}

		mx_status = mx_vme_out16( vme_record, crate, MXF_VME_A32,
					base + MX_VSC16_DIRECTION_REGISTER,
					direction_register );
		break;

	default:
		mx_status = mx_scaler_default_set_parameter_handler( scaler );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_vsc16_scaler_set_modes_of_associated_counters( MX_SCALER *scaler )
{
	static const char fname[]
		= "mxd_vsc16_scaler_set_modes_of_associated_counters()";

	MX_VSC16_SCALER *vsc16_scaler;
	MX_VSC16 *vsc16;
	uint16_t direction_register;
	mx_status_type mx_status;

	mx_status = mxd_vsc16_scaler_get_pointers( scaler,
					&vsc16_scaler, &vsc16, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* We must set the bit for this counter to 1 (count down) and
	 * set the bits for all the other counters to 0 (count up).
	 */

	direction_register = 1 << ( vsc16_scaler->counter_number - 1 );

	mx_status = mx_vme_out16( vsc16->vme_record,
				vsc16->crate_number,
				MXF_VME_A32,
			vsc16->base_address + MX_VSC16_DIRECTION_REGISTER,
				direction_register );

	return mx_status;
}

