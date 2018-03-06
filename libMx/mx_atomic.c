/*
 * Name:    mx_atomic.c
 *
 * Purpose: Atomic operation functions.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2009, 2011-2012, 2014, 2016-2018 Illinois Institute of Technology
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

/*---*/

#if defined(OS_MACOSX)
#  if defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__)
#     if (__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ >= 101200)
#        define MX_MACOSX_HAVE_STDATOMIC
#     elif (__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ >= 1040)
#        define MX_MACOSX_HAVE_OSATOMIC
#     else
#        error Peculiar version of MacOS X 10.3 or before used.
#     endif
#  else
#     include <AvailabilityMacros.h>

#     if defined(MAC_OS_X_VERSION_10_4) && \
		(MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4)
#        define MX_MACOSX_HAVE_OSATOMIC
#     endif
#  endif
#endif

/*------------------------------------------------------------------------*/

#if defined(OS_WIN32)

#define MXP_NEED_GENERIC_WRITE32	FALSE

#include <windows.h>

MX_EXPORT void
mx_atomic_initialize( void )
{
	return;
}

/*---*/

#if ( defined( MX_WIN32_WINDOWS ) && ( MX_WIN32_WINDOWS <= 0x400 ) )

/* Windows 95 did not have InterlockedExchangeAdd(), so we
 * have to do something ugly with a mutex.
 */

MX_EXPORT int32_t
mx_atomic_add32( int32_t *value_ptr, int32_t increment )
{
	static MX_MUTEX *mx_atomic_add32_mutex = NULL;

	int32_t int32_result;
	mx_status_type mx_status;

	/* Create the mutex if it does not already exist. */

	if ( mx_atomic_add32_mutex == (MX_MUTEX *) NULL ) {

		mx_status = mx_mutex_create( &mx_atomic_add32_mutex );

		if ( mx_status.code != MXE_SUCCESS ) {
			/* This really has to be a fatal error, because
			 * if synchronization fails, then we cannot 
			 * count on our system being consistent.
			 */

			(void) mx_error( MXE_OPERATING_SYSTEM_ERROR,
				"mx_atomic_add32()",
			"Unable to create a mutex for the Windows 95 "
			"version of mx_atomic_add32().  This is fatal." );

			mx_force_core_dump();

			exit(1);	/* Should not get here. */
		}
	}

	mx_mutex_lock( mx_atomic_add32_mutex );

	int32_result = *value_ptr + increment;

	mx_mutex_unlock( mx_atomic_add32_mutex );

	return int32_result;
}

#else

/* Not Windows 95 */

MX_EXPORT int32_t
mx_atomic_add32( int32_t *value_ptr, int32_t increment )
{
	LONG *long_ptr;
	LONG long_result, original_value;

	long_ptr = (LONG *) value_ptr;

	original_value = InterlockedExchangeAdd( long_ptr, (LONG) increment );

	long_result = original_value + increment;

	return long_result;
}
#endif

MX_EXPORT int32_t
mx_atomic_decrement32( int32_t *value_ptr )
{
	LONG *long_ptr;
	LONG long_result;

	long_ptr = (LONG *) value_ptr;

	long_result = InterlockedDecrement( long_ptr );

	return long_result;
}

MX_EXPORT int32_t
mx_atomic_increment32( int32_t *value_ptr )
{
	LONG *long_ptr;
	LONG long_result;

	long_ptr = (LONG *) value_ptr;

	long_result = InterlockedIncrement( long_ptr );

	return long_result;
}

MX_EXPORT int32_t
mx_atomic_read32( int32_t *value_ptr )
{
	LONG *long_ptr;
	LONG long_result;

	long_ptr = (LONG *) value_ptr;

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

#elif ( _MSC_VER >= 1200 )
	/* Visual C++ 6 */

	{
		void **pvoid_ptr;

		pvoid_ptr = (void **) long_ptr;

		long_result =
			(LONG) InterlockedCompareExchange( pvoid_ptr, 0, 0 );
	}

#else
	/* Visual C++ 4 doesn't have InterlockedCompareExchange() at all. */

	{
		long_result = *long_ptr;	/* _not_ necessarily atomic! */
	}
#endif

	return long_result;
}

MX_EXPORT void
mx_atomic_write32( int32_t *value_ptr, int32_t new_value )
{
	LONG *long_ptr;

	long_ptr = (LONG *) value_ptr;

	(void) InterlockedExchange( long_ptr, new_value );

	return;
}

#if ( defined(_MSC_VER) && (_MSC_VER < 1300) )

/* FIXME: Need a replacement for MemoryBarrier() on old Microsoft platforms. */

MX_EXPORT void
mx_atomic_memory_barrier( void )
{
	return;
}

