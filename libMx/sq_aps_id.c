/*
 * Name:    sq_aps_id.c
 *
 * Purpose: Driver for quick scans that use an MX MCS record to collect
 *          the data and simultaneously synchronously scan an Advanced
 *          Photon Source insertion device.  This scan type is a variant
 *          of the MCS quick scan and uses most of the same code.
 * 
 * Author:  William Lavender
 *
 * Requirements:  This scan type requires the presence of three MX database
 *          records with the following names:
 *
 *          id_ev         - An 'aps_gap' type MX record setup to control the
 *                          APS id gap in units of X-ray energy (eV).
 *
 *          id_ev_enabled - '1' means coupled insertion device moves are
 *                          enabled.  '0' means they are disabled.
 *
 *          id_ev_params  - A two element array of doubles whose elements are
 *
 *              id_ev_params[0] = undulator harmonic number.  The APS MEDM
 *                                controls have an alternate and better way
 *                                of setting the undulator harmonic, so this
 *                                value should be left as '1'.
 *
 *              id_ev_params[1] = offset between the commanded insertion device
 *                                position and the commanded monochromator
 *                                position in eV.
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2000-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mxconfig.h"

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_variable.h"
#include "mx_constants.h"
#include "mx_epics.h"
#include "mx_scan.h"
#include "mx_scan_quick.h"
#include "mx_mcs.h"
#include "sq_mcs.h"
#include "sq_aps_id.h"
#include "d_aps_gap.h"
#include "d_mcs_scaler.h"

#if HAVE_EPICS

#if 0
#define PRINT_SPEED( number ) \
		do { \
			MX_RECORD *theta; \
			double speed; \
			theta = mx_get_record( scan->record, "theta_real" ); \
			(void) mx_motor_get_speed( theta, &speed ); \
			MX_DEBUG(-2,("%s( %g ): *** THETA SPEED = %g ***", \
				fname, number, speed)); \
		} while (0)
#endif

MX_RECORD_FUNCTION_LIST mxs_apsid_quick_scan_record_function_list = {
	mxs_mcs_quick_scan_initialize_type,
	mxs_apsid_quick_scan_create_record_structures,
	mxs_mcs_quick_scan_finish_record_initialization,
	mxs_mcs_quick_scan_delete_record,
	mx_quick_scan_print_scan_structure
};

MX_SCAN_FUNCTION_LIST mxs_apsid_quick_scan_scan_function_list = {
	mxs_apsid_quick_scan_prepare_for_scan_start,
	mxs_apsid_quick_scan_execute_scan_body,
	mxs_apsid_quick_scan_cleanup_after_scan_end
};

MX_RECORD_FIELD_DEFAULTS mxs_apsid_quick_scan_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCAN_STANDARD_FIELDS,
	MX_QUICK_SCAN_STANDARD_FIELDS
};

mx_length_type mxs_apsid_quick_scan_num_record_fields
			= sizeof( mxs_apsid_quick_scan_defaults )
			/ sizeof( mxs_apsid_quick_scan_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxs_apsid_quick_scan_def_ptr
			= &mxs_apsid_quick_scan_defaults[0];

static mx_status_type
mxs_apsid_quick_scan_find_parameters( MX_SCAN *scan,
		MX_APSID_QUICK_SCAN_EXTENSION *apsid_quick_scan_extension )
{
	static const char fname[] = "mxs_apsid_quick_scan_find_parameters()";

	MX_RECORD *parameters_record, *id_ev_record;
	MX_APS_GAP *aps_gap_struct;
	int32_t id_ev_enabled, undulator_harmonic;
	mx_length_type num_parameters;
	double d_spacing;
	double *double_array;
	char **string_array;
	void *pointer_to_value;
	long field_type;
	mx_length_type num_dimensions, *dimension_array;
	mx_status_type mx_status;

	/* Find the APS insertion device quick scan parameters record. */

	parameters_record = mx_get_record( scan->record,
					MX_APSID_PARAMS_RECORD_NAME );

	if ( parameters_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"The APS insertion device quick scan parameters record '%s' "
		"is not defined in the current database.  This quick scan "
		"cannot proceed without it.", MX_APSID_PARAMS_RECORD_NAME );
	}

	/* This variable should be a two dimensional array of strings. */

	mx_status = mx_get_variable_parameters( parameters_record,
				&num_dimensions, &dimension_array,
				&field_type, &pointer_to_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( field_type != MXFT_STRING ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The APS insertion device quick scan parameters record '%s' "
		"is not an array of strings.", parameters_record->name );
	}
	if ( num_dimensions != 2 ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The APS insertion device quick scan parameters record '%s' "
		"should have 2 dimensions, but actually has %ld.",
			parameters_record->name, (long) num_dimensions );
	}
	if ( dimension_array[0] != MX_APSID_NUM_PARAMS ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The APS insertion device quick scan parameters record '%s' "
		"should be an array of %d strings, but actually contains "
		"%ld strings.", parameters_record->name, MX_APSID_NUM_PARAMS,
			(long) dimension_array[0] );
	}

	string_array = (char **) pointer_to_value;

