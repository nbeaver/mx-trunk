/*
 * Name:    mx_rs232.c
 *
 * Purpose: MX function library of generic RS-232 operations.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "mx_util.h"
#include "mx_rs232.h"
#include "mx_driver.h"

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
			"Invalid serial port speed = %ld", (long) rs232->speed);

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
		    "Invalid serial port word size = %d", rs232->word_size );

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
		    "Invalid serial port stop bits = %d", rs232->stop_bits );

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

	MX_DEBUG( 2,("%s: rs232->num_read_terminator_chars = %d",
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

	MX_DEBUG( 2,("%s: rs232->num_write_terminator_chars = %d",
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
			mx_hex_type transfer_flags )
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

	return mx_status;
}

MX_EXPORT mx_status_type
mx_rs232_putchar( MX_RECORD *record,
			char c,
			mx_hex_type transfer_flags )
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

	return mx_status;
}

MX_EXPORT mx_status_type
mx_rs232_getchar_with_timeout( MX_RECORD *record,
				char *c,
				mx_hex_type transfer_flags,
				double timeout_in_seconds )
{
	static const char fname[] = "mx_rs232_getchar_with_timeout()";

	MX_RS232 *rs232;
	MX_CLOCK_TICK start_tick, finish_tick, current_tick;
	MX_CLOCK_TICK timeout_clock_ticks;
	mx_status_type mx_status;

	mx_status = mx_rs232_get_pointers( record, &rs232, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Add MXF_232_NOWAIT to the transfer flags so that we can directly
	 * control the timeout.
	 */

	transfer_flags &= MXF_232_NOWAIT;

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
		return mx_error_quiet( MXE_TIMED_OUT, fname,
		"Read from RS-232 port '%s' timed out after %g seconds.",
			record->name, timeout_in_seconds );
	} else {
		return mx_error( MXE_TIMED_OUT, fname,
		"Read from RS-232 port '%s' timed out after %g seconds.",
			record->name, timeout_in_seconds );
	}
}
		
MX_EXPORT mx_status_type
mx_rs232_read( MX_RECORD *record,
		char *buffer,
		size_t max_bytes_to_read,
		size_t *bytes_read,
		mx_hex_type transfer_flags )
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

	if ( transfer_flags & MXF_232_DEBUG ) {
		MX_DEBUG(-2,("%s: received buffer = '%s'", fname, buffer));
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
		mx_hex_type transfer_flags )
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

	if ( transfer_flags & MXF_232_DEBUG ) {
		MX_DEBUG(-2, ("%s: sending buffer = '%s'", fname, buffer));
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

	return mx_status;
}

/* mx_rs232_getline() and mx_rs232_putline() are intended for reading
 * ASCII character strings.  Use mx_rs232_read() and mx_rs232_write()
 * for binary data.
 */

