/*
 * Name:    mx_atomic.c
 *
 * Purpose: Atomic operation functions.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_util.h"
#include "mx_stdint.h"
#include "mx_atomic.h"

/*---*/

#if defined(OS_MACOSX)
#  include <AvailabilityMacros.h>

#  if defined(MAC_OS_X_VERSION_10_4) && \
		(MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4)
#     define MX_HAVE_OSATOMIC
#  endif
#endif

/*------------------------------------------------------------------------*/

#if defined(OS_WIN32)

#error Not done yet.

/*------------------------------------------------------------------------*/

#elif defined(OS_MACOSX) && defined(MX_HAVE_OSATOMIC)

#include <libkern/OSAtomic.h>

MX_EXPORT void
mx_atomic_initialize( void )
{
	return;
}

/*---*/

MX_EXPORT int32_t
mx_atomic_add32( int32_t *value_ptr, int32_t increment )
{
	return OSAtomicAdd32Barrier( increment, value_ptr );
}

MX_EXPORT int32_t
mx_atomic_decrement32( int32_t *value_ptr )
{
	return OSAtomicDecrement32Barrier( value_ptr );
}

MX_EXPORT int32_t
mx_atomic_increment32( int32_t *value_ptr )
{
	return OSAtomicIncrement32Barrier( value_ptr );
}

MX_EXPORT int32_t
mx_atomic_read32( int32_t *value_ptr )
{
	return OSAtomicAdd32Barrier( 0, value_ptr );
}

MX_EXPORT void
mx_atomic_write32( int32_t *value_ptr, int32_t new_value )
{
#if 0
	*value_ptr = new_value;		/* This can't be right. */
#else
	/* FIXME - This sucks massively.  There is no guarantee that
	 * the loop will ever succeed.
	 */

	int32_t old_value;
	int status;
	unsigned long i, loop_max;

	loop_max = 1000000L;	/* FIXME - An arbitrarily chosen limit. */

	for( i = 0; i < loop_max; i++ ) {

		old_value = OSAtomicAdd32Barrier( 0, value_ptr );

		status = OSAtomicCompareAndSwap32Barrier(
					old_value, new_value, value_ptr );

		/* If the swap succeeded, then we are done. */

		if ( status ) {
			break;
		}
	}

	if ( i >= loop_max ) {
		fprintf( stderr, "mx_atomic_write() FAILED, i = %lu\n", i );
	} else {
#if 1
		fprintf( stderr, "mx_atomic_write(): i = %lu\n", i );
#endif
	}

#endif
}

/*------------------------------------------------------------------------*/

#elif defined(__GNUC__) && (MX_GNUC_VERSION >= 4001000L)

MX_EXPORT void
mx_atomic_initialize( void )
{
	return;
}

/*---*/

MX_EXPORT int32_t
mx_atomic_add32( int32_t *value_ptr, int32_t increment )
{
	return __sync_add_and_fetch( value_ptr, increment );
}

MX_EXPORT int32_t
mx_atomic_decrement32( int32_t *value_ptr )
{
	return __sync_add_and_fetch( value_ptr, -1 );
}

MX_EXPORT int32_t
mx_atomic_increment32( int32_t *value_ptr )
{
	return __sync_add_and_fetch( value_ptr, 1 );
}

MX_EXPORT int32_t
mx_atomic_read32( int32_t *value_ptr )
{
	return __sync_add_and_fetch( value_ptr, 0 );
}

MX_EXPORT void
mx_atomic_write32( int32_t *value_ptr, int32_t new_value )
{
#if 0
	*value_ptr = new_value;		/* This can't be right. */
#else
	/* FIXME - This sucks massively.  There is no guarantee that
	 * the loop will ever succeed.
	 */

	int32_t old_value;
	int status;
	unsigned long i, loop_max;

	loop_max = 1000000L;	/* FIXME - An arbitrarily chosen limit. */

	for( i = 0; i < loop_max; i++ ) {

		old_value = __sync_add_and_fetch( value_ptr, 0 );

		status = __sync_bool_compare_and_swap( value_ptr,
						old_value, new_value );

		/* If the swap succeeded, then we are done. */

		if ( status ) {
			break;
		}
	}

	if ( i >= loop_max ) {
		fprintf( stderr, "mx_atomic_write() FAILED, i = %lu\n", i );
	} else {
#if 1
		fprintf( stderr, "mx_atomic_write(): i = %lu\n", i );
#endif
	}

#endif
}

/*------------------------------------------------------------------------*/

#else

#endif