#else

MX_EXPORT void
mx_atomic_memory_barrier( void )
{
	MemoryBarrier();

	return;
}

#endif

/*------------------------------------------------------------------------*/

#elif defined(OS_MACOSX) 

/*-----*/

#  if defined(MX_MACOSX_HAVE_STDATOMIC)

/* For MacOS x 10.12 and above (or macOS if you insist). */

#define MXP_NEED_GENERIC_WRITE32	TRUE

static MX_MUTEX *mxp_atomic_write32_mutex = NULL;

#include <stdatomic.h>
#include <libkern/OSAtomic.h>

MX_EXPORT void
mx_atomic_initialize( void )
{
	(void) mx_mutex_create( &mxp_atomic_write32_mutex );
}

/*---*/

MX_EXPORT int32_t
mx_atomic_add32( int32_t *value_ptr, int32_t increment )
{
	int32_t old_value, new_value;

	old_value = atomic_fetch_add((_Atomic int32_t*) value_ptr, increment );

	new_value = old_value + increment;

	return new_value;
}

MX_EXPORT int32_t
mx_atomic_decrement32( int32_t *value_ptr )
{
	int32_t old_value, new_value;

	old_value = atomic_fetch_sub((_Atomic int32_t*) value_ptr, 1 );

	new_value = old_value - 1;

	return new_value;
}

MX_EXPORT int32_t
mx_atomic_increment32( int32_t *value_ptr )
{
	int32_t old_value, new_value;

	old_value = atomic_fetch_add((_Atomic int32_t*) value_ptr, 1 );

	new_value = old_value + 1;

	return new_value;
}

MX_EXPORT int32_t
mx_atomic_read32( int32_t *value_ptr )
{
	return atomic_fetch_add( (_Atomic int32_t*) value_ptr, 0 );
}

MX_EXPORT void
mx_atomic_memory_barrier( void )
{
	/* Apparently memory_order_acq_rel does not provide all possible
	 * memory ordering, but memory_order_seq_cst does.
	 */

	atomic_thread_fence( memory_order_seq_cst );

	return;
}

/*-----*/

#  elif defined(MX_MACOSX_HAVE_OSATOMIC)

/* For MacOS X 10.4 and above. */

#define MXP_NEED_GENERIC_WRITE32	TRUE

static MX_MUTEX *mxp_atomic_write32_mutex = NULL;

#include <libkern/OSAtomic.h>

