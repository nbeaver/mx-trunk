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
#include "mx_mutex.h"
#include "mx_atomic.h"

/* WARNING: This program cannot use any MX functions, since it runs
 * before MX has been compiled.
 */

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

#include <windows.h>

MX_EXPORT void
mx_atomic_initialize( void )
{
	return;
}

/*---*/

MX_EXPORT int32_t
mx_atomic_add32( int32_t *value_ptr, int32_t increment )
{
	volatile long *long_ptr;
	long long_result;

	long_ptr = value_ptr;

#if defined(_M_IA64)	/* Only on Itanium */
	long_result = InterlockedAdd( long_ptr, (long) increment );
#else
	long_result = (*long_ptr)++;		/* FIXME */
#endif

	return long_result;
}

MX_EXPORT int32_t
mx_atomic_decrement32( int32_t *value_ptr )
{
	LONG *long_ptr;
	LONG long_result;

	long_ptr = value_ptr;

	long_result = InterlockedDecrement( long_ptr );

	return long_result;
}

MX_EXPORT int32_t
mx_atomic_increment32( int32_t *value_ptr )
{
	LONG *long_ptr;
	LONG long_result;

	long_ptr = value_ptr;

	long_result = InterlockedIncrement( long_ptr );

	return long_result;
}

MX_EXPORT int32_t
mx_atomic_read32( int32_t *value_ptr )
{
	LONG *long_ptr;
	LONG long_result;

	long_ptr = value_ptr;

	/* Visual C++ 6 defined InterlockedCompareExchange() with
	 * a different prototype than later versions of Visual C++.
	 *
	 * The VC6 definition looked like this:
	 *
	 * PVOID WINAPI
	 * InterlockedCompareExchange( PVOID *, PVOID, PVOID );
	 *
	 * Subsequent versions of Visual C++ defined it this way:
	 *
	 * LONG WINAPI
	 * InterlockedCompareExchange( LONG *, LONG, LONG );
	 *
	 * Apparently, VC6 thought that a PVOID was equivalent to
	 * a LONG.  (Bad VC6, bad boy, ...)
	 */

#if ( _MSC_VER >= 1300 )
	long_result = InterlockedCompareExchange( long_ptr, 0, 0 );
#else
	{
		void **pvoid_ptr;

		pvoid_ptr = (void **) long_ptr;

		long_result =
			(LONG) InterlockedCompareExchange( pvoid_ptr, 0, 0 );
	}
#endif

	return long_result;
}

MX_EXPORT void
mx_atomic_write32( int32_t *value_ptr, int32_t new_value )
{
	LONG *long_ptr;

	long_ptr = value_ptr;

	(void) InterlockedExchange( long_ptr, new_value );

	return;
}

/*------------------------------------------------------------------------*/

#elif defined(OS_MACOSX) && defined(MX_HAVE_OSATOMIC)

/* For MacOS X 10.4 and above. */

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
	/* FIXME - There is no guarantee that the loop will ever succeed. */

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
}

/*------------------------------------------------------------------------*/

#elif defined(OS_SOLARIS) && ( MX_SOLARIS_VERSION >= 5010000L )

/* For Solaris 10 and above. */

#include <atomic.h>

MX_EXPORT void
mx_atomic_initialize( void )
{
	return;
}

/*---*/

MX_EXPORT int32_t
mx_atomic_add32( int32_t *value_ptr, int32_t increment )
{
	int32_t result;

	result = (int32_t) atomic_add_32_nv( (uint32_t *)value_ptr, increment );

	return result;
}

MX_EXPORT int32_t
mx_atomic_decrement32( int32_t *value_ptr )
{
	int32_t result;

	result = (int32_t) atomic_add_32_nv( (uint32_t *)value_ptr, -1 );

	return result;
}

MX_EXPORT int32_t
mx_atomic_increment32( int32_t *value_ptr )
{
	int32_t result;

	result = (int32_t) atomic_add_32_nv( (uint32_t *)value_ptr, 1 );

	return result;
}

MX_EXPORT int32_t
mx_atomic_read32( int32_t *value_ptr )
{
	int32_t result;

	result = (int32_t) atomic_add_32_nv( (uint32_t *)value_ptr, 0 );

	return result;
}

