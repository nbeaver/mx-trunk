/*
 * Name:    mx_rs232.c
 *
 * Purpose: MX function library of generic RS-232 operations.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2007, 2010-2012, 2014-2017 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_RS232_DEBUG_GETLINE_PUTLINE	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include "mx_util.h"
#include "mx_rs232.h"
#include "mx_driver.h"

/*-------------------------- Internal driver functions ----------------------*/

MX_EXPORT mx_status_type
mx_rs232_unbuffered_getline( MX_RS232 *rs232,
			char *buffer,
			size_t max_bytes_to_read,
			size_t *bytes_read,
			unsigned long local_transfer_flags,
			double timeout_in_seconds )
{
	static const char fname[] = "mx_rs232_unbuffered_getline()";

	char *array;
	mx_bool_type do_timeout;
	int tick_comparison;
	MX_CLOCK_TICK timeout_clock_ticks, current_tick, finish_tick;
	mx_bool_type terminators_seen;
	int i, start_of_terminator;
	char c;
	long error_code;
	unsigned long num_bytes_available;
	mx_status_type mx_status;

	/* Are we doing a timeout? */

	if ( timeout_in_seconds >= 0.0 ) {
		do_timeout = TRUE;
	} else
	if ( rs232->timeout < 0.0 ) {
		do_timeout = FALSE;
	} else {
		do_timeout = TRUE;

		timeout_in_seconds = rs232->timeout;
	}
	
	if ( do_timeout ) {
		timeout_clock_ticks =
		    mx_convert_seconds_to_clock_ticks( timeout_in_seconds );

		current_tick = mx_current_clock_tick();

		finish_tick = mx_add_clock_ticks( current_tick,
						timeout_clock_ticks );
	}

	array = rs232->read_terminator_array;

	terminators_seen = 0;
	start_of_terminator = INT_MAX;

#if MX_RS232_DEBUG_GETLINE_PUTLINE

	MX_DEBUG(-2,("%s: do_timeout = %d, rs232->timeout = %g",
		fname, do_timeout, rs232->timeout));
#endif

	for ( i = 0; i < max_bytes_to_read; i++ ) {

		if ( do_timeout ) {
			current_tick = mx_current_clock_tick();

			tick_comparison = mx_compare_clock_ticks(
					current_tick, finish_tick );

			if ( tick_comparison > 0 ) {

		    	    if ( rs232->rs232_flags &
			      MXF_232_SUPPRESS_TIMEOUT_ERROR_MESSAGES )
			    {
				error_code = 
					(MXE_TIMED_OUT | MXE_QUIET);
			    } else {
				error_code = MXE_TIMED_OUT;
			    }

			    return mx_error( error_code, fname,
				"Read from RS-232 port '%s' timed out "
				"after %g seconds.",
					rs232->record->name, rs232->timeout );
			}

			mx_status = mx_rs232_num_input_bytes_available(
					rs232->record, &num_bytes_available );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
#if MX_RS232_DEBUG_GETLINE_PUTLINE
			MX_DEBUG(-2,("%s: num_bytes_available = %ld",
				fname, num_bytes_available));
#endif

			if ( num_bytes_available < 1 ) {
				/* The serial port says that it 
				 * does not yet have a character
				 * available.
				 *
				 * On the next read attempt, we
				 * want to make sure that the
				 * index variable 'i' has the
				 * same value that it had this
				 * time, so we decrement 'i'
				 * by one and go back to the top
				 * of the for loop.
				 */

				/* Back up */
				i--;

				mx_msleep(1);

				/* Go to the top of the for loop. */
				continue;
			}
		}

		mx_status = mx_rs232_getchar( rs232->record,
						&c, MXF_232_WAIT );

#if MX_RS232_DEBUG_GETLINE_PUTLINE
		MX_DEBUG(-2,("%s: c = %#x '%c'", fname, c, c));
#endif

		if ( mx_status.code != MXE_SUCCESS ) {
                        /* Make the buffer contents a valid C string
			 * before returning.
			 */

			buffer[i] = '\0';

			if ( mx_status.code == MXE_NOT_READY ) {
				/* The serial port says that it 
				 * does not yet have a character
				 * available.
				 *
				 * On the next read attempt, we
				 * want to make sure that the
				 * index variable 'i' has the
				 * same value that it had this
				 * time, so we decrement 'i'
				 * by one and go back to the top
				 * of the for loop.
				 */

				/* Back up */
				i--;

				/* Go to the top of the for loop. */
				continue;
			} else {
				if ( mx_rs232_show_debugging( rs232,
						MXF_232_HIDE_FROM_DEBUG ))
				{
					MX_DEBUG(-2,
					("%s failed.\nbuffer = '%s'",
						fname, buffer));

					MX_DEBUG(-2,
			("Failed with status = %ld, c = 0x%x '%c'",
						mx_status.code, c, c));
				}
				return mx_status;
			}
		}

#if MX_RS232_DEBUG_GETLINE_PUTLINE
		if ( mx_rs232_show_debugging(rs232, 0) ) {
			MX_DEBUG(-2,
			("%s: received c = 0x%x '%c'", fname, c, c));
		}
#endif

		buffer[i] = c;

		/* If we receive a null byte, we have to decide
		 * whether or not we will ignore it.  If we
		 * do _not_ ignore it, then it will have the
		 * effect of terminating the string returned
		 * to the caller at this point.
		 */

		if ( c == '\0' ) {
			if ( (rs232->transfer_flags) & MXF_232_IGNORE_NULLS ) {

				/* Back up the loop one step
				 * and then go back to the top
				 * of the loop.  This will have
				 * the effect of overwriting
				 * the null with the next
				 * character received.
				 */

				i--;

				continue;

			} else {
				/* Do not ignore the null. */

				start_of_terminator = i;

				break;	/* Exit the for() loop. */
			}
		}

		/* Check to see if we are in the middle of a 
		 * line terminator sequence.
		 */

		if ( c != array[ terminators_seen ] ) {

			terminators_seen = 0;
			start_of_terminator = INT_MAX;
		} else {
			if ( terminators_seen == 0 ) {
				start_of_terminator = i;
			}
			terminators_seen++;

			/* Only allow up to four terminator characters.
			 */

			if ( (terminators_seen
					>= MX_RS232_MAX_TERMINATORS)
			  || (array[ terminators_seen ] == '\0') ) {

				/* Since we have now seen the entire
				 * terminator sequence, we know that
				 * we are at the end of the line.
				 * Thus, we break out of the loop here.
				 */

				break;	/* Exit the for() loop. */
			}
		}
	}

	if ( start_of_terminator < i ) {
		buffer[ start_of_terminator ] = 0;
	} else {
		buffer[ i ] = 0;
	}

	if ( bytes_read != NULL ) {
		*bytes_read = strlen( buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------*/

MX_EXPORT mx_status_type
mx_rs232_unbuffered_putline( MX_RS232 *rs232,
				char *buffer,
				size_t *bytes_written,
				unsigned long local_transfer_flags )
{
	static const char fname[] = "mx_rs232_unbuffered_putline()";

	char *array;
	size_t i, length;
	char c;
	mx_status_type mx_status;

	array = rs232->write_terminator_array;

	length = strlen( buffer );

	for ( i = 0; i < length; i++ ) {
		c = buffer[i];

#if MX_RS232_DEBUG_GETLINE_PUTLINE
		MX_DEBUG(-2,("%s: sending '%c'", fname, c));
#endif
		mx_status = mx_rs232_putchar( rs232->record, c, MXF_232_WAIT );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Send the terminator characters. */

	for ( i = 0; i < MX_RS232_MAX_TERMINATORS; i++ ) {

		c = array[i];

		if ( c == '\0' ) {
			/* Have reached the end of the terminators,
			 * so exit the loop.
			 */

			break;
		}

		if ( mx_rs232_show_debugging( rs232, MXF_232_HIDE_FROM_DEBUG ) )
		{
			MX_DEBUG(-2,
			("%s: sending write terminator 0x%x", fname, c));
		}

		mx_status = mx_rs232_putchar( rs232->record, c, MXF_232_WAIT );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*----------------------------- Public functions ----------------------------*/

MX_EXPORT mx_bool_type
mx_rs232_show_debugging( MX_RS232 *rs232,
			unsigned long transfer_flags )
{
	static const char fname[] = "mx_rs232_show_debugging()";

	if ( rs232 == (MX_RS232 *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RS232 pointer passed to this function was NULL." );

		return FALSE;
	}

	if ( transfer_flags & MXF_232_DEBUG ) {
		return TRUE;
	} else
	if ( rs232->rs232_flags & MXF_232_DEBUG_SERIAL_HEX ) {
		return TRUE;
	} else
	if ( rs232->rs232_flags & MXF_232_DEBUG_SERIAL ) {
		if ( transfer_flags & MXF_232_HIDE_FROM_DEBUG ) {
			return FALSE;
		} else {
			return TRUE;
		}
	} else {
		return FALSE;
	}
}

MX_EXPORT mx_status_type
mx_rs232_get_serial_debug( MX_RECORD *rs232_record,
				unsigned long *debug_flags )
{
	static const char fname[] = "mx_rs232_get_serial_debug()";

	MX_RS232 *rs232;
	unsigned long rs232_flags;

	if ( rs232_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RS232 pointer passed was NULL." );
	}
	if ( debug_flags == (unsigned long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The debug_flags pointer passed was NULL." );
	}

	rs232 = (MX_RS232 *) rs232_record->record_class_struct;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RS232 pointer for record '%s' is NULL.",
			rs232_record->name );
	}

	rs232_flags = rs232->rs232_flags;

	rs232_flags >>= 24;

	rs232_flags &= 0x3;

	*debug_flags = rs232_flags;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_rs232_set_serial_debug( MX_RECORD *rs232_record,
				unsigned long debug_flags )
{
	static const char fname[] = "mx_rs232_get_serial_debug()";

	MX_RS232 *rs232;
	unsigned long mask;

	if ( rs232_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RS232 pointer passed was NULL." );
	}

	rs232 = (MX_RS232 *) rs232_record->record_class_struct;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RS232 pointer for record '%s' is NULL.",
			rs232_record->name );
	}

	debug_flags &= 0x3;

	debug_flags <<= 24;

	mask = MXF_232_DEBUG_SERIAL | MXF_232_DEBUG_SERIAL_HEX;

	rs232->rs232_flags &= (~mask);

	rs232->rs232_flags |= debug_flags;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_rs232_check_port_parameters( MX_RECORD *rs232_record )
{
	static const char fname[] = "mx_rs232_check_port_parameters()";

	MX_RS232 *rs232;
	mx_status_type mx_status;

	mx_status = mx_rs232_get_pointers( rs232_record, &rs232, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Check for allowed serial port speeds. */

	switch( rs232->speed ) {
	case 300:
	case 600:
	case 1200:
	case 2400:
	case 4800:
	case 9600:
	case 19200:
	case 38400:
	case 57600:
	case 115200:
		break;
	default:
		mx_status = mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Invalid serial port speed = %ld", rs232->speed );

		rs232->speed = -1;

		return mx_status;
	}

	/* Check for allowed word sizes. */

	switch( rs232->word_size ) {
	case 7:
	case 8:
		break;
	default:
		mx_status = mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		    "Invalid serial port word size = %ld", rs232->word_size );

		rs232->word_size = -1;

		return mx_status;
	}

	/* Check for allowed parity. */

	switch( rs232->parity ) {
	case 'n':
	case 'N':
		rs232->parity = MXF_232_NO_PARITY;
		break;
	case 'e':
	case 'E':
		rs232->parity = MXF_232_EVEN_PARITY;
		break;
	case 'o':
	case 'O':
		rs232->parity = MXF_232_ODD_PARITY;
		break;
	case 'm':
	case 'M':
		rs232->parity = MXF_232_MARK_PARITY;
		break;
	case 's':
	case 'S':
		rs232->parity = MXF_232_SPACE_PARITY;
		break;
	default:
		mx_status = mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Invalid serial port parity = '%c'", rs232->parity );

		rs232->parity = -1;

		return mx_status;
	}

	/* Check for allowed stop bits. */

	switch( rs232->stop_bits ) {
	case 1:
	case 2:
		break;
	default:
		mx_status = mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		    "Invalid serial port stop bits = %ld", rs232->stop_bits );

		rs232->stop_bits = -1;

		return mx_status;
	}

	/* Check for allowed flow control. */

	switch( rs232->flow_control ) {
	case 'n':
	case 'N':
		rs232->flow_control = MXF_232_NO_FLOW_CONTROL;
		break;

	case 'h':
	case 'H':
		rs232->flow_control = MXF_232_HARDWARE_FLOW_CONTROL;
		break;

	case 's':
	case 'S':
		rs232->flow_control = MXF_232_SOFTWARE_FLOW_CONTROL;
		break;

	case 'b':
	case 'B':
		rs232->flow_control = MXF_232_BOTH_FLOW_CONTROL;
		break;

	case 'r':
	case 'R':
		rs232->flow_control = MXF_232_RTS_CTS_FLOW_CONTROL;
		break;

	case 'd':
	case 'D':
		rs232->flow_control = MXF_232_DTR_DSR_FLOW_CONTROL;
		break;

	default:
		mx_status = mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Invalid serial port flow control = '%c'",
			rs232->flow_control );

		rs232->flow_control = -1;

		return mx_status;
	}

	mx_status = mx_rs232_convert_terminator_characters( rs232_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_rs232_convert_terminator_characters( MX_RECORD *rs232_record )
{
	static const char fname[] = "mx_rs232_convert_terminator_characters()";

	MX_RS232 *rs232;
	int i, j, shift;
	unsigned char terminator_value;
	mx_status_type mx_status;

	mx_status = mx_rs232_get_pointers( rs232_record, &rs232, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Save the value of the line terminators. */

	for ( i = 0, j = 0; i < MX_RS232_MAX_TERMINATORS; i++ ) {

		rs232->read_terminator_array[i] = '\0';

		shift = 8 * ( MX_RS232_MAX_TERMINATORS - i - 1 );

		terminator_value = (unsigned char)
				( ( rs232->read_terminators ) >> shift );

		if ( ( j > 0 ) || ( terminator_value != '\0' ) ) {

			rs232->read_terminator_array[j] = terminator_value;

			j++;
		}
	}

	rs232->num_read_terminator_chars = j;

	rs232->read_terminator_array[MX_RS232_MAX_TERMINATORS] = '\0';

	MX_DEBUG( 2,("%s: rs232->num_read_terminator_chars = %ld",
			fname, rs232->num_read_terminator_chars));

#if 0
	MX_DEBUG(-2,("%s: read terminators = 0x%08lx",
			fname, rs232->read_terminators));

	for ( i = 0; i < 4; i++ ) {
		MX_DEBUG(-2,("%s: read_terminator_array[%d] = %x",
			fname, i, rs232->read_terminator_array[i] ));
	}
#endif

	for ( i = 0, j = 0; i < MX_RS232_MAX_TERMINATORS; i++ ) {

		rs232->write_terminator_array[i] = '\0';

		shift = 8 * ( MX_RS232_MAX_TERMINATORS - i - 1 );

		terminator_value = (unsigned char)
				( ( rs232->write_terminators ) >> shift );

		if ( ( j > 0 ) || ( terminator_value != '\0' ) ) {

			rs232->write_terminator_array[j] = terminator_value;

			j++;
		}
	}

	rs232->num_write_terminator_chars = j;

	rs232->write_terminator_array[MX_RS232_MAX_TERMINATORS] = '\0';

	MX_DEBUG( 2,("%s: rs232->num_write_terminator_chars = %ld",
			fname, rs232->num_write_terminator_chars));

#if 0
	MX_DEBUG(-2,("%s: write terminators = 0x%08lx",
			fname, rs232->write_terminators));

	for ( i = 0; i < 4; i++ ) {
		MX_DEBUG(-2,("%s: write_terminator_array[%d] = %x",
			fname, i, rs232->write_terminator_array[i] ));
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT char *
mx_rs232_find_terminators( char *buffer,
			size_t buffer_length,
			unsigned long num_terminators,
			char *terminator_array )
{
	char *buffer_ptr;
	size_t buffer_left;
	mx_bool_type terminators_found;
	unsigned long i;

	if ( num_terminators < 1 ) {
		return NULL;
	}

	buffer_ptr = buffer;
	buffer_left = buffer_length;

	while (1) {
		if ( buffer_left < num_terminators ) {
			return NULL;
		}

		if ( buffer_ptr[0] == '\0' ) {
			return NULL;
		}

		if ( buffer_ptr[0] == terminator_array[0] ) {
			terminators_found = TRUE;

			for ( i = 1; i < num_terminators; i++ ) {
				if ( buffer_ptr[i] != terminator_array[i] ) {
					terminators_found = FALSE;
					break;	/* Exit the for() loop. */
				}
			}

			if ( terminators_found ) {
				return buffer_ptr;
			}
		}

		buffer_ptr++;
		buffer_left--;
	}

#if !defined(OS_SOLARIS)
	return NULL;
#endif
}

MX_EXPORT mx_status_type
mx_rs232_get_pointers( MX_RECORD *rs232_record,
			MX_RS232 **rs232,
			MX_RS232_FUNCTION_LIST **function_list_ptr,
			const char *calling_fname )
{
	static const char fname[] = "mx_rs232_get_pointers()";

	if ( rs232_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The rs232_record pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( rs232_record->mx_class != MXI_RS232 ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The record '%s' passed by '%s' is not a RS232 interface.  "
	"(superclass = %ld, class = %ld, type = %ld)",
			rs232_record->name, calling_fname,
			rs232_record->mx_superclass,
			rs232_record->mx_class,
			rs232_record->mx_type );
	}

	if ( rs232 != (MX_RS232 **) NULL ) {
		*rs232 = (MX_RS232 *) (rs232_record->record_class_struct);

		if ( *rs232 == (MX_RS232 *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RS232 pointer for record '%s' passed by '%s' is NULL.",
				rs232_record->name, calling_fname );
		}
	}

	if ( function_list_ptr != (MX_RS232_FUNCTION_LIST **) NULL ) {
		*function_list_ptr = (MX_RS232_FUNCTION_LIST *)
				(rs232_record->class_specific_function_list);

		if ( *function_list_ptr == (MX_RS232_FUNCTION_LIST *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RS232_FUNCTION_LIST pointer for record '%s' passed by '%s' is NULL.",
				rs232_record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_rs232_getchar( MX_RECORD *record,
			char *c,
			unsigned long transfer_flags )
{
	static const char fname[] = "mx_rs232_getchar()";

	MX_RS232 *rs232;
	MX_RS232_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_RS232 *, char * );
	mx_status_type mx_status;

	mx_status = mx_rs232_get_pointers( record, &rs232, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = ( fl_ptr->getchar );

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Single character I/O is not supported for record '%s'.",
			record->name );
	}

	/* Invoke the function. */

	rs232->transfer_flags = transfer_flags;

	mx_status = (*fptr)( rs232, c );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_rs232_putchar( MX_RECORD *record,
			char c,
			unsigned long transfer_flags )
{
	static const char fname[] = "mx_rs232_putchar()";

	MX_RS232 *rs232;
	MX_RS232_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_RS232 *, char );
	mx_status_type mx_status;

	mx_status = mx_rs232_get_pointers( record, &rs232, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = ( fl_ptr->putchar );

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Single character I/O is not supported for record '%s'.",
			record->name );
	}


	rs232->transfer_flags = transfer_flags;

	/* Invoke the function. */

	mx_status = (*fptr)( rs232, c );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If requested, flush already written data to the destination. */

	if ( rs232->rs232_flags & MXF_232_FLUSH_AFTER_PUTCHAR ) {
		mx_status = mx_rs232_flush( record );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_rs232_getchar_with_timeout( MX_RECORD *record,
				char *c,
				unsigned long transfer_flags,
				double timeout_in_seconds )
{
	static const char fname[] = "mx_rs232_getchar_with_timeout()";

	MX_RS232 *rs232;
	MX_CLOCK_TICK start_tick, finish_tick, current_tick;
	MX_CLOCK_TICK timeout_clock_ticks;
	long error_code;
	mx_status_type mx_status;

	mx_status = mx_rs232_get_pointers( record, &rs232, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If the timeout is infinite, then use the normal getchar function. */

	if ( timeout_in_seconds < 0.0 ) {
		mx_status = mx_rs232_getchar( record, c, transfer_flags );

		return mx_status;
	}

	/* Add MXF_232_NOWAIT to the transfer flags so that we can directly
	 * control the timeout.
	 */

	transfer_flags |= MXF_232_NOWAIT;

	timeout_clock_ticks =
		mx_convert_seconds_to_clock_ticks( timeout_in_seconds );

	start_tick = mx_current_clock_tick();

	finish_tick = mx_add_clock_ticks( start_tick, timeout_clock_ticks );

	current_tick = mx_current_clock_tick();

	while( mx_compare_clock_ticks( current_tick, finish_tick ) < 0 ) {

		mx_status = mx_rs232_getchar( record, c, transfer_flags );

		if ( mx_status.code == MXE_SUCCESS ) {
			/* The character has been successfully read, so
			 * return now.
			 */

			return MX_SUCCESSFUL_RESULT;
		}

		if ( mx_status.code != MXE_NOT_READY )
			return mx_status;

		/* The serial port said that there were no characters ready
		 * so sleep for a moment and then try again.
		 */

		mx_msleep(1);

		current_tick = mx_current_clock_tick();
	}

	if ( rs232->rs232_flags & MXF_232_SUPPRESS_TIMEOUT_ERROR_MESSAGES ) {
		error_code = (MXE_TIMED_OUT | MXE_QUIET);
	} else
	if ( transfer_flags & MXF_232_SUPPRESS_TIMEOUT_ERROR_MESSAGES ) {
		error_code = (MXE_TIMED_OUT | MXE_QUIET);
	} else {
		error_code = MXE_TIMED_OUT;
	}

	return mx_error( error_code, fname,
	"Read from RS-232 port '%s' timed out after %g seconds.",
			record->name, timeout_in_seconds );
}
		
MX_EXPORT mx_status_type
mx_rs232_read( MX_RECORD *record,
		char *buffer,
		size_t max_bytes_to_read,
		size_t *bytes_read,
		unsigned long transfer_flags )
{
	static const char fname[] = "mx_rs232_read()";

	MX_RS232 *rs232;
	MX_RS232_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_RS232 *, char *, size_t, size_t * );
	size_t i, bytes_read_by_driver;
	char c;
	int buffered_io;
	mx_status_type mx_status;

	mx_status = mx_rs232_get_pointers( record, &rs232, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = ( fl_ptr->read );

	if ( rs232->rs232_flags & MXF_232_UNBUFFERED_IO ) {
		buffered_io = FALSE;
	} else {
		buffered_io = TRUE;
	}

	/* Invoke the function. */

	rs232->transfer_flags = transfer_flags;

	if ( buffered_io && ( fptr != NULL ) ) {
		mx_status = (*fptr)( rs232, buffer,
			max_bytes_to_read, &bytes_read_by_driver );
	} else {
		for ( i = 0; i < max_bytes_to_read; i++ ) {
			mx_status = mx_rs232_getchar(record, &c, MXF_232_WAIT);

			if ( mx_status.code != MXE_SUCCESS )
				break;		/* Exit the for() loop. */

			buffer[i] = c;
		}

		if ( i >= max_bytes_to_read ) {
			bytes_read_by_driver = max_bytes_to_read;
		} else {
			bytes_read_by_driver = i;
		}
	}

	/* Null terminate the buffer so that it is a valid C string. */

	buffer[bytes_read_by_driver] = '\0';

	if ( mx_rs232_show_debugging( rs232, transfer_flags ) ) {
		MX_DEBUG(-2,("%s: received '%s' from '%s'",
				fname, buffer, record->name));
	}

	if ( bytes_read != NULL ) {
		*bytes_read = bytes_read_by_driver;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_rs232_write( MX_RECORD *record,
		char *buffer,
		size_t bytes_to_write,
		size_t *bytes_written,
		unsigned long transfer_flags )
{
	static const char fname[] = "mx_rs232_write()";

	MX_RS232 *rs232;
	MX_RS232_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_RS232 *, char *, size_t, size_t * );
	size_t i, bytes_written_by_driver;
	int buffered_io;
	mx_status_type mx_status;

	mx_status = mx_rs232_get_pointers( record, &rs232, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = fl_ptr->write;

	if ( mx_rs232_show_debugging( rs232, transfer_flags ) ) {
		MX_DEBUG(-2,
		("%s: sending '%s' to '%s'", fname, buffer, record->name));
	}

	if ( rs232->rs232_flags & MXF_232_UNBUFFERED_IO ) {
		buffered_io = FALSE;
	} else {
		buffered_io = TRUE;
	}

	/* Invoke the function. */

	rs232->transfer_flags = transfer_flags;

	if ( buffered_io && ( fptr != NULL ) ) {
		mx_status = (*fptr)( rs232, buffer,
				bytes_to_write, &bytes_written_by_driver );
	} else {
		for ( i = 0; i < bytes_to_write; i++ ) {
			mx_status = mx_rs232_putchar( record,
						buffer[i], MXF_232_WAIT );

			if ( mx_status.code != MXE_SUCCESS )
				break;		/* Exit the for() loop. */
		}

		if ( i >= bytes_to_write ) {
			bytes_written_by_driver = bytes_to_write;
		} else {
			bytes_written_by_driver = i;
		}
	}

	if ( bytes_written != NULL ) {
		*bytes_written = bytes_written_by_driver;
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If requested, flush already written data to the destination. */

	if ( rs232->rs232_flags & MXF_232_FLUSH_AFTER_WRITE ) {
		mx_status = mx_rs232_flush( record );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_rs232_read_with_timeout( MX_RECORD *record,
		char *buffer,
		size_t max_bytes_to_read,
		size_t *bytes_read,
		unsigned long transfer_flags,
		double timeout_in_seconds )
{
	static const char fname[] = "mx_rs232_read_with_timeout()";

	MX_RS232 *rs232;
	MX_RS232_FUNCTION_LIST *fl_ptr;
	size_t i, bytes_read_by_driver;
	char c;
	int buffered_io;
	mx_status_type mx_status;

	mx_status = mx_rs232_get_pointers( record, &rs232, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( rs232->rs232_flags & MXF_232_UNBUFFERED_IO ) {
		buffered_io = FALSE;
	} else {
		buffered_io = TRUE;
	}

	MXW_UNUSED( buffered_io );

	/* Loop over the requested buffer length with a timeout. */

	rs232->transfer_flags = transfer_flags;

	for ( i = 0; i < max_bytes_to_read; i++ ) {
		mx_status = mx_rs232_getchar_with_timeout( record, &c,
							transfer_flags,
							timeout_in_seconds );

		if ( mx_status.code != MXE_SUCCESS )
			break;		/* Exit the for() loop. */

		buffer[i] = c;
	}

	if ( i >= max_bytes_to_read ) {
		bytes_read_by_driver = max_bytes_to_read;
	} else {
		bytes_read_by_driver = i;
	}

	/* Null terminate the buffer so that it is a valid C string. */

	buffer[bytes_read_by_driver] = '\0';

	if ( mx_rs232_show_debugging( rs232, transfer_flags ) ) {
		MX_DEBUG(-2,("%s: received '%s' from '%s'",
				fname, buffer, record->name));
	}

	if ( bytes_read != NULL ) {
		*bytes_read = bytes_read_by_driver;
	}

	return mx_status;
}

/* mx_rs232_getline() and mx_rs232_putline() are intended for reading
 * text strings.  Use mx_rs232_read() and mx_rs232_write() for binary data.
 */

MX_EXPORT mx_status_type
mx_rs232_getline( MX_RECORD *record,
			char *buffer,
			size_t max_bytes_to_read,
			size_t *bytes_read,
			unsigned long transfer_flags )
{
	static const char fname[] = "mx_rs232_getline()";

	MX_RS232 *rs232;
	MX_RS232_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_RS232 *, char *, size_t, size_t * );
	unsigned long local_bytes_read;
	mx_bool_type buffered_io;
	mx_status_type mx_status;

	mx_status = mx_rs232_get_pointers( record, &rs232, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	rs232->rs232_flags = rs232->rs232_flags;

	if ( rs232->rs232_flags & MXF_232_UNBUFFERED_IO ) {
		buffered_io = FALSE;
	} else {
		buffered_io = TRUE;
	}

	/* If requested, force mx_rs232_getline() to always ignore nulls,
	 * even if this particular invocation did not set the necessary flag.
	 */

	if ( rs232->rs232_flags & MXF_232_ALWAYS_IGNORE_NULLS ) {
		transfer_flags |= MXF_232_IGNORE_NULLS;
	}

	rs232->transfer_flags = transfer_flags;

	/* Does this driver have a getline function? */

	fptr = fl_ptr->getline;

	if ( buffered_io && ( fptr != NULL ) ) {

		/* If so, invoke it. */

		mx_status =
			(*fptr)( rs232, buffer,
				max_bytes_to_read,
				&local_bytes_read );

	} else {
		/* Otherwise, handle input a character at a time. */

		mx_status = mx_rs232_unbuffered_getline(
						rs232,
						buffer,
						max_bytes_to_read,
						&local_bytes_read,
						MXF_232_HIDE_FROM_DEBUG,
						-1.0 );
	}

	if ( mx_rs232_show_debugging( rs232, transfer_flags ) ) {
	    if ( rs232->rs232_flags & MXF_232_DEBUG_SERIAL_HEX ) {
		unsigned long i, max_values_to_show;

		max_values_to_show = 10;

		if ( rs232->rs232_flags & MXF_232_DEBUG_SERIAL ) {
		    fprintf( stderr, "%s: received '%s' ", fname, buffer );
		}

		if ( 0 == (int) buffer[0] ) {
		    fprintf( stderr, "0x0 " );
		} else {
		    fprintf( stderr, "%#x ", (int) buffer[0] );
		}

		for ( i = 1; i < max_values_to_show; i++ ) {
		    if ( buffer[i] == '\0' ) {
			break;
		    }
		    fprintf( stderr, "%#x ", (int) buffer[i] );
		}

		if ( i >= max_values_to_show ) {
		    fprintf( stderr, "... " );
		}

		fprintf( stderr, "from '%s'\n", record->name );
	    } else {
		MX_DEBUG(-2,("%s: received '%s' from '%s'",
				fname, buffer, record->name));
	    }
	}

	if ( bytes_read != NULL ) {
		*bytes_read = local_bytes_read;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_rs232_putline( MX_RECORD *record,
		char *buffer,
		size_t *bytes_written,
		unsigned long transfer_flags )
{
	static const char fname[] = "mx_rs232_putline()";

	MX_RS232 *rs232;
	MX_RS232_FUNCTION_LIST *fl_ptr;
	mx_status_type (*putline_fn)( MX_RS232 *, char *, size_t * );
	mx_status_type (*write_fn)( MX_RS232 *, char *, size_t, size_t * );
	size_t length;
	mx_bool_type buffered_io;
	mx_status_type mx_status;

	mx_status = mx_rs232_get_pointers( record, &rs232, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mx_rs232_show_debugging( rs232, transfer_flags ) ) {
		MX_DEBUG(-2, ("%s: sending '%s' to '%s'",
			fname, buffer, record->name));
	}

	if ( rs232->rs232_flags & MXF_232_UNBUFFERED_IO ) {
		buffered_io = FALSE;
	} else {
		buffered_io = TRUE;
	}

	rs232->transfer_flags = transfer_flags;

	/* Does this driver have a putline or a write function? */

	putline_fn = fl_ptr->putline;
	write_fn   = fl_ptr->write;

	if ( buffered_io && ( putline_fn != NULL ) ) {

		/* If it has putline, invoke it. */

		mx_status = (*putline_fn)( rs232, buffer, bytes_written );

	} else
	if ( buffered_io && ( write_fn != NULL ) ) {

		size_t written_1, written_2;

		/* If it has write, use that to send the buffer. */

		length = strlen( buffer );

		mx_status = (*write_fn)( rs232, buffer, length, &written_1 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* If the port has write terminator characters,
		 * send those as well.
		 */

		written_2 = 0;

		if ( rs232->num_write_terminator_chars > 0 ) {

			mx_status = (*write_fn)( rs232,
					rs232->write_terminator_array,
					rs232->num_write_terminator_chars,
					&written_2 );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		if ( bytes_written != NULL ) {
			*bytes_written = written_1 + written_2;
		}

	} else {
		/* Otherwise, handle output a character at a time. */

		mx_status = mx_rs232_unbuffered_putline(
						rs232,
						buffer,
						bytes_written,
						MXF_232_HIDE_FROM_DEBUG );
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If requested, flush already written data to the destination. */

#if 0
	MX_DEBUG(-2,("%s: '%s' rs232->rs232_flags = %#lx",
		fname, rs232->record->name, rs232->rs232_flags));
#endif

	if ( rs232->rs232_flags & MXF_232_FLUSH_AFTER_PUTLINE ) {
		mx_status = mx_rs232_flush( record );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_rs232_getline_with_timeout( MX_RECORD *record,
		char *buffer,
		size_t max_bytes_to_read,
		size_t *bytes_read,
		unsigned long transfer_flags,
		double timeout_in_seconds )
{
	static const char fname[] = "mx_rs232_getline_with_timeout()";

	MX_RS232 *rs232 = NULL;
	mx_status_type mx_status;

	mx_status = mx_rs232_get_pointers( record, &rs232, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_unbuffered_getline( rs232,
				buffer, max_bytes_to_read, bytes_read,
				transfer_flags, timeout_in_seconds );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_rs232_num_input_bytes_available( MX_RECORD *record,
				unsigned long *num_input_bytes_available )
{
	static const char fname[] = "mx_rs232_num_input_bytes_available()";

	MX_RS232 *rs232;
	MX_RS232_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_RS232 * );
	mx_status_type mx_status;

	mx_status = mx_rs232_get_pointers( record, &rs232, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = fl_ptr->num_input_bytes_available;

	if ( fptr == NULL ) {
		rs232->num_input_bytes_available = 1;
	} else {
		mx_status = (*fptr)( rs232 );
	}

	if ( num_input_bytes_available != NULL ) {
		*num_input_bytes_available = rs232->num_input_bytes_available;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_rs232_wait_for_input_available( MX_RECORD *record,
				unsigned long *num_input_bytes_available,
				double wait_timeout_in_seconds )
{
	static const char fname[] = "mx_rs232_wait_for_input_available()";

	MX_RS232 *rs232;
	MX_RS232_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_RS232 *, double );
	mx_status_type mx_status;

	MX_CLOCK_TICK current_tick, finish_tick, timeout_in_ticks;
	int comparison;
	mx_bool_type wait_forever;
	unsigned long num_bytes_available;

	mx_status = mx_rs232_get_pointers( record, &rs232, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = fl_ptr->wait_for_input_available;

	if ( fptr != NULL ) {
		mx_status = (*fptr)( rs232, wait_timeout_in_seconds );
	} else {
		if ( wait_timeout_in_seconds < 0.0 ) {
			wait_forever = TRUE;
		} else {
			wait_forever = FALSE;

			timeout_in_ticks = mx_convert_seconds_to_clock_ticks(
						wait_timeout_in_seconds );

			current_tick = mx_current_clock_tick();

			finish_tick = mx_add_clock_ticks( current_tick,
							timeout_in_ticks );
		}

		while (1) {
			mx_status = mx_rs232_num_input_bytes_available(
						record, &num_bytes_available );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( num_bytes_available > 0 ) {
				break;	/* Exit the while() loop. */
			}

			if ( wait_forever == FALSE ) {
				comparison = mx_compare_clock_ticks(
							current_tick,
							finish_tick );

				if ( comparison >= 0 ) {
					return mx_error( MXE_TIMED_OUT, fname,
					"Timed out after waiting %f seconds "
					"for available input from '%s'.",
						wait_timeout_in_seconds,
						record->name );
				}
			}
		}
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_rs232_discard_unread_input( MX_RECORD *record,
				unsigned long transfer_flags )
{
	static const char fname[] = "mx_rs232_discard_unread_input()";

	MX_RS232 *rs232;
	MX_RS232_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_RS232 * );
	mx_status_type mx_status;

	mx_status = mx_rs232_get_pointers( record, &rs232, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	rs232->transfer_flags = transfer_flags;

	fptr = fl_ptr->discard_unread_input;

	if ( fptr != NULL ) {
		mx_status = (*fptr)( rs232 );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_rs232_discard_unwritten_output( MX_RECORD *record,
				unsigned long transfer_flags )
{
	static const char fname[] = "mx_rs232_discard_unwritten_output()";

	MX_RS232 *rs232;
	MX_RS232_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_RS232 * );
	mx_status_type mx_status;

	mx_status = mx_rs232_get_pointers( record, &rs232, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	rs232->transfer_flags = transfer_flags;

	fptr = fl_ptr->discard_unwritten_output;

	if ( fptr != NULL ) {
		mx_status = (*fptr)( rs232 );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_rs232_discard_until_string( MX_RECORD *record,
				char *string_to_look_for,
				mx_bool_type discard_read_terminators,
				unsigned long transfer_flags,
				double timeout )
{
	static const char fname[] = "mx_rs232_discard_until_string()";

	MX_RS232 *rs232 = NULL;
	size_t i, string_length;
	char c;
	mx_status_type mx_status;

	mx_status = mx_rs232_get_pointers( record, &rs232, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If string_to_look_for is NULL, then we immediately return,
	 * since there is no string to look for.
	 */

	if ( string_to_look_for == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	string_length = strlen( string_to_look_for );

	i = 0;

	while( TRUE ) {
		if ( i >= string_length ) {
			/* We have seen the complete string_to_look_for. */
			break;
		}

		mx_status = mx_rs232_getchar_with_timeout( record,
						&c, transfer_flags, timeout );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( c == string_to_look_for[i] ) {
			i++;
		} else {
			i = 0;
		}
	}

	/* If discard_read_terminators is set, then string_to_look_for
	 * is expected to be followed by RS232 read terminators, which
	 * are to be discarded as well.
	 */

	if ( discard_read_terminators ) {
		for ( i = 0; i < rs232->num_read_terminator_chars; i++ ) {

			mx_status = mx_rs232_getchar_with_timeout( record,
						&c, transfer_flags, timeout );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( c != rs232->read_terminator_array[i] ) {
				return mx_error( MXE_INTERFACE_IO_ERROR, fname,
				"The string to look for '%s' was not "
				"followed by the expected read terminators "
				"for RS232 interface '%s'.  Instead, we saw "
				"the character '%c' (%#x).",
					string_to_look_for,
					record->name, c, c );
			}
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_rs232_get_signal_state( MX_RECORD *record,
			unsigned long *signal_state )
{
	static const char fname[] = "mx_rs232_get_signal_state()";

	MX_RS232 *rs232;
	MX_RS232_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_RS232 * );
	mx_status_type mx_status;

	mx_status = mx_rs232_get_pointers( record, &rs232, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	rs232->old_signal_state = rs232->signal_state;

	fptr = fl_ptr->get_signal_state;

	if ( fptr == NULL ) {
		rs232->signal_state = 0;

		return mx_error( MXE_UNSUPPORTED, fname,
	"The '%s' driver for RS-232 port '%s' does not support reading "
	"the state of the RS-232 signal lines.",
			mx_get_driver_name( record ), record->name );
	}

	mx_status = (*fptr)( rs232 );

	if ( signal_state != NULL ) {
		*signal_state = rs232->signal_state;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_rs232_set_signal_state( MX_RECORD *record,
			unsigned long signal_state )
{
	static const char fname[] = "mx_rs232_set_signal_state()";

	MX_RS232 *rs232;
	MX_RS232_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_RS232 * );
	mx_status_type mx_status;

	mx_status = mx_rs232_get_pointers( record, &rs232, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	rs232->old_signal_state = rs232->signal_state;

	fptr = fl_ptr->set_signal_state;

	if ( fptr == NULL ) {
		rs232->signal_state = 0;

		return mx_error( MXE_UNSUPPORTED, fname,
	"The '%s' driver for RS-232 port '%s' does not support changing "
	"the state of the RS-232 signal lines.",
			mx_get_driver_name( record ), record->name );
	}

	rs232->signal_state = signal_state;

	mx_status = (*fptr)( rs232 );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_rs232_print_signal_state( MX_RECORD *record )
{
	static const char fname[] = "mx_rs232_print_signal_state()";

	MX_RS232 *rs232;
	unsigned long signal_state;
	MX_RS232_FUNCTION_LIST *fl_ptr;
	int request_to_send, clear_to_send;
	int data_set_ready, data_terminal_ready;
	int data_carrier_detect, ring_indicator;
	mx_status_type (*fptr)( MX_RS232 * );
	mx_status_type mx_status;

	mx_status = mx_rs232_get_pointers( record, &rs232, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = fl_ptr->get_signal_state;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
	"The '%s' driver for RS-232 port '%s' does not support reading "
	"the state of the RS-232 signal lines.",
			mx_get_driver_name( record ), record->name );
	}

	mx_status = (*fptr)( rs232 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	signal_state = rs232->signal_state;

	if ( signal_state & MXF_232_REQUEST_TO_SEND ) {
		request_to_send = 1;
	} else {
		request_to_send = 0;
	}

	if ( signal_state & MXF_232_CLEAR_TO_SEND ) {
		clear_to_send = 1;
	} else {
		clear_to_send = 0;
	}

	if ( signal_state & MXF_232_DATA_TERMINAL_READY ) {
		data_terminal_ready = 1;
	} else {
		data_terminal_ready = 0;
	}

	if ( signal_state & MXF_232_DATA_SET_READY ) {
		data_set_ready = 1;
	} else {
		data_set_ready = 0;
	}

	if ( signal_state & MXF_232_DATA_CARRIER_DETECT ) {
		data_carrier_detect = 1;
	} else {
		data_carrier_detect = 0;
	}

	if ( signal_state & MXF_232_RING_INDICATOR ) {
		ring_indicator = 1;
	} else {
		ring_indicator = 0;
	}

	mx_info(
	"Port '%s' status = %#lx : RI=%d DCD=%d DSR=%d DTR=%d CTS=%d RTS=%d",
		record->name, signal_state,
		ring_indicator, data_carrier_detect,
		data_set_ready, data_terminal_ready,
		clear_to_send, request_to_send
		);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_rs232_get_signal_bit( MX_RECORD *record, long bit_type, long *bit_value )
{
	static const char fname[] = "mx_rs232_get_signal_bit()";

	MX_RS232 *rs232;
	unsigned long signal_state;
	mx_status_type mx_status;

	mx_status = mx_rs232_get_pointers( record, &rs232, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( bit_value == (long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The bit_value pointer passed was NULL." );
	}

	mx_status = mx_rs232_get_signal_state( record, &signal_state );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( signal_state & bit_type ) {
		*bit_value = 1;
	} else {
		*bit_value = 0;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_rs232_set_signal_bit( MX_RECORD *record, long bit_type, long bit_value )
{
	static const char fname[] = "mx_rs232_get_signal_bit()";

	MX_RS232 *rs232;
	unsigned long signal_state;
	mx_status_type mx_status;

	mx_status = mx_rs232_get_pointers( record, &rs232, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_get_signal_state( record, &signal_state );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( bit_value == 0 ) {
		signal_state &= ( ~ bit_type );
	} else {
		signal_state |= bit_type;
	}

	mx_status = mx_rs232_set_signal_state( record, signal_state );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_rs232_get_configuration( MX_RECORD *record, long *speed,
				long *word_size, char *parity,
				long *stop_bits, char *flow_control,
				unsigned long *read_terminators,
				unsigned long *write_terminators )
{
	static const char fname[] = "mx_rs232_get_configuration()";

	MX_RS232 *rs232;
	MX_RS232_FUNCTION_LIST *flist;
	mx_status_type ( *fptr )( MX_RS232 * );
	mx_status_type mx_status;

	mx_status = mx_rs232_get_pointers( record, &rs232, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Optionally call the driver specific 'get configuration' function. */

	fptr = flist->get_configuration;

	if ( fptr != NULL ) {
		mx_status = (*fptr)( rs232 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( speed != (long *) NULL ) {
		*speed = rs232->speed;
	}
	if ( word_size != (long *) NULL ) {
		*word_size = rs232->word_size;
	}
	if ( parity != (char *) NULL ) {
		*parity = rs232->parity;
	}
	if ( stop_bits != (long *) NULL ) {
		*stop_bits = rs232->stop_bits;
	}
	if ( flow_control != (char *) NULL ) {
		*flow_control = rs232->flow_control;
	}
	if ( read_terminators != (unsigned long *) NULL ) {
		*read_terminators = rs232->read_terminators;
	}
	if ( write_terminators != (unsigned long *) NULL ) {
		*write_terminators = rs232->write_terminators;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_rs232_set_configuration( MX_RECORD *record, long speed,
				long word_size, char parity,
				long stop_bits, char flow_control,
				unsigned long read_terminators,
				unsigned long write_terminators )
{
	static const char fname[] = "mx_rs232_set_configuration()";

	MX_RS232 *rs232;
	MX_RS232_FUNCTION_LIST *flist;
	mx_status_type ( *fptr )( MX_RS232 * );
	mx_status_type mx_status;

	mx_status = mx_rs232_get_pointers( record, &rs232, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( speed != MXF_232_DONT_CARE ) {
		rs232->speed = speed;
	}
	if ( word_size != MXF_232_DONT_CARE ) {
		rs232->word_size = word_size;
	}
	if ( parity != MXF_232_DONT_CARE ) {
		rs232->parity = parity;
	}
	if ( stop_bits != MXF_232_DONT_CARE ) {
		rs232->stop_bits = stop_bits;
	}
	if ( flow_control != MXF_232_DONT_CARE ) {
		rs232->flow_control = flow_control;
	}
	if ( read_terminators != MXF_232_DONT_CARE ) {
		rs232->read_terminators = read_terminators;
	}
	if ( write_terminators != MXF_232_DONT_CARE ) {
		rs232->write_terminators = write_terminators;
	}

	/* Optionally call the driver specific 'set configuration' function. */

	fptr = flist->set_configuration;

	if ( fptr != NULL ) {
		mx_status = (*fptr)( rs232 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Update the terminator characters. */

	mx_status = mx_rs232_convert_terminator_characters( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_rs232_verify_configuration( MX_RECORD *record, long speed,
				long word_size, char parity,
				long stop_bits, char flow_control,
				unsigned long read_terminators,
				unsigned long write_terminators )
{
	static const char fname[] = "mx_rs232_verify_configuration()";

	MX_RS232 *rs232;
	mx_status_type mx_status;

	mx_status = mx_rs232_get_pointers( record, &rs232, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( speed != MXF_232_DONT_CARE ) {
		if ( speed != rs232->speed ) {
			return mx_error(
				MXE_HARDWARE_CONFIGURATION_ERROR, fname,
"The speed for RS-232 port '%s' is currently %ld, but should be %ld.",
				record->name, rs232->speed, speed );
		}
	}
	if ( word_size != MXF_232_DONT_CARE ) {
		if ( word_size != rs232->word_size ) {
			return mx_error(
				MXE_HARDWARE_CONFIGURATION_ERROR, fname,
"The word size for RS-232 port '%s' is currently %ld, but should be %ld.",
				record->name, rs232->word_size, word_size );
		}
	}
	if ( parity != (char) MXF_232_DONT_CARE ) {
		if ( parity != rs232->parity ) {
			return mx_error(
				MXE_HARDWARE_CONFIGURATION_ERROR, fname,
"The parity for RS-232 port '%s' is currently '%c', but should be '%c'.",
				record->name, rs232->parity, parity );
		}
	}
	if ( stop_bits != MXF_232_DONT_CARE ) {
		if ( stop_bits != rs232->stop_bits ) {
			return mx_error(
				MXE_HARDWARE_CONFIGURATION_ERROR, fname,
"The stop bits for RS-232 port '%s' are currently %ld, but should be %ld.",
				record->name, rs232->stop_bits, stop_bits );
		}
	}
	if ( flow_control != (char) MXF_232_DONT_CARE ) {
		if ( flow_control != rs232->flow_control ) {
			return mx_error(
				MXE_HARDWARE_CONFIGURATION_ERROR, fname,
"The flow_control for RS-232 port '%s' is currently '%c', but should be '%c'.",
				record->name,
				rs232->flow_control, flow_control );
		}
	}
	if ( read_terminators != MXF_232_DONT_CARE ) {
		if ( read_terminators != rs232->read_terminators ) {
			return mx_error(
				MXE_HARDWARE_CONFIGURATION_ERROR, fname,
"The read terminators for RS-232 port '%s' are currently %#lx, "
"but should be %#lx.", record->name,
				rs232->read_terminators, read_terminators );
		}
	}
	if ( write_terminators != MXF_232_DONT_CARE ) {
		if ( write_terminators != rs232->write_terminators ) {
			return mx_error(
				MXE_HARDWARE_CONFIGURATION_ERROR, fname,
"The write terminators for RS-232 port '%s' are currently %#lx, "
"but should be %#lx.", record->name,
				rs232->write_terminators, write_terminators );
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_rs232_send_break( MX_RECORD *record )
{
	static const char fname[] = "mx_rs232_send_break()";

	MX_RS232 *rs232;
	MX_RS232_FUNCTION_LIST *flist;
	mx_status_type ( *fptr )( MX_RS232 * );
	mx_status_type mx_status;

	mx_status = mx_rs232_get_pointers( record, &rs232, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = flist->send_break;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Sending break signals is not supported by the '%s' driver "
		"for RS-232 device '%s'.",
			mx_get_driver_name( record ), record->name );
	} else {
		mx_status = (*fptr)( rs232 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_rs232_get_echo( MX_RECORD *record,
		mx_bool_type *echo_state )
{
	static const char fname[] = "mx_rs232_get_echo()";

	MX_RS232 *rs232;
	MX_RS232_FUNCTION_LIST *flist;
	mx_status_type ( *fptr )( MX_RS232 * );
	mx_status_type mx_status;

	mx_status = mx_rs232_get_pointers( record, &rs232, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = flist->get_echo;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Getting the echo state is not supported by the '%s' driver "
		"for RS-232 device '%s'.",
			mx_get_driver_name( record ), record->name );
	} else {
		mx_status = (*fptr)( rs232 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( echo_state != (mx_bool_type *) NULL ) {
		*echo_state = rs232->echo;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_rs232_set_echo( MX_RECORD *record,
		mx_bool_type echo_state )
{
	static const char fname[] = "mx_rs232_set_echo()";

	MX_RS232 *rs232;
	MX_RS232_FUNCTION_LIST *flist;
	mx_status_type ( *fptr )( MX_RS232 * );
	mx_status_type mx_status;

	mx_status = mx_rs232_get_pointers( record, &rs232, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = flist->set_echo;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Setting the echo state is not supported by the '%s' driver "
		"for RS-232 device '%s'.",
			mx_get_driver_name( record ), record->name );
	} else {
		rs232->echo = echo_state;

		mx_status = (*fptr)( rs232 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_rs232_flush( MX_RECORD *record )
{
	static const char fname[] = "mx_rs232_flush()";

	MX_RS232 *rs232;
	MX_RS232_FUNCTION_LIST *flist;
	mx_status_type ( *fptr )( MX_RS232 * );
	mx_status_type mx_status;

	mx_status = mx_rs232_get_pointers( record, &rs232, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = flist->flush;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Flushing bytes to the destination is not supported "
		"by the '%s' driver for RS-232 device '%s'.",
			mx_get_driver_name( record ), record->name );
	} else {
		mx_status = (*fptr)( rs232 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_rs232_send_file( MX_RECORD *record,
			char *filename,
			mx_bool_type use_raw_io )
{
	static const char fname[] = "mx_rs232_send_file()";
	MX_RS232 *rs232;
	FILE *file;
	int saved_errno;
	char buffer[4096];
	size_t bytes_read;
	char *ptr;
	mx_status_type mx_status;

	mx_status = mx_rs232_get_pointers( record, &rs232, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( filename == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The filename pointer passed for rs232 port '%s' was NULL.",
			record->name );
	}

	/* Attempt to open the file. */

	file = fopen( filename, "r" );

	if ( file == (FILE *) NULL ) {
		saved_errno = errno;

		switch( saved_errno ) {
		case ENOENT:
			return mx_error( MXE_NOT_FOUND, fname,
			"The requested file '%s' for RS232 port '%s' "
			"does not exist.",
				filename, record->name );
			break;
		case EACCES:
			return mx_error( MXE_PERMISSION_DENIED, fname,
			"You do not have the permissions necessary to access "
			"file '%s' for RS232 port '%s'.",
				filename, record->name );
			break;
		default:
			return mx_error( MXE_FILE_IO_ERROR, fname,
			"The attempt to open file '%s' "
			"for RS232 port '%s' failed.  "
			"Errno = %d, error message = '%s'",
				filename, record->name,
				saved_errno, strerror(saved_errno) );
			break;
		}
	}

	/* Read in bytes from the file and send them to the RS-232 port. */

	if ( use_raw_io ) {

		while (1) {
			bytes_read = fread( buffer, 1, sizeof(buffer), file );

			if ( bytes_read == 0 ) {
				if ( feof(file) ) {
					(void) fclose( file );
					return MX_SUCCESSFUL_RESULT;
				}
				if ( ferror(file) ) {
					(void) fclose( file );
					return mx_error(MXE_FILE_IO_ERROR,fname,
					"An error occurred while reading "
					"from file '%s'.",
						filename );
				} else {
					MX_DEBUG(-2,("%s: bytes_read == 0 "
					"for file '%s', but EOF or error "
					"did not occur.", fname, filename ));
				}
			}

			mx_status = mx_rs232_write( record, buffer,
						bytes_read, 0, 0 );

			if ( mx_status.code != MXE_SUCCESS ) {
				(void) fclose( file );
				return mx_status;
			}
		}
	} else {
		/* We have been requested to use line-oriented I/O.  This is
		 * normally done if we want the RS-232 record to deal with
		 * handling line terminators.
		 */

		/* Set the FILE structure to be line buffered. */

		setvbuf( file, (char *)NULL, _IOLBF, BUFSIZ );

		/* Now copy the lines. */

		while (1) {
			ptr = mx_fgets( buffer, sizeof(buffer), file );

			if ( ptr == (char *) NULL ) {
				if ( feof(file) ) {
					(void) fclose( file );
					return MX_SUCCESSFUL_RESULT;
				}
				if ( ferror(file) ) {
					(void) fclose( file );
					return mx_error(MXE_FILE_IO_ERROR,fname,
					"An error occurred while reading "
					"from file '%s'.",
						filename );
				} else {
					MX_DEBUG(-2,("%s: NULL pointer "
					"from mx_fgets() "
					"for file '%s', but EOF or error "
					"did not occur.", fname, filename ));
				}
			}

			mx_status = mx_rs232_putline( record, buffer, NULL, 0 );

			if ( mx_status.code != MXE_SUCCESS ) {
				(void) fclose( file );
				return mx_status;
			}
		}
	}

	MXW_NOT_REACHED( return MX_SUCCESSFUL_RESULT );
}

