/*
 * Name:    mx_debugger.h
 *
 * Purpose: Define functions for dynamically debugging programs.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------
 *
 * Copyright 1999-2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_DEBUGGER_H__
#define __MX_DEBUGGER_H__

#include "mx_thread.h"

/*
 * mx_start_debugger() attempts to start an external debugger such as the
 * Visual C++ debugger, DDD, gdb, etc. with the program stopped at the
 * line where mx_start_debugger() was invoked.
 *
 * The 'command' argument to mx_start_debugger() is a platform-dependent
 * way of changing the debugger to be started, modifying the way the debugger
 * is run, or anything else appropriate for a given platform.  If the 'command'
 * argument is set to NULL, then a default debugging environment will be
 * started.
 *
 * Warning: This feature is not implemented on all platforms.
 */

MX_API void mx_prepare_for_debugging( char *command,
				int just_in_time_debugging );

MX_API int mx_just_in_time_debugging_is_enabled( void );

MX_API void mx_start_debugger( char *command );

MX_API int mx_debugger_is_present( void );

MX_API void mx_wait_for_debugger( void );

/* mx_breakpoint_helper() is a tiny function that can be used as the
 * target of a debugger breakpoint, assuming it hasn't been optimized
 * away by compiler optimization.  It exists because some versions of
 * GDB have problems with setting breakpoints in C++ constructors.
 * By adding a call to the empty mx_breakpoint_helper() function, it
 * now becomes easy to set a breakpoint in a constructor.
 */

MX_API int mx_breakpoint_helper( void );

/* mx_breakpoint() inserts a debugger breakpoint into the code.
 * This function is not available on all platforms.
 *
 * Note: There is a duplicate of the definition of mx_breakpoint() in mx_util.h
 */

MX_API void mx_breakpoint( void );

/* "Numbered breakpoints" only fire when their breakpoint number has
 * been enabled.
 */

MX_API void mx_numbered_breakpoint( unsigned long breakpoint_number );

MX_API void mx_set_numbered_breakpoint( unsigned long breakpoint_number,
					int breakpoint_enable );

MX_API int mx_get_numbered_breakpoint( unsigned long breakpoint_number );

/* Watchpoints (also known as data breakpoints) are used to interrupt the
 * flow of execution when a user specified range of addresses is either
 * read/written/executed depending on the access flags supplied when 
 * the watchpoint is initially set.
 */

typedef struct mx_watchpoint_type{
	MX_THREAD *target_thread;
	void *value_pointer;
	long value_datatype;
	unsigned long flags;
	void *callback_function;
	void *callback_arguments;

	void *watchpoint_private;

	/* The following is a place to store the old value found at
	 * the watchpoint.  This assumes that the value to be stored
	 * is not longer than 8 bytes (64 bits).  The truth of this
	 * assumption is tested by a preprocessor define in the
	 * file mx_debugger.c.
	 */
	unsigned char old_value[8];
} MX_WATCHPOINT;

MX_API int mx_set_watchpoint( MX_WATCHPOINT **watchpoint,
			MX_THREAD *target_thread,
			void *value_pointer,
			long value_datatype,
			unsigned long flags,
			void *callback_function( MX_WATCHPOINT *, void * ),
			void *callback_arguments );

MX_API int mx_clear_watchpoint( MX_WATCHPOINT *watchpoint );

MX_API int mx_show_watchpoints( void );

/*
 * mx_set_debugger_started_flag() provides a way to directly set the internal
 * 'mx_debugger_started' flag.  This can be useful if an MX program is started
 * manually from a debugger.
 */

MX_API void mx_set_debugger_started_flag( int started_flag );

MX_API int mx_get_debugger_started_flag( void );

/*
 * mx_global_debug_pointer and mx_global_debug_initialized are used to
 * provide a global "debug" object that can be accessed from anywhere
 * in MX.  These objects are provided only for debugging and should
 * not be used in normal operation.  These declarations are only
 * valid in C.  If you use them from C++ code, you may get multiple
 * definitions of symbols.
 */

#ifndef __cplusplus
MX_API int mx_global_debug_initialized[10];
MX_API void *mx_global_debug_pointer[10];
#endif

#endif /* __MX_DEBUGGER_H__ */