MX_EXPORT void
mx_atomic_write32( int32_t *value_ptr, int32_t new_value )
{
	/* FIXME - There is no guarantee that the loop will ever succeed. */

	uint32_t new_value_2;
	unsigned long i, loop_max;

	loop_max = 1000000L;	/* FIXME - An arbitrarily chosen limit. */

	for( i = 0; i < loop_max; i++ ) {

		atomic_and_32( (uint32_t *) value_ptr, 0 );

		/* Somebody else may have changed the value after our
		 * atomic "and", so we must check the value returned
		 * by the atomic "add" to see that we got the expected
		 * value.
		 */

		new_value_2 =
			atomic_add_32_nv( (uint32_t *) value_ptr, new_value );

		/* If the swap succeeded, then we are done. */

		if ( new_value_2 == new_value ) {
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
}

/*------------------------------------------------------------------------*/

#elif defined(OS_IRIX)

#include <sgidefs.h>
#include <mutex.h>

MX_EXPORT void
mx_atomic_initialize( void )
{
	return;
}

/*---*/

MX_EXPORT int32_t
mx_atomic_add32( int32_t *value_ptr, int32_t increment )
{
	int32_t result;

	result =
	    (int32_t) add_then_test32( (__uint32_t *) value_ptr, increment );

	return result;
}

MX_EXPORT int32_t
mx_atomic_decrement32( int32_t *value_ptr )
{
	int32_t result;

	result = (int32_t) add_then_test32( (__uint32_t *) value_ptr, -1L );

	return result;
}

MX_EXPORT int32_t
mx_atomic_increment32( int32_t *value_ptr )
{
	int32_t result;

	result = (int32_t) add_then_test32( (__uint32_t *) value_ptr, 1L );

	return result;
}

MX_EXPORT int32_t
mx_atomic_read32( int32_t *value_ptr )
{
	int32_t result;

	result = (int32_t) add_then_test32( (__uint32_t *) value_ptr, 0L );

	return result;
}

MX_EXPORT void
mx_atomic_write32( int32_t *value_ptr, int32_t new_value )
{
	(void) test_and_set32( (__uint32_t *) value_ptr, new_value );
}

/*------------------------------------------------------------------------*/

#elif defined(__GNUC__) && (MX_GNUC_VERSION >= 4001000L) && \
	( defined(__i486__) || defined(__i586__) || \
	  defined(__i686__) || defined(__MMX__) )

/* For GCC 4.1 and above.
 *
 * On x86, the atomic builtins are not available if we use march=i386.
 */

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
	/* FIXME - There is no guarantee that the loop will ever succeed. */

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
}

/*------------------------------------------------------------------------*/

/* This is an implementation using a mutex that is intended for build targets
 * not handled above.  The mutex is used to serialize all access to atomic
 * operations.  The relevant targets are:
 *
 *    GCC 4.0.x and before
 *    MacOS X 10.3.x and before
 *    Solaris 9.x and before
 *    GCC on any platform not explicitly handled above.
 */

#elif defined(__GNUC__) || defined(OS_MACOSX) || defined(OS_SOLARIS)

static MX_MUTEX *mxp_atomic_mutex = NULL;

MX_EXPORT void
mx_atomic_initialize( void )
{
	(void) mx_mutex_create( &mxp_atomic_mutex );
}

MX_EXPORT int32_t
mx_atomic_add32( int32_t *value_ptr, int32_t increment )
{
	int32_t result;

	mx_mutex_lock( mxp_atomic_mutex );

	*value_ptr += increment;
	result = *value_ptr;

	mx_mutex_unlock( mxp_atomic_mutex );

	return result;
}

MX_EXPORT int32_t
mx_atomic_decrement32( int32_t *value_ptr )
{
	int32_t result;

	mx_mutex_lock( mxp_atomic_mutex );

	(*value_ptr)--;
	result = *value_ptr;

	mx_mutex_unlock( mxp_atomic_mutex );

	return result;
}

MX_EXPORT int32_t
mx_atomic_increment32( int32_t *value_ptr )
{
	int32_t result;

	mx_mutex_lock( mxp_atomic_mutex );

	(*value_ptr)++;
	result = *value_ptr;

	mx_mutex_unlock( mxp_atomic_mutex );

	return result;
}

MX_EXPORT int32_t
mx_atomic_read32( int32_t *value_ptr )
{
	int32_t result;

	mx_mutex_lock( mxp_atomic_mutex );

	result = *value_ptr;

	mx_mutex_unlock( mxp_atomic_mutex );

	return result;
}

MX_EXPORT void
mx_atomic_write32( int32_t *value_ptr, int32_t new_value )
{
	mx_mutex_lock( mxp_atomic_mutex );

	*value_ptr = new_value;

	mx_mutex_unlock( mxp_atomic_mutex );

	return;
}

/*---*/

/*------------------------------------------------------------------------*/

#else

#error MX atomic operations have not been defined for this platform.

#endif