MX_EXPORT void
mx_atomic_initialize( void )
{
	(void) mx_mutex_create( &mxp_atomic_write32_mutex );
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
mx_atomic_memory_barrier( void )
{
#error Not yet tested for old Mac OS X.

	UInt8 test_variable;

	OSTestAndSet( 1, &test_variable );

	return;
}

#  endif

/*-----*/

/*------------------------------------------------------------------------*/

#elif defined(OS_SOLARIS) && ( MX_SOLARIS_VERSION >= 5010000L ) \
	|| defined(OS_MINIX)

/* For Solaris 10 and above. */

#define MXP_NEED_GENERIC_WRITE32	TRUE

static MX_MUTEX *mxp_atomic_write32_mutex = NULL;

#include <atomic.h>

MX_EXPORT void
mx_atomic_initialize( void )
{
	(void) mx_mutex_create( &mxp_atomic_write32_mutex );
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
mx_atomic_memory_barrier( void )
{
	membar_producer();

	return;
}

/*------------------------------------------------------------------------*/

#elif defined(OS_IRIX)

#define MXP_NEED_GENERIC_WRITE32	FALSE

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

MX_EXPORT void
mx_atomic_memory_barrier( void )
{
#error Very much not yet tested for Irix.

	/* In general, MX will use a mutex to rendezvous with another thread,
	 * so for our case the following is overly elaborate.  Oh well.
	 */
	barrier_t mx_barrier_shared_area;

	barrier_t *mx_barrier = new_barrier( &mx_barrier_shared_area );

	barrier( mx_barrier, 1 );

	free_barrier( mx_barrier );

	return;
}

/*------------------------------------------------------------------------*/

#elif defined(OS_QNX)

#define MXP_NEED_GENERIC_WRITE32	TRUE

static MX_MUTEX *mxp_atomic_write32_mutex = NULL;

#include <atomic.h>

MX_EXPORT void
mx_atomic_initialize( void )
{
	(void) mx_mutex_create( &mxp_atomic_write32_mutex );
}

/*---*/

MX_EXPORT int32_t
mx_atomic_add32( int32_t *value_ptr, int32_t increment )
{
	int32_t result;

	result = (int32_t) atomic_add_value( (uint32_t *)value_ptr, increment );

	return result;
}

MX_EXPORT int32_t
mx_atomic_decrement32( int32_t *value_ptr )
{
	int32_t result;

	result = (int32_t) atomic_add_value( (uint32_t *)value_ptr, -1 );

	return result;
}

MX_EXPORT int32_t
mx_atomic_increment32( int32_t *value_ptr )
{
	int32_t result;

	result = (int32_t) atomic_add_value( (uint32_t *)value_ptr, 1 );

	return result;
}

MX_EXPORT int32_t
mx_atomic_read32( int32_t *value_ptr )
{
	int32_t result;

	result = (int32_t) atomic_add_value( (uint32_t *)value_ptr, 0 );

	return result;
}

MX_EXPORT void
mx_atomic_memory_barrier( void )
{
#error not yet implemented.  QNX docs mention pthread_barrier_init() etc.
}

/*------------------------------------------------------------------------*/

#elif defined(OS_VMS) && !defined(__VAX)

#define MXP_NEED_GENERIC_WRITE32	TRUE

static MX_MUTEX *mxp_atomic_write32_mutex = NULL;

#include <builtins.h>

MX_EXPORT void
mx_atomic_initialize( void )
{
	(void) mx_mutex_create( &mxp_atomic_write32_mutex );
}

/*---*/

MX_EXPORT int32_t
mx_atomic_add32( int32_t *value_ptr, int32_t increment )
{
	__ADD_ATOMIC_LONG( value_ptr, increment );

	return *value_ptr;
}

MX_EXPORT int32_t
mx_atomic_decrement32( int32_t *value_ptr )
{
	__ADD_ATOMIC_LONG( value_ptr, -1 );

	return *value_ptr;
}

MX_EXPORT int32_t
mx_atomic_increment32( int32_t *value_ptr )
{
	__ADD_ATOMIC_LONG( value_ptr, 1 );

	return *value_ptr;
}

MX_EXPORT int32_t
mx_atomic_read32( int32_t *value_ptr )
{
	__ADD_ATOMIC_LONG( value_ptr, 0 );

	return *value_ptr;
}

MX_EXPORT void
mx_atomic_memory_barrier( void )
{
#error Not yet implemented for OpenVMS.  Look up Synchronization Primitives.
}

/*------------------------------------------------------------------------*/

#elif defined( MX_CLANG_VERSION) || \
      ( defined(__GNUC__) && (MX_GNUC_VERSION >= 4001000L) && \
	( defined(__i486__) || defined(__i586__) || \
	  defined(__i686__) || defined(__MMX__) ) )

/* For GCC 4.1 and above.
 *
 * On x86, the atomic builtins are not available if we use march=i386.
 */

#define MXP_NEED_GENERIC_WRITE32	TRUE

static MX_MUTEX *mxp_atomic_write32_mutex = NULL;

MX_EXPORT void
mx_atomic_initialize( void )
{
	(void) mx_mutex_create( &mxp_atomic_write32_mutex );
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
mx_atomic_memory_barrier( void )
{
	__sync_synchronize();

	return;
}

/*------------------------------------------------------------------------*/

/* This is an implementation using a mutex that is intended for build targets
 * not handled above.  The mutex is used to serialize all access to atomic
 * operations.  The relevant targets are:
 *
 *    GCC 4.0.x and before
 *    MacOS X 10.3.x and before
 *    Solaris 9.x and before
 *    Vax VMS
 *    GCC on any platform not explicitly handled above.
 */

#elif defined(__GNUC__) || defined(OS_MACOSX) || defined(OS_SOLARIS) \
	|| defined(OS_VMS) || defined(OS_UNIXWARE)

#define MXP_NEED_GENERIC_WRITE32	FALSE

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

/* WARNING: Note that the following fallback "implementation" of a memory
 * barrier does not actually do anything.  Trying to emulate atomic ops
 * on a platform that does not support atomics is a rather dicey thing
 * to do anyway.
 */

MX_EXPORT void
mx_atomic_memory_barrier( void )
{
	return;
}

/*---*/

/*------------------------------------------------------------------------*/

#else

#error MX atomic operations have not been defined for this platform.

#endif

/*------------------------------------------------------------------------*/

/* The following is a lock-based version of mx_atomic_write32() for platforms
 * that do not have an intrinsic version of this.
 */

#if MXP_NEED_GENERIC_WRITE32

MX_EXPORT void
mx_atomic_write32( int32_t *value_ptr, int32_t new_value )
{
	mx_mutex_lock( mxp_atomic_write32_mutex );

	*value_ptr = new_value;

	mx_mutex_unlock( mxp_atomic_write32_mutex );

	return;
}

#endif /* MXP_NEED_GENERIC_WRITE32 */

