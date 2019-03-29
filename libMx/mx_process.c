/*
 * Name:    mx_process.c
 *
 * Purpose: MX event processing functions.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2004-2008, 2011-2017, 2019
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define PROCESS_DEBUG			FALSE

#define PROCESS_DEBUG_TIMING		FALSE

#define PROCESS_DEBUG_QUEUEING		FALSE

#define PROCESS_DEBUG_VALUE_CHANGED	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#if !defined(OS_WIN32) && !defined(OS_ECOS)
#include <sys/times.h>
#endif

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_socket.h"
#include "mx_net.h"
#include "mx_callback.h"
#include "mx_clock.h"
#include "mx_hrt_debug.h"

#include "mx_process.h"
#include "pr_handlers.h"

typedef struct {
	long mx_superclass;
	long mx_class;
	mx_status_type (*setup_fn) ( MX_RECORD * );
} MX_PROCESS_FUNCTION_SETUP;

static MX_PROCESS_FUNCTION_SETUP mx_process_function_setup_array[] = {

{ MXR_DEVICE, MXC_MOTOR, mx_setup_motor_process_functions },
{ MXR_DEVICE, MXC_SCALER, mx_setup_scaler_process_functions },
{ MXR_DEVICE, MXC_TIMER, mx_setup_timer_process_functions },
{ MXR_DEVICE, MXC_AMPLIFIER, mx_setup_amplifier_process_functions },
{ MXR_DEVICE, MXC_RELAY, mx_setup_relay_process_functions },
{ MXR_DEVICE, MXC_MULTICHANNEL_ANALYZER, mx_setup_mca_process_functions },
{ MXR_DEVICE, MXC_MULTICHANNEL_ENCODER, mx_setup_mce_process_functions },
{ MXR_DEVICE, MXC_MULTICHANNEL_SCALER, mx_setup_mcs_process_functions },
{ MXR_DEVICE, MXC_ANALOG_INPUT, mx_setup_analog_input_process_functions },
{ MXR_DEVICE, MXC_ANALOG_OUTPUT, mx_setup_analog_output_process_functions },
{ MXR_DEVICE, MXC_DIGITAL_INPUT, mx_setup_digital_input_process_functions },
{ MXR_DEVICE, MXC_DIGITAL_OUTPUT, mx_setup_digital_output_process_functions},
{ MXR_DEVICE, MXC_PULSE_GENERATOR, mx_setup_pulser_process_functions},
{ MXR_DEVICE, MXC_AUTOSCALE, mx_setup_autoscale_process_functions },
{ MXR_DEVICE, MXC_SINGLE_CHANNEL_ANALYZER, mx_setup_sca_process_functions },
{ MXR_DEVICE, MXC_SAMPLE_CHANGER, mx_setup_sample_changer_process_functions },
{ MXR_DEVICE, MXC_MULTICHANNEL_ANALOG_INPUT, mx_setup_mcai_process_functions },
{ MXR_DEVICE, MXC_PAN_TILT_ZOOM, mx_setup_ptz_process_functions },
{ MXR_DEVICE, MXC_VIDEO_INPUT, mx_setup_video_input_process_functions },
{ MXR_DEVICE, MXC_AREA_DETECTOR, mx_setup_area_detector_process_functions },
{ MXR_DEVICE, MXC_WAVEFORM_INPUT, mx_setup_waveform_input_process_functions },
{ MXR_DEVICE, MXC_WAVEFORM_OUTPUT, mx_setup_waveform_output_process_functions },
{ MXR_INTERFACE, MXI_RS232, mx_setup_rs232_process_functions },
{ MXR_INTERFACE, MXI_GPIB, mx_setup_gpib_process_functions },
{ MXR_INTERFACE, MXI_VME, mx_setup_vme_process_functions },
{ MXR_VARIABLE, MXC_ANY, mx_setup_variable_process_functions },
{ MXR_LIST_HEAD, MXC_ANY, mx_setup_list_head_process_functions },
{ MXR_SCAN, MXC_ANY, NULL },
{ MXR_SERVER, MXC_ANY, NULL },
{ MXR_OPERATION, MXC_ANY, mx_setup_operation_process_functions },
{ MXR_SPECIAL, MXC_ANY, NULL },
};

static int mx_num_process_function_setup_elements =
			sizeof( mx_process_function_setup_array )
			/ sizeof( mx_process_function_setup_array[0] );

MX_EXPORT mx_status_type
mx_initialize_database_processing( MX_RECORD *mx_record_list )
{
	MX_RECORD *current_record, *list_head_record;
	mx_status_type mx_status;

	list_head_record = mx_record_list->list_head;

	current_record = list_head_record;

	do {
		mx_status = mx_initialize_record_processing( current_record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		current_record = current_record->next_record;

	} while ( current_record != list_head_record );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_initialize_record_processing( MX_RECORD *record )
{
	static const char fname[] = "mx_initialize_record_processing()";

	MX_RECORD_FUNCTION_LIST *flist;
	mx_status_type ( *process_function_setup ) ( MX_RECORD * );
	mx_status_type ( *special_processing_setup ) ( MX_RECORD * );
	long this_superclass, this_class;
	unsigned long processing_flags;
	int i;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s: invoked for record '%s'", fname, record->name));

	process_function_setup = NULL;

	/* Do we use the default processing function for this record? */

	processing_flags = record->record_processing_flags;

	/* Has the record processing already been set up?  If so, then
	 * just return.
	 */

	if ( processing_flags & MXF_PROC_PROCESSING_IS_INITIALIZED ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* We need to set up record processing. */

	if ( processing_flags & MXF_PROC_BYPASS_DEFAULT_PROCESSING ) {

		/* No, we do not use the default processing function,
		 * so we do nothing here.
		 */

	} else {

		/* First, setup the record processing that is used for all
		 * MX drivers.
		 */

		mx_status = mx_setup_record_process_functions( record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Look for a default processing function for this class
		 * of MX drivers.
		 */

		for (i = 0; i < mx_num_process_function_setup_elements; i++ )
		{
		    this_superclass = 
			    mx_process_function_setup_array[i].mx_superclass;

		    this_class = mx_process_function_setup_array[i].mx_class;

		    if ( ( this_class == record->mx_class )
		      && ( this_superclass == record->mx_superclass ) )
		    {
			process_function_setup =
				mx_process_function_setup_array[i].setup_fn;
		    } else
		    if ( ( this_class == MXC_ANY )
		      && ( this_superclass == record->mx_superclass ) )
		    {
			process_function_setup =
				mx_process_function_setup_array[i].setup_fn;
		    }
		}

		/* If a default processing function is available, invoke it. */

		if ( process_function_setup != NULL ) {
		    mx_status = (*process_function_setup)( record );

		    if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		}
	}

	/* If a special processing function is available, invoke it. */

	flist = (MX_RECORD_FUNCTION_LIST *) record->record_function_list;

	special_processing_setup = flist->special_processing_setup;

	if ( special_processing_setup != NULL ) {
		mx_status = (*special_processing_setup)( record );

		if ( mx_status.code != MXE_SUCCESS )
		    return mx_status;
	}

	record->record_processing_flags |= MXF_PROC_PROCESSING_IS_INITIALIZED;

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_process_record_field_without_callbacks( MX_RECORD *record,
			MX_RECORD_FIELD *record_field,
			MX_SOCKET_HANDLER *socket_handler,
			int direction )
{
	static const char fname[] =
		"mx_process_record_field_without_callbacks()";

	mx_status_type (*process_fn) ( void *, void *, void *, int );
	unsigned long rp_flags;
	mx_status_type mx_status;

#if PROCESS_DEBUG_TIMING
	MX_HRT_TIMING measurement;

	MX_HRT_START( measurement );
#endif

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}
	if ( record_field == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD_FIELD pointer passed was NULL." );
	}

	/* Check to see if record processing has been set up for this record. */

	rp_flags = record->record_processing_flags;

	if ( (rp_flags & MXF_PROC_PROCESSING_IS_INITIALIZED) == 0 ) {
		mx_status = mx_initialize_record_processing( record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/*---*/

	/* If there is one, call the process function to read data
	 * from the hardware or server or whatever.
	 */

	process_fn = record_field->process_function;

#if PROCESS_DEBUG
	if ( direction == MX_PROCESS_GET ) {
		MX_DEBUG(-1,("%s: '%s.%s' MX_PROCESS_GET (%ld)",
			fname, record->name, record_field->name,
			record_field->label_value ));
	} else
	if ( direction == MX_PROCESS_PUT ) {
		MX_DEBUG(-1,("%s: '%s.%s' MX_PROCESS_PUT (%ld)",
			fname, record->name, record_field->name,
			record_field->label_value ));
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal direction flag (%d) passed.  The only "
		"allowed values are MX_PROCESS_GET (%d) and "
		"MX_PROCESS_PUT (%d).", direction,
			MX_PROCESS_GET, MX_PROCESS_PUT );
	}

	MX_DEBUG(-1,("%s: process_fn = %p", fname, process_fn));
#endif /* PROCESS_DEBUG */

	if ( process_fn == NULL ) {
		mx_status = MX_SUCCESSFUL_RESULT;
	} else {
		/* Only invoke the process function for non-negative
		 * values of record_field->label_value.  A field
		 * with a negative label value is just a placeholder
		 * for some value, so it does not need any further
		 * record processing.
		 */

#if PROCESS_DEBUG_TIMING && 0
		MX_HRT_END( measurement );

		MX_HRT_RESULTS( measurement, fname,
"#1 before record processing for '%s.%s'", record->name, record_field->name );
#endif

#if PROCESS_DEBUG
		MX_DEBUG(-1,("%s: record_field->label_value = %ld",
			fname, record_field->label_value));
#endif
		if ( record_field->label_value < 0 ) {
			return  MX_SUCCESSFUL_RESULT;
		} else {
			/* If the record field is already active, then
			 * do not process the record field.
			 */

			if ( record_field->active ) {
#if PROCESS_DEBUG
				MX_DEBUG(-1,
	    ("%s: Returning early since the record field is already active.",
	    				fname));
#endif
				return MX_SUCCESSFUL_RESULT;
			}

			record_field->active = TRUE;

			/* Invoke the record processing function. */

			mx_status = ( *process_fn )
				    ( record, record_field,
					socket_handler, direction );

			record_field->active = FALSE;

#if PROCESS_DEBUG
			MX_DEBUG(-1,(
				"%s: process_fn returned mx_status.code = %ld",
					fname, mx_status.code ));
#endif
		}

#if PROCESS_DEBUG_TIMING && 0
		MX_HRT_END( measurement );

		MX_HRT_RESULTS( measurement, fname,
"#2 after record processing for '%s.%s'", record->name, record_field->name );
#endif
	}

#if PROCESS_DEBUG
	MX_DEBUG(-2,("%s: mx_status.code = %ld", fname, mx_status.code ));
#endif

#if PROCESS_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
"- end of processing for '%s.%s'", record->name, record_field->name );
#endif

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_process_record_field( MX_RECORD *record,
			MX_RECORD_FIELD *record_field,
			MX_SOCKET_HANDLER *socket_handler,
			int direction,
			mx_bool_type *value_changed_ptr )
{
	static const char fname[] = "mx_process_record_field()";

	mx_bool_type value_changed;
	mx_status_type mx_status;

#if PROCESS_DEBUG_TIMING
	MX_HRT_TIMING measurement;

	MX_HRT_START( measurement );
#endif

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}
	if ( record_field == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD_FIELD pointer passed was NULL." );
	}

	/*---*/

	value_changed = FALSE;

	if ( value_changed_ptr != NULL ) {
		*value_changed_ptr = value_changed;
	}

	mx_status = mx_process_record_field_without_callbacks( record,
							record_field,
							socket_handler,
							direction );

#if PROCESS_DEBUG
	MX_DEBUG(-2,("%s: mx_status.code = %ld", fname, mx_status.code ));
#endif

	if ( mx_status.code == MXE_SUCCESS ) {

		/* If the process function succeeded, then we must check to
		 * see whether or not we must send value changed callbacks.
		 */

#if PROCESS_DEBUG
		MX_DEBUG(-2,
	("%s: '%s.%s' value_changed_test_function = %p, callback_list = %p",
			fname, record->name, record_field->name,
			record_field->value_changed_test_function,
			record_field->callback_list));
#endif
		mx_status = mx_test_for_value_changed( record_field,
							direction,
							&value_changed );

#if PROCESS_DEBUG
		MX_DEBUG(-2,("%s: --> value_changed = %d",
			fname, value_changed));
#endif

		if ( value_changed
		  && (record_field->callback_list != NULL ) )
		{
			mx_status = mx_local_field_invoke_callback_list(
					record_field, MXCBT_VALUE_CHANGED );
		}
	}

	if ( value_changed_ptr != NULL ) {
		*value_changed_ptr = value_changed;
	}

#if PROCESS_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
"- end of processing for '%s.%s'", record->name, record_field->name );
#endif

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_process_record_field_by_name( MX_RECORD *record,
				const char *field_name,
				MX_SOCKET_HANDLER *socket_handler,
				int direction,
				mx_bool_type *value_changed )
{
	static const char fname[] = "mx_process_record_field_by_name()";

	MX_RECORD_FIELD *field;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}
	if ( field_name == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The field name pointer passed for record '%s' is NULL.",
			record->name );
	}

	mx_status = mx_find_record_field( record, field_name, &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_process_record_field( record, field, socket_handler,
						direction, value_changed );

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_test_for_value_changed( MX_RECORD_FIELD *record_field,
				int direction,
				mx_bool_type *value_changed_ptr )
{
#if PROCESS_DEBUG_VALUE_CHANGED
	static const char fname[] = "mx_test_for_value_changed()";
#endif
	mx_status_type (*vc_test_fn)( MX_RECORD_FIELD *, int, mx_bool_type * );
	mx_status_type mx_status;

#if PROCESS_DEBUG_VALUE_CHANGED
	MX_DEBUG(-2,("%s: vvvvvvvvvvvvvvvvvvvv",fname));
	MX_DEBUG(-2,("%s invoked for field '%s'", fname, record_field->name));
#endif

	/* Does this record field have a custom value changed test function?
	 *
	 * We always invoke the test function, just in case the test function
	 * has necessary side effects.
	 */

	vc_test_fn = record_field->value_changed_test_function;

	if ( vc_test_fn != NULL ) {
		/* If so, invoke it instead of the standard test. */

		mx_status = (*vc_test_fn)( record_field,
					direction, value_changed_ptr );
	} else
	if ( (record_field->datatype == MXFT_STRING)
	  || (record_field->num_dimensions >= 2) )
	{
		/* FIXME: For strings or multidimensional arrays,
		 * we treat PUT operations as if they _always_
		 * change the value.  This is oversimplistic.
		 */

		if ( direction == MX_PROCESS_PUT ) {
			*value_changed_ptr = TRUE;
		} else {
			*value_changed_ptr = FALSE;
		}

		mx_status = MX_SUCCESSFUL_RESULT;
	} else {
		/* For 0-d and 1-d arrays that are not strings, we invoke
		 * the default value changed test.
		 */

		mx_status = mx_default_test_for_value_changed( record_field,
							direction,
							value_changed_ptr );
	}

	/* If the field has the value_has_changed_manual_override flag set,
	 * then we unconditionally say that the value has changed.
	 */

	if ( record_field->value_has_changed_manual_override ) {
		*value_changed_ptr = TRUE;
	}

	record_field->value_has_changed_manual_override = FALSE;

#if PROCESS_DEBUG_VALUE_CHANGED
	MX_DEBUG(-2,("%s: *value_changed_ptr = %d",
		fname, (int) *value_changed_ptr));

	MX_DEBUG(-2,("%s: ^^^^^^^^^^^^^^^^^^^^",fname));
#endif

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_default_test_for_value_changed( MX_RECORD_FIELD *record_field,
				int direction,
				mx_bool_type *value_changed_ptr )
{
	static const char fname[] = "mx_default_test_for_value_changed()";

	void *array_ptr;
	char *char_array;
	unsigned char *uchar_array;
	short *short_array;
	unsigned short *ushort_array;
	mx_bool_type *bool_array;
	long *long_array;
	unsigned long *ulong_array;
	float *float_array;
	double *double_array;
	int64_t *int64_array;
	uint64_t *uint64_array;

	double sum, new_value, difference, threshold;
	unsigned long i, num_elements;
	mx_bool_type value_changed;

	if ( record_field == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD_FIELD pointer passed was NULL." );
	}
	if ( value_changed_ptr == (mx_bool_type *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The value_changed pointer passed was NULL." );
	}

#if PROCESS_DEBUG_VALUE_CHANGED
	MX_DEBUG(-2,("%s invoked for field '%s'", fname, record_field->name));
	MX_DEBUG(-2,("%s: num_dimensions = %ld",
		fname, record_field->num_dimensions));
#endif

	value_changed = FALSE;
	new_value = record_field->last_value;

	/* FIXME: Only invoke callbacks for fields for 1-d or 0-d arrays. */

	if ( record_field->num_dimensions < 2 ) {

		if ( record_field->num_dimensions == 0 ) {
			num_elements = 1;
		} else {
			num_elements = record_field->dimension[0];
		}

#if PROCESS_DEBUG_VALUE_CHANGED
		MX_DEBUG(-2,("%s: num_elements = %ld", fname, num_elements));
#endif

		array_ptr = mx_get_field_value_pointer(record_field);

		sum = 0.0;

		switch( record_field->datatype ) {
		case MXFT_CHAR:
			char_array = array_ptr;
			
			for ( i = 0; i < num_elements; i++ ) {
				sum += char_array[i];
			}
			break;
		case MXFT_UCHAR:
			uchar_array = array_ptr;
			
			for ( i = 0; i < num_elements; i++ ) {
				sum += uchar_array[i];
			}
			break;
		case MXFT_SHORT:
			short_array = array_ptr;
			
			for ( i = 0; i < num_elements; i++ ) {
				sum += short_array[i];
			}
			break;
		case MXFT_USHORT:
			ushort_array = array_ptr;
			
			for ( i = 0; i < num_elements; i++ ) {
				sum += ushort_array[i];
			}
			break;
		case MXFT_BOOL:
			bool_array = array_ptr;
			
			for ( i = 0; i < num_elements; i++ ) {
				sum += bool_array[i];
			}
			break;
		case MXFT_LONG:
			long_array = array_ptr;
			
			for ( i = 0; i < num_elements; i++ ) {
				sum += long_array[i];
			}
			break;
		case MXFT_ULONG:
		case MXFT_HEX:
			ulong_array = array_ptr;
			
			for ( i = 0; i < num_elements; i++ ) {
				sum += ulong_array[i];
			}
			break;
		case MXFT_FLOAT:
			float_array = array_ptr;
			
			for ( i = 0; i < num_elements; i++ ) {
				sum += float_array[i];
			}
			break;
		case MXFT_DOUBLE:
			double_array = array_ptr;
			
			for ( i = 0; i < num_elements; i++ ) {
				sum += double_array[i];
			}
			break;
		case MXFT_INT64:
			int64_array = array_ptr;
			
			for ( i = 0; i < num_elements; i++ ) {
				sum += int64_array[i];
			}
			break;
		case MXFT_UINT64:
			uint64_array = array_ptr;
			
#if ( defined(_MSC_VER) && (_MSC_VER < 1300) )
			/* For Visual C++ 6.0 SP6 and before */

			for ( i = 0; i < num_elements; i++ ) {
				double double_temp;

				double_temp =
					mx_uint64_to_double( uint64_array[i] );

				sum += double_temp;
			}
#else
			/* For everyone else. */

			for ( i = 0; i < num_elements; i++ ) {
				sum += uint64_array[i];
			}
#endif
			break;
		default:
			break;
		}

		new_value = mx_divide_safely( sum, num_elements );

		difference = fabs( new_value - record_field->last_value );

		threshold = record_field->value_change_threshold;

#if PROCESS_DEBUG_VALUE_CHANGED
		MX_DEBUG(-2,("%s: last_value = %g, new_value = %g",
			fname, record_field->last_value, new_value));
		MX_DEBUG(-2,("%s: difference = %g, threshold = %g",
			fname, difference, threshold));
#endif

		if ( difference > threshold ) {
			value_changed = TRUE;

			/* Only update 'last_value' if we
			 * have tripped the value changed
			 * threshold.
			 */

			record_field->last_value = new_value;
		}
	}

	*value_changed_ptr = value_changed;

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_update_next_allowed_event_time( MX_RECORD *record,
				MX_RECORD_FIELD *record_field )
{

#if PROCESS_DEBUG_QUEUEING
	static const char fname[] = "mx_update_next_allowed_event_time()";
#endif

	MX_EVENT_TIME_MANAGER *event_time_manager;
	MX_CLOCK_TICK event_interval, current_clock_tick;

	/* If there is no record or no event time manager, then there is
	 * nothing we can do here.
	 */

	if ( record == (MX_RECORD *) NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	event_time_manager = record->event_time_manager;

	if ( event_time_manager == (MX_EVENT_TIME_MANAGER *) NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* If there is a record field, but it has the 
	 * MXFF_NO_NEXT_EVENT_TIME_UPDATE flag set, then
	 * we do not update next_allowed_event_time.
	 */

	if ( record_field != (MX_RECORD_FIELD *) NULL ) {
		if ( record_field->flags & MXFF_NO_NEXT_EVENT_TIME_UPDATE )
			return MX_SUCCESSFUL_RESULT;
	}

	/* If the event interval is (0,0) then we do nothing here. */

	event_interval = event_time_manager->event_interval;

	if ( (event_interval.low_order == 0)
	  && (event_interval.high_order == 0) )
	{
		return MX_SUCCESSFUL_RESULT;
	}

	/* If we get here, we update the next_allowed_event_time
	 * structure with the current time.
	 */

	current_clock_tick = mx_current_clock_tick();

	event_time_manager->next_allowed_event_time =
				    mx_add_clock_ticks( current_clock_tick,
							event_interval );
#if PROCESS_DEBUG_QUEUEING
	MX_DEBUG(-2, ("%s: next_allowed_event_time = (%lu,%lu)", fname,
			event_time_manager->next_allowed_event_time.high_order,
			event_time_manager->next_allowed_event_time.low_order));
#endif

	return MX_SUCCESSFUL_RESULT;
}