MX_EXPORT mx_status_type
mx_rs232_getline( MX_RECORD *record,
			char *buffer,
			size_t max_bytes_to_read,
			size_t *bytes_read,
			mx_hex_type transfer_flags )
{
	static const char fname[] = "mx_rs232_getline()";

	MX_RS232 *rs232;
	MX_RS232_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_RS232 *, char *, size_t, size_t * );
	char *array;
	int i, terminators_seen, start_of_terminator, buffered_io;
	char c;
	int do_timeout, tick_comparison;
	uint32_t num_bytes_available;
	MX_CLOCK_TICK timeout_clock_ticks, current_tick, finish_tick;
	mx_status_type mx_status;

	mx_status = mx_rs232_get_pointers( record, &rs232, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( rs232->rs232_flags & MXF_232_UNBUFFERED_IO ) {
		buffered_io = FALSE;
	} else {
		buffered_io = TRUE;
	}

	rs232->transfer_flags = transfer_flags;

	/* Does this driver have a getline function? */

	fptr = fl_ptr->getline;

	if ( buffered_io && ( fptr != NULL ) ) {

		/* If so, invoke it. */

		mx_status =
			(*fptr)(rs232, buffer, max_bytes_to_read, bytes_read);

	} else {
		/* Otherwise, handle input a character at a time. */

		if ( rs232->timeout < 0.0 ) {
			do_timeout = FALSE;
		} else {
			do_timeout = TRUE;

			timeout_clock_ticks =
			    mx_convert_seconds_to_clock_ticks( rs232->timeout );

			current_tick = mx_current_clock_tick();

			finish_tick = mx_add_clock_ticks( current_tick,
							timeout_clock_ticks );
		}

		array = rs232->read_terminator_array;

		terminators_seen = 0;
		start_of_terminator = INT_MAX;

		for ( i = 0; i < max_bytes_to_read; i++ ) {

			if ( do_timeout ) {
				current_tick = mx_current_clock_tick();

				tick_comparison = mx_compare_clock_ticks(
						current_tick, finish_tick );

				if ( tick_comparison > 0 ) {

			    	    if ( rs232->rs232_flags &
				      MXF_232_SUPPRESS_TIMEOUT_ERROR_MESSAGES )
				    {
					return mx_error_quiet(
						MXE_TIMED_OUT, fname,
		"Read from RS-232 port '%s' timed out after %g seconds.",
						record->name, rs232->timeout );
				    } else {
					return mx_error( MXE_TIMED_OUT, fname,
		"Read from RS-232 port '%s' timed out after %g seconds.",
						record->name, rs232->timeout );
				    }
				}

				mx_status = mx_rs232_num_input_bytes_available(
						record, &num_bytes_available );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

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

			mx_status = mx_rs232_getchar(record, &c, MXF_232_WAIT);

			if ( mx_status.code != MXE_SUCCESS ) {
	                        /* Make the buffer contents a valid C string
				 * before returning.
				 */

				buffer[i] = '\0';

#if 0	/* FIXME: Verify that the replacement works with all serial ports! */

				if ( mx_status.code != MXE_NOT_READY ) {
#else
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
#endif

					if ( transfer_flags & MXF_232_DEBUG ) {

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
#if 0
			if ( transfer_flags & MXF_232_DEBUG ) {
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
				if ( transfer_flags & MXF_232_IGNORE_NULLS ) {

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

		mx_status = MX_SUCCESSFUL_RESULT;
	}

	if ( transfer_flags & MXF_232_DEBUG ) {
		MX_DEBUG(-2,("%s: received buffer = '%s'", fname, buffer));
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_rs232_putline( MX_RECORD *record,
		char *buffer,
		size_t *bytes_written,
		mx_hex_type transfer_flags )
{
	static const char fname[] = "mx_rs232_putline()";

	MX_RS232 *rs232;
	MX_RS232_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_RS232 *, char *, size_t * );
	char *array;
	int i, length, buffered_io;
	char c;
	mx_status_type mx_status;

	mx_status = mx_rs232_get_pointers( record, &rs232, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( transfer_flags & MXF_232_DEBUG ) {
		MX_DEBUG(-2, ("%s: sending buffer = '%s'", fname, buffer));
	}

	if ( rs232->rs232_flags & MXF_232_UNBUFFERED_IO ) {
		buffered_io = FALSE;
	} else {
		buffered_io = TRUE;
	}

	rs232->transfer_flags = transfer_flags;

	/* Does this driver have a putline function? */

	fptr = fl_ptr->putline;

	if ( buffered_io && ( fptr != NULL ) ) {

		/* If so, invoke it. */

		mx_status = (*fptr)( rs232, buffer, bytes_written );

	} else {
		/* Otherwise, handle output a character at a time. */

		array = rs232->write_terminator_array;

		length = strlen( buffer );

		for ( i = 0; i < length; i++ ) {
			c = buffer[i];

			mx_status = mx_rs232_putchar(record, c, MXF_232_WAIT);

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

			if ( transfer_flags & MXF_232_DEBUG ) {
				MX_DEBUG(-2,
				("%s: sending write terminator 0x%x",
					fname, c));
			}

			mx_status = mx_rs232_putchar(record, c, MXF_232_WAIT);

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_rs232_num_input_bytes_available( MX_RECORD *record,
				uint32_t *num_input_bytes_available )
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
mx_rs232_discard_unread_input( MX_RECORD *record,
				mx_hex_type transfer_flags )
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
				mx_hex_type transfer_flags )
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
mx_rs232_get_signal_state( MX_RECORD *record,
			mx_hex_type *signal_state )
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
			mx_hex_type signal_state )
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
mx_rs232_get_signal_bit( MX_RECORD *record, int bit_type, int *bit_value )
{
	static const char fname[] = "mx_rs232_get_signal_bit()";

	MX_RS232 *rs232;
	mx_hex_type signal_state;
	mx_status_type mx_status;

	mx_status = mx_rs232_get_pointers( record, &rs232, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( bit_value == (int *) NULL ) {
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
mx_rs232_set_signal_bit( MX_RECORD *record, int bit_type, int bit_value )
{
	static const char fname[] = "mx_rs232_get_signal_bit()";

	MX_RS232 *rs232;
	mx_hex_type signal_state;
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
mx_rs232_verify_configuration( MX_RECORD *record, int32_t speed,
				int32_t word_size, char parity,
				int32_t stop_bits, char flow_control,
				mx_hex_type read_terminators,
				mx_hex_type write_terminators )
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
			record->name, (long) rs232->speed, (long) speed );
		}
	}
	if ( word_size != MXF_232_DONT_CARE ) {
		if ( word_size != rs232->word_size ) {
			return mx_error(
				MXE_HARDWARE_CONFIGURATION_ERROR, fname,
"The word size for RS-232 port '%s' is currently %d, but should be %d.",
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
"The stop bits for RS-232 port '%s' are currently %d, but should be %d.",
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
				(unsigned long) rs232->read_terminators,
				(unsigned long) read_terminators );
		}
	}
	if ( write_terminators != MXF_232_DONT_CARE ) {
		if ( write_terminators != rs232->write_terminators ) {
			return mx_error(
				MXE_HARDWARE_CONFIGURATION_ERROR, fname,
"The write terminators for RS-232 port '%s' are currently %#lx, "
"but should be %#lx.", record->name,
				(unsigned long) rs232->write_terminators,
				(unsigned long) write_terminators );
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

