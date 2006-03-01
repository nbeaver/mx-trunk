/*
 * Name:    i_aps_adcmod2.c
 *
 * Purpose: MX interface driver for the ADCMOD2 electrometer system designed
 *          by Steve Ross of the Advanced Photon Source.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2003-2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_hrt.h"
#include "mx_stdint.h"
#include "mx_vme.h"

#include "i_aps_adcmod2.h"

MX_RECORD_FUNCTION_LIST mxi_aps_adcmod2_record_function_list = {
	NULL,
	mxi_aps_adcmod2_create_record_structures,
	NULL,
	NULL,
	mxi_aps_adcmod2_print_structure,
	NULL,
	NULL,
	mxi_aps_adcmod2_open
};

MX_RECORD_FIELD_DEFAULTS mxi_aps_adcmod2_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_APS_ADCMOD2_STANDARD_FIELDS
};

long mxi_aps_adcmod2_num_record_fields
		= sizeof( mxi_aps_adcmod2_record_field_defaults )
			/ sizeof( mxi_aps_adcmod2_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_aps_adcmod2_rfield_def_ptr
			= &mxi_aps_adcmod2_record_field_defaults[0];

/* --- */

static mx_status_type
mxi_aps_adcmod2_get_pointers( MX_RECORD *record,
			MX_APS_ADCMOD2 **aps_adcmod2,
			const char *calling_fname )
{
	static const char fname[] = "mxi_aps_adcmod2_get_pointers()";

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( aps_adcmod2 == (MX_APS_ADCMOD2 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_APS_ADCMOD2 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*aps_adcmod2 = (MX_APS_ADCMOD2 *) (record->record_type_struct);

	if ( *aps_adcmod2 == (MX_APS_ADCMOD2 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_APS_ADCMOD2 pointer for record '%s' is NULL.",
			record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*=== Public functions ===*/

MX_EXPORT mx_status_type
mxi_aps_adcmod2_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_aps_adcmod2_create_record_structures()";

	MX_APS_ADCMOD2 *aps_adcmod2;
	int i;

	/* Allocate memory for the necessary structures. */

	aps_adcmod2 = (MX_APS_ADCMOD2 *) malloc( sizeof(MX_APS_ADCMOD2) );

	if ( aps_adcmod2 == (MX_APS_ADCMOD2 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_APS_ADCMOD2 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = aps_adcmod2;
	record->class_specific_function_list = NULL;

	aps_adcmod2->record = record;

	for ( i = 0; i < MX_APS_ADCMOD2_MAX_AMPLIFIERS; i++ ) {
		aps_adcmod2->amplifier_array[i] = NULL;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_aps_adcmod2_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxi_aps_adcmod2_print_structure()";

	MX_APS_ADCMOD2 *aps_adcmod2;
	MX_RECORD *this_record;
	int i;
	mx_status_type mx_status;

	mx_status = mxi_aps_adcmod2_get_pointers( record, &aps_adcmod2, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "Record '%s':\n\n", record->name);
	fprintf(file, "  name                       = \"%s\"\n",
						record->name);

	fprintf(file, "  superclass                 = interface\n");
	fprintf(file, "  class                      = generic\n");
	fprintf(file, "  type                       = APS_ADCMOD2\n");
	fprintf(file, "  label                      = \"%s\"\n",
						record->label);

	fprintf(file, "  vme record                 = \"%s\"\n",
						aps_adcmod2->vme_record->name);

	fprintf(file, "  crate_number               = %lu\n",
						aps_adcmod2->crate_number);

	fprintf(file, "  base address               = %#lx\n",
						aps_adcmod2->base_address);

	fprintf(file, "  amplifiers in use          = (");

	for ( i = 0; i < MX_APS_ADCMOD2_MAX_AMPLIFIERS; i++ ) {

		this_record = aps_adcmod2->amplifier_array[i];

		if ( this_record == NULL ) {
			fprintf( file, "NULL" );
		} else {
			fprintf( file, "%s", this_record->name );
		}
		if ( i != (MX_APS_ADCMOD2_MAX_AMPLIFIERS - 1) ) {
			fprintf( file, ", " );
		}
	}

	fprintf(file, ")\n");

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_aps_adcmod2_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_aps_adcmod2_open()";

	MX_APS_ADCMOD2 *aps_adcmod2;
	double raw_measurement_interval;	/* in seconds */
	double update_interval;			/* in seconds */
	double ticks_per_second;
	mx_status_type mx_status;

	mx_status = mxi_aps_adcmod2_get_pointers( record, &aps_adcmod2, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* --- */

	raw_measurement_interval = mx_divide_safely( 1.0,
				aps_adcmod2->raw_value_read_frequency );

	aps_adcmod2->microseconds_between_raw_measurements
				= mx_round( 1.0e6 * raw_measurement_interval );

	/* --- */

	ticks_per_second = mx_clock_ticks_per_second();

	if ( aps_adcmod2->raw_value_read_frequency > (0.5 * ticks_per_second) )
	{
		aps_adcmod2->use_udelay = TRUE;
	} else {
		aps_adcmod2->use_udelay = FALSE;
	}

	MX_DEBUG( 2,("%s: use_udelay = %d", fname, aps_adcmod2->use_udelay));

	/* --- */

	update_interval = mx_divide_safely( 1.0,
				aps_adcmod2->averaged_value_update_frequency );

	aps_adcmod2->ticks_between_averaged_measurements =
		mx_convert_seconds_to_clock_ticks( update_interval );

	aps_adcmod2->next_measurement_time = mx_current_clock_tick();

	return mx_status;
}

/* ====== */

MX_EXPORT mx_status_type
mxi_aps_adcmod2_in16( MX_APS_ADCMOD2 *aps_adcmod2, unsigned long offset,
				uint16_t *word_value )
{
	static const char fname[] = "mxi_aps_adcmod2_in16()";

	unsigned long address;
	mx_status_type mx_status;

	if ( aps_adcmod2 == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"NULL aps_adcmod2 pointer passed." );
	}
	if ( offset > 0xffffff ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal VME address offset %lu for APS_ADCMOD2 record '%s'",
				offset, aps_adcmod2->record->name );
	}

	address = aps_adcmod2->base_address + offset;

	mx_status = mx_vme_in16( aps_adcmod2->vme_record,
				aps_adcmod2->crate_number, MXF_VME_A24,
				address, word_value );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_aps_adcmod2_out16( MX_APS_ADCMOD2 *aps_adcmod2, unsigned long offset,
				uint16_t word_value )
{
	static const char fname[] = "mxi_aps_adcmod2_out16()";

	unsigned long address;
	mx_status_type mx_status;

	if ( aps_adcmod2 == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"NULL aps_adcmod2 pointer passed." );
	}
	if ( offset > 0xffffff ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal VME address offset %lu for APS_ADCMOD2 record '%s'",
				offset, aps_adcmod2->record->name );
	}

	address = aps_adcmod2->base_address + offset;

	mx_status = mx_vme_out16( aps_adcmod2->vme_record,
				aps_adcmod2->crate_number, MXF_VME_A24,
				address, word_value );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_aps_adcmod2_command( MX_APS_ADCMOD2 *aps_adcmod2, long electrometer_number,
				uint16_t command, uint16_t value )
{
	static const char fname[] = "mxi_aps_adcmod2_command()";

	unsigned long address;
	mx_status_type mx_status;

	if ( aps_adcmod2 == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"NULL aps_adcmod2 pointer passed." );
	}

	/* Write the command value. */

	address = aps_adcmod2->base_address + 0x08;

	mx_status = mx_vme_out16( aps_adcmod2->vme_record,
				aps_adcmod2->crate_number, MXF_VME_A24,
				address, value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Write the command number. */

	address = aps_adcmod2->base_address + 0x10;

	mx_status = mx_vme_out16( aps_adcmod2->vme_record,
				aps_adcmod2->crate_number, MXF_VME_A24,
				address, command );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Write the device type which is always 0xA000 for this device. */

	address = aps_adcmod2->base_address + 0x18;

	mx_status = mx_vme_out16( aps_adcmod2->vme_record,
				aps_adcmod2->crate_number, MXF_VME_A24,
				address, 0xA000 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the command. */

	address = aps_adcmod2->base_address + 0x20;

	mx_status = mx_vme_out16( aps_adcmod2->vme_record,
				aps_adcmod2->crate_number, MXF_VME_A24,
				address, 0xCCCC );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_aps_adcmod2_latch_values( MX_APS_ADCMOD2 *aps_adcmod2 )
{
	static const char fname[] = "mxi_aps_adcmod2_latch_values()";

	MX_CLOCK_TICK current_time;
	uint16_t block_read_array[2*MX_APS_ADCMOD2_MAX_INPUTS];
	double sum_array[MX_APS_ADCMOD2_MAX_INPUTS];
	int i, j, k;
	unsigned long num_measurements, sleep_usec;
	double average_value;
	mx_status_type mx_status;

	if ( aps_adcmod2 == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"NULL aps_adcmod2 pointer passed." );
	}

	MX_DEBUG( 2,("%s: base_address = %#lx",
		fname, aps_adcmod2->base_address));

	for ( j = 0; j < MX_APS_ADCMOD2_MAX_INPUTS; j++ ) {
		sum_array[j] = 0.0;
	}

	num_measurements = aps_adcmod2->num_raw_measurements_to_average;

	sleep_usec = aps_adcmod2->microseconds_between_raw_measurements;

	for ( i = 0; i < num_measurements; i++ ) {

		if ( i > 0 ) {
			if ( aps_adcmod2->use_udelay ) {
				mx_udelay( sleep_usec );
			} else {
				mx_usleep( sleep_usec );
			}
		}

		mx_status = mx_vme_multi_in16( aps_adcmod2->vme_record,
					aps_adcmod2->crate_number,
					MXF_VME_A24,
					4,
					aps_adcmod2->base_address,
					MX_APS_ADCMOD2_MAX_INPUTS,
					block_read_array );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		for ( j = 0; j < MX_APS_ADCMOD2_MAX_INPUTS; j++ ) {

			k = 2*j;

			sum_array[j] += ( ((double) block_read_array[k])
					+ ((double) block_read_array[k+1]) );

		}
	}

	for ( j = 0; j < MX_APS_ADCMOD2_MAX_INPUTS; j++ ) {
		average_value = mx_divide_safely( 0.5 * sum_array[j],
						(double) num_measurements );

		aps_adcmod2->input_value[j] =
			(uint16_t) mx_round( average_value );
	}

	current_time = mx_current_clock_tick();

	aps_adcmod2->next_measurement_time =
		mx_add_clock_ticks( current_time,
			aps_adcmod2->ticks_between_averaged_measurements );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_aps_adcmod2_read_value( MX_APS_ADCMOD2 *aps_adcmod2,
				unsigned long analog_input_number,
				uint16_t *word_value )
{
	static const char fname[] = "mxi_aps_adcmod2_read_value()";

	MX_CLOCK_TICK current_time, next_time;
	mx_status_type mx_status;

	if ( aps_adcmod2 == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"NULL aps_adcmod2 pointer passed." );
	}
	if ( word_value == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"NULL word_value pointer passed." );
	}

	if ( analog_input_number >= MX_APS_ADCMOD2_MAX_INPUTS ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"Requested analog input number %lu for APS ADCMOD2 '%s' "
		"is larger than the maximum allowed value of %d.",
			analog_input_number, aps_adcmod2->record->name,
			MX_APS_ADCMOD2_MAX_INPUTS - 1 );
	}

	next_time = aps_adcmod2->next_measurement_time;

	current_time = mx_current_clock_tick();

	if ( mx_compare_clock_ticks( next_time, current_time ) <= 0 ) {

		/* It is time for the next measurement, so take it. */

		mx_status = mxi_aps_adcmod2_latch_values( aps_adcmod2 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status; 
	}

	*word_value = aps_adcmod2->input_value[ analog_input_number ];

	return MX_SUCCESSFUL_RESULT;
}