#if 0
	{
		long i;

		for ( i = 0; i < MX_APSID_NUM_PARAMS; i++ ) {
			MX_DEBUG(-2,("%s: string_array[%ld] = '%s'",
				fname, i, string_array[i]));
		}
	}
#endif

	/* Find the insertion device control record. */

	id_ev_record = mx_get_record( scan->record,
				string_array[ MX_APSID_ID_EV_RECORD_NAME ] );

	if ( id_ev_record == NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
"The insertion device record '%s' was not found, so the APS "
"insertion device quick scan '%s' cannot be run.  Scan aborted.",
			string_array[ MX_APSID_ID_EV_RECORD_NAME ],
			scan->record->name );
	}

	apsid_quick_scan_extension->id_ev_record = id_ev_record;

	MX_DEBUG( 2,("%s: insertion device record = '%s'",
		fname, id_ev_record->name ));

	/* Figure out which sector we are at. */

	aps_gap_struct = (MX_APS_GAP *) id_ev_record->record_type_struct;

	apsid_quick_scan_extension->sector_number
					= aps_gap_struct->sector_number;

	MX_DEBUG( 2,("%s: sector number = %d",
		fname, apsid_quick_scan_extension->sector_number ));

	/* Get the insertion device enable setting. */

	mx_status = mx_get_int32_variable_by_name( scan->record->list_head,
			string_array[ MX_APSID_ID_EV_ENABLED_NAME ],
			&id_ev_enabled );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	apsid_quick_scan_extension->id_ev_enabled = id_ev_enabled;

	MX_DEBUG( 2,("%s: id_ev_enabled = %d", fname, id_ev_enabled));

	/* Get the insertion device harmonic number and energy offset. */

	mx_status = mx_get_1d_array_by_name( scan->record->list_head,
			string_array[ MX_APSID_ID_EV_PARAMS_NAME ],
			MXFT_DOUBLE, &num_parameters, &pointer_to_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	double_array = (double *) pointer_to_value;

	apsid_quick_scan_extension->id_ev_harmonic = double_array[0];
	apsid_quick_scan_extension->id_ev_offset = double_array[1];

	MX_DEBUG( 2,("%s: id_ev_harmonic = %d, id_ev_offset = %g", fname,
			apsid_quick_scan_extension->id_ev_harmonic, 
			apsid_quick_scan_extension->id_ev_offset ));

	/* Get the undulator harmonic from EPICS. */

	mx_status = mx_get_int32_variable_by_name( scan->record->list_head,
			string_array[ MX_APSID_UNDULATOR_HARMONIC_NAME ],
			&undulator_harmonic );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	apsid_quick_scan_extension->undulator_harmonic = undulator_harmonic;

	MX_DEBUG( 2,("%s: undulator_harmonic = %ld",
				fname, (long) undulator_harmonic));

	/* Get the monochromator crystal D-spacing. */

	mx_status = mx_get_double_variable_by_name( scan->record->list_head,
			string_array[ MX_APSID_D_SPACING_NAME ], &d_spacing );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	apsid_quick_scan_extension->d_spacing = d_spacing;

	MX_DEBUG( 2,("%s: d_spacing = %g", fname, d_spacing));

	/* Initialize any MX EPICS data structures that need it. */

	if ( apsid_quick_scan_extension->busy_pv.pvname[0] == '\0' ) {

		mx_epics_pvname_init(
			&(apsid_quick_scan_extension->busy_pv),
			"ID%02d:Busy.VAL",
				apsid_quick_scan_extension->sector_number );

		mx_epics_pvname_init(
			&(apsid_quick_scan_extension->energy_set_pv),
			"ID%02d:EnergySet.VAL",
				apsid_quick_scan_extension->sector_number );

		mx_epics_pvname_init(
			&(apsid_quick_scan_extension->gap_set_pv),
			"ID%02d:GapSet.VAL",
				apsid_quick_scan_extension->sector_number );

		mx_epics_pvname_init(
			&(apsid_quick_scan_extension->harmonic_value_pv),
			"ID%02d:HarmonicValue.VAL",
				apsid_quick_scan_extension->sector_number );

		mx_epics_pvname_init(
			&(apsid_quick_scan_extension->message1_pv),
			"ID%02d:Message1.VAL",
				apsid_quick_scan_extension->sector_number );

		mx_epics_pvname_init(
			&(apsid_quick_scan_extension->ss_end_gap_pv),
			"ID%02d:SSEndGap.VAL",
				apsid_quick_scan_extension->sector_number );

		mx_epics_pvname_init(
			&(apsid_quick_scan_extension->ss_start_pv),
			"ID%02d:SSStart.VAL",
				apsid_quick_scan_extension->sector_number );

		mx_epics_pvname_init(
			&(apsid_quick_scan_extension->ss_start_gap_pv),
			"ID%02d:SSStartGap.VAL",
				apsid_quick_scan_extension->sector_number );

		mx_epics_pvname_init(
			&(apsid_quick_scan_extension->ss_state_pv),
			"ID%02d:SSState.VAL",
				apsid_quick_scan_extension->sector_number );

		mx_epics_pvname_init(
			&(apsid_quick_scan_extension->ss_time_pv),
			"ID%02d:SSTime.VAL",
				apsid_quick_scan_extension->sector_number );

		mx_epics_pvname_init(
			&(apsid_quick_scan_extension->sync_scan_mode_pv),
			"ID%02d:SyncScanMode.VAL",
				apsid_quick_scan_extension->sector_number );

	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxs_apsid_quick_scan_compute_gap(
		MX_APSID_QUICK_SCAN_EXTENSION *apsid_quick_scan_extension,
		MX_RECORD *motor_record,
		double position,
		double *gap )
{
	static const char fname[] = "mxs_apsid_quick_scan_compute_gap()";

	double energy, d_spacing, gap_calculated, gap_from_epics;
	int sector_number;
	mx_status_type mx_status;

	sector_number = apsid_quick_scan_extension->sector_number;

	d_spacing = apsid_quick_scan_extension->d_spacing;

	/* Keep the compiler from complaining about unused variables. */

	gap_calculated = gap_from_epics = 0.0;

	/* Convert the position into a monochromator X-ray energy. */

	switch( motor_record->mx_type ) {
	case MXT_MTR_ENERGY:
		energy = position;
		break;
	case MXT_MTR_WAVELENGTH:
		energy = mx_divide_safely( MX_HC, position );
		break;
	case MXT_MTR_WAVENUMBER:
		energy = mx_divide_safely( MX_HC * position, 2.0 * MX_PI );
		break;
	default:
		/* All other motor types are assumed to be theta in degrees. */

		energy = mx_divide_safely( MX_HC,
					2.0 * d_spacing * sin( position ) );
		break;
	}

	MX_DEBUG( 2,
		("%s: position = %g, energy = %g", fname, position, energy));

	/* Given the needed X-ray energy, there are two ways of calculating
	 * the required gap.
	 */

#if 0
	{
		/* One way is to calculate it approximately from first
		 * principles.
		 *
		 * On-axis magnetic field in Tesla from the gap in mm:
		 *
		 *    B = B0 * exp( - pi * gap / lambda_u  )
		 *
		 *    where lambda_u is the undulator period and B0 is
		 *    constant that depends on the strength of the
		 *    magnet poles.
		 *
		 * Deflection parameter K:
		 *
		 *    K = 0.0934 * lambda_u * B
		 *
		 *    where lambda_u is in mm and B is in tesla.
		 *
		 * First harmonic on-axis wavelength lambda_1:
		 *
		 *                   lambda_u
		 *    lambda_1 = ---------------- * ( 1 + 0.5 * K**2 )
		 *               ( 2 * gamma_e**2 )
		 *
		 * Finally, the energy:
		 *
		 *    E_n = n * h * c / lambda_1
		 *
		 *    where n is the undulator harmonic number.
		 *
		 * For APS undulator A, gamma_e = 13698.6, lambda_u = 33 mm
		 * and B0 is approximately 2.38 tesla.
		 */

		static double gamma_e = 13698.6;/* for 7 GeV electrons */
		static double lambda_u = 33.0;	/* in mm */
		static double B0 = 2.38;	/* in tesla */

		double lambda_1, lambda_ratio;
		double B, K, K_squared;
		long harmonic;

		/* Get the current undulator harmonic setting. */

		sprintf( pvname, "ID%02d:HarmonicValue.VAL", sector_number );

		mx_status = mx_caget_by_name( pvname, MX_CA_LONG, 1, &harmonic );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		MX_DEBUG( 2,("%s: harmonic = %ld", fname, harmonic));

		lambda_1 = mx_divide_safely( MX_HC * (double)harmonic, energy );

		MX_DEBUG( 2,("%s: lambda_1 = %g", fname, lambda_1));

		/* lambda_1 is in angstroms, while lambda_u is in mm. */

		lambda_ratio = 1.0e-7 * lambda_1 / lambda_u;

		MX_DEBUG( 2,("%s: lambda_ratio = %g", fname, lambda_ratio));

		K_squared = 2.0 *
			( 2.0 * gamma_e * gamma_e * lambda_ratio - 1.0 );

		MX_DEBUG( 2,("%s: K_squared = %g", fname, K_squared));

		if ( K_squared < 0.0 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Somehow K_squared = %g is less than zero for an energy of %g eV.  "
	"This should not be able to happen.", K_squared, energy );
		}

		K = sqrt( K_squared );

		MX_DEBUG( 2,("%s: K = %g", fname, K));

		/* Compute the on-axis magnetic field. */

		B = K / ( 0.0934 * lambda_u );

		MX_DEBUG( 2,("%s: B = %g", fname, B));

		/* Finally, compute the gap in mm. */

		gap_calculated = - ( lambda_u / MX_PI ) * log( B / B0 );

		MX_DEBUG( 2,("%s: gap_calculated = %g", fname, gap_calculated));
	}
#endif

#if 1
	{
		/* The other way is to ask the insertion device control
		 * system for the value.
		 */

		double original_gap;
		double id_ev_offset, id_ev_energy, id_ev_energy_keV;
		int id_ev_harmonic;

		/* First, save the old gap setpoint. */

		mx_status = mx_caget( &(apsid_quick_scan_extension->gap_set_pv),
					MX_CA_DOUBLE, 1, &original_gap );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Change the energy setpoint. */

		id_ev_harmonic = apsid_quick_scan_extension->id_ev_harmonic;
		id_ev_offset   = apsid_quick_scan_extension->id_ev_offset;

		id_ev_energy = mx_divide_safely( energy + id_ev_offset,
							id_ev_harmonic );

		id_ev_energy_keV = id_ev_energy / 1000.0;

		mx_status = mx_caput( &(apsid_quick_scan_extension->energy_set_pv),
					MX_CA_DOUBLE, 1, &id_ev_energy_keV );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Wait a moment for the setpoint change to propagate
		 * through EPICS.
		 */

		mx_sleep(1);

		/* This has the side effect of changing the gap setpoint
		 * to the value corresponding to the energy setpoint.
		 * We can now read that back.
		 */

		mx_status = mx_caget( &(apsid_quick_scan_extension->gap_set_pv),
					MX_CA_DOUBLE, 1, &gap_from_epics );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Finally, restore the original gap setpoint. */

		mx_status = mx_caget( &(apsid_quick_scan_extension->gap_set_pv),
					MX_CA_DOUBLE, 1, &original_gap );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		MX_DEBUG( 2,("%s: gap_from_epics = %g", fname, gap_from_epics));
	}
#endif

	*gap = gap_from_epics;

	return MX_SUCCESSFUL_RESULT;
}

/* --- */

MX_EXPORT mx_status_type
mxs_apsid_quick_scan_create_record_structures( MX_RECORD *record )
{
	mx_status_type mx_status;

	mx_status = mxs_mcs_quick_scan_create_record_structures( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Modify the superclass specific function list to point to
	 * custom versions of the scanning functions.
	 */

	record->superclass_specific_function_list
				= &mxs_apsid_quick_scan_scan_function_list;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_apsid_quick_scan_prepare_for_scan_start( MX_SCAN *scan )
{
	static const char fname[] = "mxs_apsid_quick_scan_prepare_for_scan_start()";

	MX_QUICK_SCAN *quick_scan;
	MX_MCS_QUICK_SCAN *mcs_quick_scan;
	MX_APSID_QUICK_SCAN_EXTENSION *apsid_quick_scan_extension;
	MX_RECORD *id_ev_record, *energy_record;
	MX_MOTOR *id_ev_motor;
	double id_ev_start_position, energy_start_position;
	double id_ev_harmonic, id_ev_offset;
	int busy;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	mx_status = mxs_mcs_quick_scan_get_pointers( scan,
			&quick_scan, &mcs_quick_scan, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Allocate some memory for the quick scan extension structure
	 * if it has not already been done.
	 */

	if ( mcs_quick_scan->extension_ptr != NULL ) {
		apsid_quick_scan_extension = (MX_APSID_QUICK_SCAN_EXTENSION *)
			mcs_quick_scan->extension_ptr; 
	} else {
		apsid_quick_scan_extension = (MX_APSID_QUICK_SCAN_EXTENSION *)
			malloc( sizeof(MX_APSID_QUICK_SCAN_EXTENSION) );

		if ( apsid_quick_scan_extension == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
"Ran out of memory trying to allocate an MX_APSID_QUICK_SCAN_EXTENSION "
"structure for scan record '%s'.", scan->record->name );
		}

		mcs_quick_scan->extension_ptr = apsid_quick_scan_extension;

		apsid_quick_scan_extension->busy_pv.pvname[0] = '\0';
		apsid_quick_scan_extension->energy_set_pv.pvname[0] = '\0';
		apsid_quick_scan_extension->gap_set_pv.pvname[0] = '\0';
		apsid_quick_scan_extension->harmonic_value_pv.pvname[0] = '\0';
		apsid_quick_scan_extension->message1_pv.pvname[0] = '\0';
		apsid_quick_scan_extension->ss_end_gap_pv.pvname[0] = '\0';
		apsid_quick_scan_extension->ss_start_pv.pvname[0] = '\0';
		apsid_quick_scan_extension->ss_start_gap_pv.pvname[0] = '\0';
		apsid_quick_scan_extension->ss_state_pv.pvname[0] = '\0';
		apsid_quick_scan_extension->ss_time_pv.pvname[0] = '\0';
		apsid_quick_scan_extension->sync_scan_mode_pv.pvname[0] = '\0';
	}

	/* Read in the various APS insertion device scan parameters. */

	mx_status = mxs_apsid_quick_scan_find_parameters( scan,
						apsid_quick_scan_extension );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Are insertion device coupled moves currently enabled in the
	 * MX database?
	 */

	if ( apsid_quick_scan_extension->id_ev_enabled != 1 ) {

		/* If coupled moves are not enabled, then run this scan
		 * as an ordinary MCS quick scan.
		 */

		mx_free( mcs_quick_scan->extension_ptr );

		/* Prepare for scan start. */

		mx_status = mxs_mcs_quick_scan_prepare_for_scan_start( scan );

		return mx_status;
	}

	/* If we get here, coupled insertion device moves _are_ enabled,
	 * so we need to perform extra preparation steps for this scan.
	 */

	id_ev_record = apsid_quick_scan_extension->id_ev_record;

	id_ev_motor = (MX_MOTOR *) id_ev_record->record_class_struct;

	if ( id_ev_motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MOTOR pointer for insertion device '%s' is NULL.",
			id_ev_record->name );
	}

	id_ev_harmonic = apsid_quick_scan_extension->id_ev_harmonic;
	id_ev_offset = apsid_quick_scan_extension->id_ev_offset;

	/* The monochromator energy should be the first motor. */

	energy_record = scan->motor_record_array[0];

	/* Compute the start position of the insertion device. */

	energy_start_position = quick_scan->start_position[0];

	id_ev_start_position = mx_divide_safely(
				energy_start_position + id_ev_offset,
						id_ev_harmonic );

	/* Start the insertion device moving towards the start position. */

#if 0
	mx_epics_set_debug_flag( TRUE );
#endif

	mx_info("Moving the insertion device '%s' to %g %s.",
					id_ev_record->name,
					id_ev_start_position,
					id_ev_motor->units );

	mx_status = mx_motor_move_absolute( id_ev_record,
					id_ev_start_position, MXF_MTR_NOWAIT );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now we are ready to perform the rest of the setup
	 * for the quick scan.
	 */

	MX_DEBUG( 2,("%s: before regular prepare_for_scan_start", fname));

#if 0
	PRINT_SPEED( 1.0 );
#endif

	mx_status = mxs_mcs_quick_scan_prepare_for_scan_start( scan );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: after regular prepare_for_scan_start", fname));

#if 0
	PRINT_SPEED( 2.0 );
#endif

	/* If the insertion device is still moving to the start position,
	 * wait for it to be done.
	 */

	mx_status = mx_motor_is_busy( id_ev_record, &busy );

	if ( mx_status.code != MXE_SUCCESS ) {
		(void) mx_scan_restore_speeds( scan );
		return mx_status;
	}

	if ( busy ) {
		mx_info(
	"Waiting for the insertion device '%s' to get to the start position.",
			id_ev_record->name );

		mx_status = mx_wait_for_motor_stop( id_ev_record, 0 );

		if ( mx_status.code != MXE_SUCCESS ) {
			(void) mx_scan_restore_speeds( scan );
			return mx_status;
		}
	}

	mx_info( "The insertion device '%s' is at the start position.",
			id_ev_record->name );

	MX_DEBUG( 2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_apsid_quick_scan_execute_scan_body( MX_SCAN *scan )
{
	static const char fname[] = "mxs_apsid_quick_scan_execute_scan_body()";

	MX_QUICK_SCAN *quick_scan;
	MX_MCS_QUICK_SCAN *mcs_quick_scan;
	MX_APSID_QUICK_SCAN_EXTENSION *apsid_quick_scan_extension;
	MX_RECORD *id_ev_record;
	MX_RECORD *input_device_record;
	MX_RECORD *mcs_record;
	MX_MCS_SCALER *mcs_scaler;
	int sector_number;
	unsigned long num_elements;
	char message1[100];
	double starting_gap, ending_gap, gap_scan_time;
	long i, busy, stop, start;
	char status_byte;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	mx_status = mxs_mcs_quick_scan_get_pointers( scan,
			&quick_scan, &mcs_quick_scan, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If the insertion device coupling is turned off, the pointer
	 * mcs_quick_scan->extension_ptr will be NULL.  In that case,
	 * just run this as a normal MCS quick scan.
	 */

	if ( mcs_quick_scan->extension_ptr == NULL ) {
		mx_status = mxs_mcs_quick_scan_execute_scan_body( scan );

		return mx_status;
	}

	MX_DEBUG( 2,("%s: EPICS debug flag = %d",
				fname, mx_epics_get_debug_flag() ));

	/* Get some values that we will need. */

	apsid_quick_scan_extension = (MX_APSID_QUICK_SCAN_EXTENSION *)
					mcs_quick_scan->extension_ptr;

	id_ev_record = apsid_quick_scan_extension->id_ev_record;

	sector_number = apsid_quick_scan_extension->sector_number;

	/* Find the MCS record via the first input device record, which
	 * should be an MCS scaler record.
	 */

	if ( scan->num_input_devices <= 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The APS insertion device scan '%s' did not define any input devices.",
			scan->record->name );
	}

	input_device_record = ( scan->input_device_array )[0];

	mcs_scaler = (MX_MCS_SCALER *) input_device_record->record_type_struct;

	mcs_record = mcs_scaler->mcs_record;

	/**** Configure the insertion device for synchronous scanning. ****/

	/* Set the gap to end at. */

	mx_status = mxs_apsid_quick_scan_compute_gap(
					apsid_quick_scan_extension,
					(scan->motor_record_array)[0],
					(quick_scan->end_position)[0],
					&ending_gap );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_caput( &(apsid_quick_scan_extension->ss_end_gap_pv),
				MX_CA_DOUBLE, 1, &ending_gap );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
"The attempt to use synchronous scanning for APS sector %d-ID failed.  "
"Perhaps synchronous scanning is not enabled for your sector?",
			sector_number );
	}

	/* Set the gap to start at. */

	mx_status = mxs_apsid_quick_scan_compute_gap(
					apsid_quick_scan_extension,
					(scan->motor_record_array)[0],
					(quick_scan->start_position)[0],
					&starting_gap );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_caput( &(apsid_quick_scan_extension->ss_start_gap_pv),
				MX_CA_DOUBLE, 1, &starting_gap );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the scanning time. */

	gap_scan_time = mcs_quick_scan->scan_body_time;

	if ( gap_scan_time < 0.0 ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Unable to get the undulator scanning time for scan '%s'.",
			scan->record->name );
	}

	mx_status = mx_caput( &(apsid_quick_scan_extension->ss_time_pv),
				MX_CA_DOUBLE, 1, &gap_scan_time );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Toggle the synchronous scan mode off and then back on. */

	stop = 0;

	mx_status = mx_caput( &(apsid_quick_scan_extension->sync_scan_mode_pv),
				MX_CA_LONG, 1, &stop );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	start = 1;

	mx_status = mx_caput( &(apsid_quick_scan_extension->sync_scan_mode_pv),
				MX_CA_LONG, 1, &start );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Start the undulator gap moving. */

	start = 1;

	mx_status = mx_caput( &(apsid_quick_scan_extension->ss_start_pv),
				MX_CA_LONG, 1, &start );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait for the premove to finish. */

	do {
	    /* Check the status of the synchronous scanning. */

	    mx_status = mx_caget( &(apsid_quick_scan_extension->ss_state_pv),
					MX_CA_CHAR, 1, &status_byte );

	    if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	    /* If we get here, mx_status.code == MXE_SUCCESS.  This fact
	     * is depended on by the logic of the rest of the loop.
	     */

	    /* Perform a series of tests to see if the undulator is doing
	     * the right thing.  If any of them fail, abort the scan.
	     */

	    if ( status_byte != 2 ) {

		/* Check to see of the undulator is moving. */

		mx_status = mx_caget( &(apsid_quick_scan_extension->busy_pv),
					MX_CA_LONG, 1, &busy );

		if ( mx_status.code == MXE_SUCCESS ) {
		    /* Only proceed if mx_caget_by_name() succeeded. */

		    if ( busy == 0 ) {
			/* The undulator did not start moving.  Try to
			 * find out why.
			 */

			mx_status = mx_epics_get_num_elements(
				&(apsid_quick_scan_extension->message1_pv),
				&num_elements );

			if ( mx_status.code == MXE_SUCCESS ) {
			    /* Only proceed if mx_epics_get_num_elements()
			     * succeeded.
			     */

			    mx_status = mx_caget(
				&(apsid_quick_scan_extension->message1_pv),
					MX_CA_STRING, num_elements, message1 );

			    if ( mx_status.code == MXE_SUCCESS ) {
				/* Only proceed if mx_caget_by_name()
				 * succeeded.
				 */

				/* Format the error message that we will
				* send to the users.
				*/

				mx_status = mx_error(
				    MXE_DEVICE_ACTION_FAILED, fname,
			"The undulator did not start moving.  Reason = '%s'",
				    message1 );
			    }
			}
		    }
		}
	    }

	    if ( mx_status.code == MXE_SUCCESS ) {
		/* Give the user a chance to interrupt this loop. */

		if ( mx_user_requested_interrupt() ) {
		    mx_status = mx_error( MXE_INTERRUPTED, fname,
			"The quick scan was interrupted by the user." );
		}
	    }

	    /* If anything failed above, abort the scan. */

	    if ( mx_status.code != MXE_SUCCESS ) {

		/* Stop the motors. */

		for ( i = 0; i < scan->num_motors; i++ ) {
		    (void) mx_motor_soft_abort( scan->motor_record_array[i] );
		}

		/* Stop the MCS. */

		(void) mx_mcs_stop( mcs_record );

		/* Return the reason for the abort. */

		return mx_status;
	    }

	    mx_msleep(1);  /* Give other processes a chance to run. */

	    /* End of the do...while() loop. */

	} while ( status_byte != 2 );

	/* The status byte changes to 2 at 2.5 seconds before the insertion
	 * device reaches the start position.  Thus, we must wait for an
	 * additional 2.5 seconds before starting the other motors.
	 */

	mx_msleep(2500);

	mx_info("Starting the monochromator.");

	/**** Start the rest of the motors moving. ****/

	mx_status = mxs_mcs_quick_scan_execute_scan_body( scan );

	MX_DEBUG( 2,("%s complete.", fname));

	return mx_status;
}

MX_EXPORT mx_status_type
mxs_apsid_quick_scan_cleanup_after_scan_end( MX_SCAN *scan )
{
	static const char fname[] =
		"mxs_apsid_quick_scan_cleanup_after_scan_end()";

	MX_QUICK_SCAN *quick_scan;
	MX_MCS_QUICK_SCAN *mcs_quick_scan;
	MX_APSID_QUICK_SCAN_EXTENSION *apsid_quick_scan_extension;
	long stop;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	mx_status = mxs_mcs_quick_scan_get_pointers( scan,
			&quick_scan, &mcs_quick_scan, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If the insertion device coupling was turned on, we will
	 * have to turn off synchronous scanning mode now.
	 */

	if ( mcs_quick_scan->extension_ptr != NULL ) {

		apsid_quick_scan_extension = (MX_APSID_QUICK_SCAN_EXTENSION *)
					mcs_quick_scan->extension_ptr;

		/* Turn off synchronous scan mode.  Ignore any errors about
		 * this from Channel Access, since we are going to invoke
		 * mxs_mcs_quick_scan_cleanup_after_scan_end() regardless
		 * of whether there were Channel Access errors or not.
		 */

		stop = 0;

		(void) mx_caput(
			&(apsid_quick_scan_extension->sync_scan_mode_pv),
			MX_CA_LONG, 1, &stop );
	}

	mx_status = mxs_mcs_quick_scan_cleanup_after_scan_end( scan );

#if 0
	mx_epics_set_debug_flag( FALSE );
#endif

#if 0
	PRINT_SPEED( 3.0 );
#endif

	MX_DEBUG( 2,("%s complete.", fname));

	return mx_status;
}

#endif /* HAVE_EPICS */
