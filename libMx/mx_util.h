/*
 * Name:    mx_util.h
 *
 * Purpose: Define utility functions and generally used symbols.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------
 *
 * Copyright 1999-2023 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_UTIL_H__
#define __MX_UTIL_H__

/** @file mx_util.h 
 * Common MX utility functions that do not use MX databases.
 */

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "mx_private_version.h"

#include <string.h>	/* We get 'size_t' from here. */
#include <stdarg.h>	/* We get 'va_list' from here. */

/*-----*/

#include <time.h>	/* We usually get 'struct timespec' from here. */

#if defined( OS_BSD )
#include <sys/time.h>	/* Sometimes we get 'struct timespec' from here. */
#endif

#if defined( OS_DJGPP )
#include <sys/wtime.h>	/* Sometimes we get 'struct timespec' from here. */
#endif

/*-----*/

/* Some build targets do not define 'struct timespec' in a header file.
 * We need to check for these special cases.  How annoying.
 */

#if defined( OS_WIN32 )
#   if defined( _MSC_VER )
#      if (_MSC_VER < 1900)
#         define __MX_NEED_TIMESPEC  1
#      else
#         define __MX_NEED_TIMESPEC  0
#      endif
#   elif defined( __GNUC__ )
#      if defined(_TIMESPEC_DEFINED)
#         define __MX_NEED_TIMESPEC  0
#      else
#         define __MX_NEED_TIMESPEC  1
#      endif
#   else
#      define __MX_NEED_TIMESPEC  1
#   endif

#elif defined( OS_VMS )
#   if (__VMS_VER < 80000000)
#      define __MX_NEED_TIMESPEC  1
#   else
#      define __MX_NEED_TIMESPEC  0
#   endif

#else
#   define __MX_NEED_TIMESPEC  0
#endif

/*-----*/

#if __MX_NEED_TIMESPEC

/* If we need to define 'struct timespec' ourselves, then we
 * include a C++ safe declaration below.
 */

#ifdef __cplusplus
extern "C" {
#endif

struct timespec {
	time_t tv_sec;	  /* seconds */
	long   tv_nsec;   /* nanoseconds */
};

#ifdef __cplusplus
}
#endif

#endif /* __MX_NEED_TIMESPEC */

/*-----*/

#ifdef __MX_NEED_TIMESPEC
#   undef __MX_NEED_TIMESPEC
#endif

/*-----*/

/* Prohibit MX code from using poisoned C runtime functions.
 *
 * This only applies to MX-supplied programs and libraries.
 * It is not imposed on third-party programs or on programs
 * that define MX_NO_POISON.
 */

#if !defined(MX_NO_POISON)
#  if defined(__MX_LIBRARY__) || defined(__MX_APP__)
#    include "mx_poison.h"
#  endif
#endif

/*-----*/

/*
 * Macros for declaring shared library or DLL functions.
 *
 * The MX_API and MX_EXPORT macros are used for the declaration of functions
 * in shared libraries or DLLs.  They must be used by publically visible
 * functions in a shared library or DLL and must _not_ be used by functions
 * that are not in a shared library or DLL.
 *
 * MX_API and MX_EXPORT are similar but not the same.  MX_API is used only
 * in header files (.h files), while MX_EXPORT is used only in .c files
 * where the body of the function appears.
 */

#if defined(OS_WIN32)
#   if defined(__GNUC__)	/* MinGW */
#      if defined(__MX_LIBRARY__)
#         define MX_API		__attribute__ ((dllexport))
#         define MX_EXPORT	__attribute__ ((dllexport))
#      else
#         define MX_API		__attribute__ ((dllimport))
#         define MX_EXPORT	__ERROR_ONLY_USE_THIS_IN_LIBRARIES__
#      endif
#   else
#      if defined(__MX_LIBRARY__)
#         define MX_API		_declspec(dllexport)
#         define MX_EXPORT	_declspec(dllexport)
#      else
#         define MX_API		_declspec(dllimport)
#         define MX_EXPORT	__ERROR_ONLY_USE_THIS_IN_LIBRARIES__
#      endif
#   endif

#elif defined(OS_VMS)
#      define MX_API		extern
#      define MX_EXPORT

#else
#   if defined(__MX_LIBRARY__)
#      define MX_API		extern
#      define MX_EXPORT
#   else
#      define MX_API		extern
#      define MX_EXPORT	__ERROR_ONLY_USE_THIS_IN_LIBRARIES__
#   endif
#endif

/*
 * The MX_API_PRIVATE macro is used to indicate functions that may be exported
 * by the MX library, but which are not intended to be used by general
 * application programs.  At present, it is just an alias for MX_API, but
 * this may change later.
 */

#define MX_API_PRIVATE		MX_API

/*-----*/

#if defined(OS_WIN32)
#   define MX_STDCALL	__stdcall
#else
#   define MX_STDCALL
#endif

/*------------------------------------------------------------------------*/

#if ( defined(_MSC_VER) && (_MSC_VER >= 1400) )
   /* Visual C++ 2005 Express and above. */

   /* FIXME: The following statement disables Visual C++ warning messages
    *        about deprecated functions.  Someday we should rewrite the
    *        code so that we no longer need this.
    */

#  pragma warning( disable:4996 )
#endif

/*------------------------------------------------------------------------*/

/*
 * Solaris 2 defines SIG_DFL, SIG_ERR, SIG_IGN, and SIG_HOLD in a way
 * such that GCC on Solaris 2 generates a warning about a function
 * definition being an invalid prototype.
 */

/* Not sure which version of Solaris this was for.  For now we disable it. */

#if 0 && defined( OS_SOLARIS ) && defined( __GNUC__ )

# ifdef SIG_DFL
#   undef SIG_DFL
# endif

# ifdef SIG_ERR
#   undef SIG_ERR
# endif

# ifdef SIG_IGN
#   undef SIG_IGN
# endif

# ifdef SIG_HOLD
#   undef SIG_HOLD
# endif

#define SIG_DFL  (void(*)(int))0
#define SIG_ERR  (void(*)(int))-1
#define SIG_IGN  (void(*)(int))1
#define SIG_HOLD (void(*)(int))2

#endif /* OS_SOLARIS && __GNUC__ */

/*------------------------------------------------------------------------*/

#if defined( OS_DJGPP )

   /* Prevent Watt-32 from making its own definitions of int16_t and int32_t. */

#  define HAVE_INT16_T
#  define HAVE_INT32_T

   /* This has to appear before we include <sys/param.h> below, which
    * includes <sys/swap.h>, which requires these prototypes to exist.
    * They are also used by the mx_socket.h header file.
    */
   extern __inline__ unsigned long  __ntohl( unsigned long );
   extern __inline__ unsigned short __ntohs( unsigned short );

#endif /* OS_DJGPP */

#if defined( OS_WIN32 )
#  include <stdlib.h>
#  define MXU_FILENAME_LENGTH		_MAX_PATH

#elif defined( OS_VXWORKS )
#  include <limits.h>
#  define MXU_FILENAME_LENGTH		PATH_MAX

#elif defined( OS_ECOS ) || defined( OS_HURD )
#  include <limits.h>
#  define MXU_FILENAME_LENGTH		_POSIX_PATH_MAX

#elif defined( OS_VMS )
#  define MXU_FILENAME_LENGTH		255	/* According to comp.os.vms */

#else
#  include <sys/param.h>
#  if defined( MAXPATHLEN )
#     define MXU_FILENAME_LENGTH	MAXPATHLEN
#  else
#     error Maximum path length not yet defined for this platform.
#  endif
#endif

/*------------------------------------------------------------------------*/

#if defined( __GNUC__)
#  define MXW_UNUSED( x ) \
	(void) (x)
#elif 0
#  define MXW_UNUSED( x ) \
	do { (x) = (x); } while(0)
#else
#  define MXW_UNUSED( x )
#endif

/*------------------------------------------------------------------------*/

/*
 * Some compilers will emit a warning message if they find a statement
 * that can never be reached.  For some other compilers, they will emit
 * a warning if a statement is _not_ placed at the unreachable location.
 * An example of this is a while(1) loop that can never be broken out of 
 * with a return statement following it.
 *
 * The MXW_NOT_REACHED() was created to handle this situation.
 *
 * Note for Solaris:
 *   In principle, for the Sun Studio or Solaris Studio compiler you could
 *   bracket the not reached line with something like this
 *
 *   #pragma error_messages (off, E_STATEMENT_NOT_REACHED)
 *       unreached_line();
 *   #pragma error_messages (on, E_STATEMENT_NOT_REACHED)
 *
 *   But you cannot use #pragmas in a macro definition.
 *
 *   Alternately you could use C99's _Pragma() which can go in a macro.
 *   But older versions of Sun Studio do not have _Pragma(), so we can't
 *   use it.
 *
 *   For now, we deal with this by just not emitting the offending statement.
 *
 * Note for #else case:
 *
 *   The 'x' at the end of the macro _must_ _not_ be surrounded by a pair
 *   of parenthesis.  If you _did_ put in parenthesis, you would end up
 *   with situations like
 *
 *   (return MX_SUCCESSFUL_RESULT);
 *
 *   which is not valid C.
 */

#if defined(__SUNPRO_C) || defined(__USLC__) \
	|| ( defined(OS_HPUX) && defined(__ia64) )
#  define MXW_NOT_REACHED( x )
#else
#  define MXW_NOT_REACHED( x )    x
#endif

/*------------------------------------------------------------------------*/

/* We need to make sure that FALSE and TRUE are defined to be 0 and something
 * else that is not 0.
 *
 * Some very old versions of Glibc define TRUE and FALSE in <rpc/types.h>,
 * but do not test for the prior existence of the TRUE and FALSE macros.
 *
 * For those versions, the cleanest solution is to include <rpc/types.h>
 * here before our own definition of TRUE and FALSE.
 */

#if ( defined(MX_GLIBC_VERSION) && (MX_GLIBC_VERSION < 2001000L) )
#  include <rpc/types.h>

#else
#  if !defined(TRUE)
#     define TRUE	1
#  endif

#  if !defined(FALSE)
#     define FALSE	0
#  endif
#endif

/*------------------------------------------------------------------------*/

/* Define malloc(), free(), etc. as well as memory debugging functions. */

#include "mx_malloc.h"

/*------------------------------------------------------------------------*/

/* Make the rest of the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------------*/

/* The following definitions allow for typechecking of printf and scanf
 * style function arguments with GCC.
 */

#if defined( __GNUC__ )

#define MX_PRINTFLIKE( a, b ) __attribute__ ((format (printf, a, b)))
#define MX_SCANFLIKE( a, b )  __attribute__ ((format (scanf, a, b)))

#else

#define MX_PRINTFLIKE( a, b )
#define MX_SCANFLIKE( a, b )

#endif

/*------------------------------------------------------------------------*/

/* MX_INLINE tells the compiler to think about the hypothetical possibility
 * of inlining the function that follows the keyword.  If it wants to.
 * In the fullness of time.
 *
 * Usually you will want to say 'static MX_INLINE'.
 */

#if ( defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901l) )
    /* C99 */
#   define MX_INLINE	inline

#elif defined( OS_WIN32 )
#   define MX_INLINE	_inline

#elif defined(__GNUC__)
#   define MX_INLINE	__inline__

#elif defined(__USLC__)
    /* UnixWare C compiler. */
#   define MX_INLINE

#else
#   error MX_INLINE is not yet implemented for this build target.
#endif

/*------------------------------------------------------------------------*/

#define MX_WHITESPACE		" \t"

#define MXU_STRING_LENGTH       20
#define MXU_BUFFER_LENGTH	400
#define MXU_HOSTNAME_LENGTH	100

/*------------------------------------------------------------------------*/

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

/*------------------------------------------------------------------------*/

/* Environment variable related functions. */

/**
 * Set the value of an environment variable.
 *
 * @param[in] env_name    Environment variable name.
 * @param[in] env_value   New value of the environment string.
 *
 * @return Returns -1 if setting the environment variable failed
 * and 0 if it succeeded.
 *
 * For Linux, changing the value of the environment variable LD_LIBRARY_PATH
 * after your program has started will not have any effect on the search
 * for shared libraries.  The dynamic loader checks the the value of
 * the variable LD_LIBRARY_PATH only once at program startup time before
 * the program's main() function is called.  Therefore any changes to 
 * LD_LIBRARY_PATH after main() has been invoked will have no effect.
 */

MX_API int mx_setenv( const char *env_name, const char *env_value );

/**
 * Expand environment variables in the provided string.
 *
 * @param[out] expanded_env_value  The environment variable string after expansion.
 * @param[in] original_env_value   The environment variable string before expansion.
 * @param[in] max_env_size         The maximum length of 'new_env_value'.
 *
 * @return Returns -1 on failure.  Otherwise returns the length of 
 * the expanded string.
 *
 * mx_expand_env() replaces instances of %%VAR% (for Windows)
 * or $VAR (for everyone else) in the original string with the contents of
 * those variables.
 */

MX_API int mx_expand_env( char *expanded_env_value,
			const char *original_env_value,
			size_t max_env_size );

/*------------------------------------------------------------------------*/

/* Sleep functions with higher resolution than sleep(). */

/**
 * Asks that the current process sleep for the requested number of seconds.
 *
 * @param[in] seconds
 *
 * @return Returns nothing.
 *
 */

MX_API void mx_sleep( unsigned long seconds );

/**
 * Asks that the current process sleep for the requested number of milliseconds.
 *
 * @param[in] milliseconds
 *
 * @return Returns nothing.
 *
 */

MX_API void mx_msleep( unsigned long milliseconds );

/**
 * Asks that the current process sleep for the requested number of microseconds.
 *
 * @param[in] microseconds
 *
 * @return Returns nothing.
 *
 */

MX_API void mx_usleep( unsigned long microseconds );

/*--- Stack debugging tools ---*/

/**
 * Display a stack traceback for the current thread.
 *
 * @return Returns nothing.
 *
 * mx_stack_traceback() does its best to provide a traceback of the
 * function call stack at the time of invocation.  However, the 
 * functionality available differs between different operating systems,
 * so the amount of available information can vary.  Furthermore, if
 * the stack for the current thread is corrupted, then a call to
 * mx_stack_traceback() may return incorrect information and may
 * even crash your program.
 */

MX_API void mx_stack_traceback( void );

/**
 * Check for stack corruption.
 *
 * @return Returns TRUE if stack is valid.  Might return FALSE if
 * the stack is not valid.
 *
 * mx_stack_check() tries to see if the stack is in a valid state.
 * It returns TRUE if the stack is valid and attempts to return FALSE
 * if the stack is not valid.  However, in cases of bad stack frame
 * corruption, it may not be possible to successfully invoke and
 * return from this function, so keep one's expectations low.
 */

MX_API int mx_stack_check( void );

/*--- Stack reporting tools ---*/

/**
 * Estimates how much stack is unused.
 *
 * @return Number of bytes unused on the stack.
 *
 * mx_stack_available() _might_ not take into account things like
 * automatic stack expansion or might not work at all depending on
 * the platform, so don't take it too seriously.  It is just
 * for helping with debugging.
 *
 * NOTE: mx_stack_available() relies on mx_initialize_stack_calc()
 * being called at or near the beginning of the main() function
 * for your program.
 */

MX_API long mx_stack_available( void );

/**
 * Initializes the stack reporting tools like mx_stack_available().
 *
 * @param[in] stack_base    The address of a variable at the beginning of the stack.
 *
 * @return Returns nothing.
 *
 * The 'stack_base' variable should be the address of the first variable
 * allocated on the stack for your main() function.
 * The mx_stack_available() function will not work correctly if
 * mx_initialize_stack_calc() was not called at the start of your program.
 * Note that variables with the 'static' keyword in their definition
 * cannot be used, since they are not allocated on the stack.
 */

MX_API void mx_initialize_stack_calc( const void *stack_base );

/**
 * Returns the stack_base pointer saved by mx_initialize_stack_calc().
 *
 * @return Returns the stack base pointer.
 */

MX_API const void *mx_get_stack_base( void );

/**
 * Report the direction that the stack grows in.
 *
 * @return Returns TRUE if the stack grows up and FALSE if the stack grows down.
 */

MX_API int mx_stack_grows_up( void );

/**
 * Reports the byte offset between the supplied stack address and the stack base.
 *
 * @param[in] stack_base      Should be the stack base address reported by
 *   mx_get_stack_base().
 * @param[in] stack_address   Should be the address of a variable on the stack.
 *
 * @return Returns the address offset in bytes betweeen the two addresses
 * on the stack.
 *
 * Note that if the two addresses are not both from the stack, then 
 * mx_get_stack_offset() will not return a meaningful value.
 */

MX_API long mx_get_stack_offset( const void *stack_base,
				const void *stack_address );

/*--- Heap debugging tools ---*/

/* mx_heap_pointer_is_valid() checks to see if the supplied pointer
 * was allocated from the heap.
 */

MX_API int mx_heap_pointer_is_valid( void *pointer );

/* mx_heap_check() looks to see if the heap is in a valid state.
 * It returns TRUE if the heap is valid and FALSE if the heap is
 * corrupted.  Your program is likely to crash soon if the heap 
 * is corrupted though.
 */

/*--- Flag values for mx_heap_check() ---*/

#define MXF_HEAP_CHECK_OK			0x1
#define MXF_HEAP_CHECK_OK_VERBOSE		0x2
#define MXF_HEAP_CHECK_CORRUPTED		0x4
#define MXF_HEAP_CHECK_CORRUPTED_VERBOSE	0x8
#define MXF_HEAP_CHECK_STACK_TRACEBACK		0x10

#define MXF_HEAP_CHECK_OK_ALL	(MXF_HEAP_CHECK_OK | MXF_HEAP_CHECK_OK_VERBOSE)
#define MXF_HEAP_CHECK_CORRUPTED_ALL \
		(MXF_HEAP_CHECK_CORRUPTED | MXF_HEAP_CHECK_CORRUPTED_VERBOSE)

MX_API int mx_heap_check( unsigned long heap_flags );

/*--- Other debugging tools ---*/

/* mx_pointer_is_valid() __attempts__ to see if the memory range pointed
 * to by the supplied pointer is mapped into the current process with
 * the requested access mode.  The length argument is the length of the
 * memory range that is checked.  The access mode is a bit-wise OR of the
 * desired R_OK and W_OK bits as defined in "mx_unistd.h".
 *
 * WARNING: mx_pointer_is_valid() is not guaranteed to work on all
 * platforms by any means.  On some platforms, it may have a severe
 * performance penalty and can have other very undesirable side effects.
 * You should not use this function during normal operation!
 */

MX_API int mx_pointer_is_valid( void *pointer, size_t length, int access_mode );

/*
 * mx_force_core_dump() attempts to force the creation of a snapshot
 * of the state of the application that can be examined with a debugger
 * and then exit.  On Unix, this is done with a core dump.  On other
 * platforms, an appropriate platform-specific action should occur.
 */

MX_API void mx_force_core_dump( void );

/*
 * mx_force_immediate_exit() attempts to force the current process to
 * immediately exit without performing any of the normal shutdown procedures.
 * On some platforms, mx_force_immediate_exit() is more immediate and ugly
 * than something like _exit().
 *
 * For Windows, this ensures that an external debugger will not be invoked
 * as part of the shutdown.
 */

MX_API void mx_force_immediate_exit( void );

/* This is a duplicate of the definition in mx_debugger.h.  It is here just
 * so that we do not need to include "mx_debugger.h" everywhere.
 */

MX_API void mx_breakpoint( void );

/*
 * mx_hex_char_to_unsigned_long() converts a hexadecimal character to an
 * unsigned long integer.  mx_hex_string_to_unsigned_long() does the same
 * thing for a string.
 */

MX_API unsigned long mx_hex_char_to_unsigned_long( char c );

MX_API unsigned long mx_hex_string_to_unsigned_long( char *string );

MX_API long mx_hex_string_to_long( char *string );

/* mx_string_to_long() and mx_string_to_unsigned_long() can handle decimal,
 * octal, and hexadecimal representations of numbers.  If the string starts
 * with '0x' the number is taken to be hexadecimal.  If not, then if it
 * starts with '0', it is taken to be octal.  Otherwise, it is taken to be
 * decimal.  The functions are just wrappers for strtol() and strtoul(), so
 * read strtol's and strtoul's man pages for the real story.
 */

MX_API long mx_string_to_long( char *string );

MX_API unsigned long mx_string_to_unsigned_long( char *string );

/* mx_round() rounds to the nearest integer, while mx_round_away_from_zero()
 * and mx_round_toward_zero() round to the integer in the specified direction.
 *
 * The mx_round_away_from_zero() function subtracts threshold from the
 * number before rounding it up, while mx_round_toward_zero() adds it.
 * This is to prevent numbers that are only non-integral due to roundoff
 * error from being incorrectly rounded to the wrong integer.
 *
 * mx_round_down() rounds to the next lower integer, while mx_round_up()
 * rounds to the next higher integer.
 */

MX_API long mx_round( double value );
MX_API long mx_round_down( double value );
MX_API long mx_round_up( double value );
MX_API long mx_round_away_from_zero( double value, double threshold );
MX_API long mx_round_toward_zero( double value, double threshold );

/* mx_multiply_safely multiplies two floating point numbers by each other
 * while testing for and avoiding infinities.
 */

MX_API double mx_multiply_safely( double multiplier1, double multiplier2 );

/* mx_divide_safely divides two floating point numbers by each other
 * while testing for and avoiding division by zero.
 */

MX_API double mx_divide_safely( double numerator, double denominator );

/* mx_difference() computes a relative difference function. */

MX_API double mx_difference( double value1, double value2 );

/* mx_match() does simple wildcard matching. */

MX_API int mx_match( const char *pattern, const char *string );

/* mx_parse_command_line() takes a string and creates argv[] and envp[]
 * style arrays from it.  Spaces and tabs are taken to be whitespace, while
 * text enclosed by double quotes are assumed to be a single token.  Tokens
 * with an embedded '=' character are assumed to be environment variables.
 * The argv and envp arrays are terminated by NULL entries.
 *
 * The argv and envp arrays returned by mx_parse_command_line() must be
 * freed by mx_free_command_line().  It is not sufficient to just directly
 * free the argv and envp arrays, since mx_free_command_line() must also
 * free an internal buffer created by strdup().
 */

MX_API int mx_parse_command_line( const char *command_line,
		int *argc, char ***argv, int *envc, char ***envp );

MX_API void mx_free_command_line( char **argv, char **envp );

/* mx_free_pointer() is a wrapper for free() that attempts to verify
 * that the pointer is valid to be freed before trying to free() it.
 * The function returns TRUE if freeing the memory succeeded and 
 * FALSE if not.
 */

MX_API void mx_free_pointer( void *pointer );

/* mx_free() is a macro that checks to see if the pointer is NULL
 * before attempting to free it.  After the pointer is freed, the
 * local copy of the pointer is set to NULL.
 */

#define mx_free( ptr ) \
			do {					\
				if ( (ptr) != NULL ) {		\
					free((void *) ptr);	\
					(ptr) = NULL;		\
				} \
			} while (0)

#if ( defined(OS_WIN32) && defined(_MSC_VER) ) || defined(OS_VXWORKS) \
	|| defined(OS_DJGPP) || (defined(OS_VMS) && (__VMS_VER < 70320000 ))

/* These provide definitions of snprintf() and vsnprintf() for systems
 * that do not come with them.  On most such systems, snprintf() and
 * vsnprintf() are merely redefined as sprintf() and vsprintf().
 * Obviously, this removes the buffer overrun safety on such platforms.
 * However, snprintf() and vsnprintf() are supported on most systems
 * and hopefully the buffer overruns will be detected on systems
 * that that support them.
 */

#define snprintf	mx_snprintf
#define vsnprintf	mx_vsnprintf

MX_API int mx_snprintf( char *dest, size_t maxlen, const char *format, ... );

MX_API int mx_vsnprintf( char *dest, size_t maxlen, const char *format,
							va_list args );
#endif

/* If you want to use something like snprintf() to print a list of arguments,
 * but you do not know how many arguments you will have until run time, then 
 * snprintf() cannot help you, since C does not allow you to manually create
 * a va_list using portable C code.
 *
 * Instead, we use our homebrew mx_snprint_from_pointer_array(), where you
 * provide an array of void pointers to individual arguments that you
 * want to print.  snprintf() is actually used to implement the printing
 * of individual items from 'pointer_array'.
 */

MX_API int mx_snprintf_from_pointer_array( char *dest,
					size_t maxlen,
					const char *format,
					size_t num_pointers,
					void **pointer_array );

#if defined(OS_LINUX) || defined(OS_WIN32) || defined(OS_IRIX) \
	|| defined(OS_HPUX) || defined(OS_TRU64) || defined(OS_VMS) \
	|| defined(OS_QNX) || defined(OS_VXWORKS) || defined(OS_RTEMS) \
	|| defined(OS_DJGPP) || defined(OS_ECOS) || defined(OS_HURD)

/* These prototypes provide definitions of strlcpy() and strlcat() for
 * systems that do not come with them.  For systems that do not come with
 * them, the OpenBSD source code for strlcpy() and strlcat() is bundled
 * with the base MX distribution in the directory mx/tools/generic/src.
 */

MX_API size_t strlcpy( char *dest, const char *src, size_t maxlen );

MX_API size_t strlcat( char *dest, const char *src, size_t maxlen );

#endif

#if defined(OS_WIN32) || defined(OS_QNX) || defined(OS_HURD) \
	|| defined(OS_SOLARIS) || defined(OS_UNIXWARE) \
	|| ( defined(OS_CYGWIN) && (CYGWIN_VERSION_DLL_COMBINED < 2000000) ) \
	|| ( defined(MX_GLIBC_VERSION) && (MX_GLIBC_VERSION < 2001000) )

/* This prototype provides a definition of strcasestr() for systems that
 * do not come with it.  For such systems, the OpenBSD source code for
 * strcasestr() is bundled with the base MX distribution in the directory
 * mx/tools/generic/src.
 */

MX_API char *strcasestr( const char *s, const char *find );

#endif

#if defined(OS_WIN32) || defined(OS_DJGPP) || defined(OS_VXWORKS)

/* This prototype provides a definition of strptime() for systems that
 * do not come with it.  For such systems, the NetBSD source code for
 * strptime() is bundled with the base MX distribution in the directory
 * mx/tools/generic/strptime.
 */

MX_API char *strptime( const char *s, const char *format, struct tm *tm );

#endif

/* mx_strncmp_end() is analogous to strncmp(), but it compares its second
 * argument to the _end_ of the first argument instead of the beginning.
 */

MX_API int mx_strncmp_end( const char *s1, const char *s2, size_t n );

#define mx_strcmp_end(s1,s2)  mx_strncmp_end( (s1), (s2), strlen((s2)) )

/*----------------------*/

/* mx_fopen() is a wrapper for fopen() that works around issues with certain
 * platforms.
 *
 * The most common issue is on Windows where each DLL or EXE in a process has
 * its own separate copies of the stdio-related variables.  mx_fopen() is
 * located in the libMx library, which means that all calls to mx_fopen()
 * make use of the libMx versions of the stdio variables.
 */

MX_API FILE *mx_fopen( const char *pathname, const char *mode );

MX_API int mx_fclose( FILE *stream );

/* mx_fgets() is a replacement for fgets() that automatically trims off
 * trailing newlines.
 */

MX_API char *mx_fgets( char *s, int size, FILE *stream );

/*----------------------*/

/* Case insensitive string comparisons. */

#if defined(OS_VXWORKS)

   MX_API int mx_strcasecmp( const char *s1, const char *s2 );
   MX_API int mx_strncasecmp( const char *s1, const char *s2, size_t n );

#elif defined(_MSC_VER) || defined(__BORLANDC__) || defined(OS_DJGPP) \
	|| defined(OS_QNX)
#  define mx_strcasecmp   stricmp
#  define mx_strncasecmp  strnicmp

#else
#  define mx_strcasecmp   strcasecmp
#  define mx_strncasecmp  strncasecmp
#endif

/* The following function is only used by Visual C++ 6.0 SP6 and before. */

#if ( defined(_MSC_VER) && (_MSC_VER < 1300) )
MX_API double mx_uint64_to_double( unsigned __int64 );
#endif

/* == Debugging functions. == */

MX_API const char *mx_timestamp( char *buffer, size_t buffer_length );

/* Note that in any call to MX_DEBUG(), _all_ the arguments together 
 * after the debug level are enclosed in _one_ extra set of parentheses.
 * If you don't do this, the code will not compile.
 *
 * For example,
 *    MX_DEBUG( 2, ("The current value of foo is %d\n", foo) );
 * 
 * This is so that the preprocessor will treat everything after "2,"
 * as one _big_ macro argument.  This is the closest one can come to
 * "varargs" macros in standard ANSI C.  Also, the line below that
 * reads "mx_debug_function  text ;" is _not_ in error and is _not_
 * missing any needed parentheses.  The preprocessor will get the
 * needed parentheses from the extra set of parentheses in the
 * original invocation of MX_DEBUG().  Go read up on just how macro
 * expansion by the C preprocessor is supposed to work if you want
 * to understand this better.  Also, the Frequently Asked Questions
 * file for comp.lang.c on Usenet has a few words about this.
 *
 * Yes, it's a trick, but it's a trick that works very well as long
 * you don't forget those extra set of parentheses.  Incidentally,
 * you still need the parentheses even if the format field in the
 * second argument is a constant string with no %'s in it.  That's
 * because if you don't you'll end up after the macro expansion
 * with something like:   mx_debug_function "This doesn't work" ;
 */

MX_API void mx_debug_function( const char *format, ... ) MX_PRINTFLIKE( 1, 2 );

#ifndef DEBUG
#define MX_DEBUG( level, text )
#else
#define MX_DEBUG( level, text ) \
		if ( (level) <= mx_get_debug_level() )  { \
			mx_debug_function  text ; \
		}
#endif

MX_API void mx_set_debug_level( int debug_level );
MX_API int  mx_get_debug_level( void );

MX_API void mx_set_debug_output_function( void (*)( char * ) );
MX_API void mx_debug_default_output_function( char *string );

MX_API void mx_debug_pause( const char *format, ... ) MX_PRINTFLIKE( 1, 2 );

/* === User interrupts. === */

#define MXF_USER_INT_NONE	0
#define MXF_USER_INT_ABORT	1
#define MXF_USER_INT_PAUSE	2

#define MXF_USER_INT_ERROR	(-1)

MX_API int  mx_user_requested_interrupt( void );
MX_API int  mx_user_requested_interrupt_or_pause( void );
MX_API void mx_set_user_interrupt_function( int (*)( void ) );
MX_API int  mx_default_user_interrupt_function( void );
MX_API void mx_set_user_requested_interrupt( int interrupt_flag );

/* === Informational messages. === */

MX_API void mx_info( const char *format, ... ) MX_PRINTFLIKE( 1, 2 );

MX_API void mx_info_dialog( char *text_prompt,
					char *gui_prompt,
					char *button_label );

MX_API void mx_info_entry_dialog( char *text_prompt,
					char *gui_prompt,
					int echo_characters,
					char *response,
					size_t max_response_length );

MX_API void mx_set_info_output_function( void (*)( char * ) );
MX_API void mx_info_default_output_function( char *string );

MX_API void mx_set_info_dialog_function( void (*)( char *, char *, char * ) );
MX_API void mx_info_default_dialog_function( char *, char *, char * );

MX_API void mx_set_info_entry_dialog_function(
			void (*)( char *, char *, int, char *, size_t ) );
MX_API void mx_info_default_entry_dialog_function(
					char *, char *, int, char *, size_t );

/* Informational messages during a scan are handled specially, since some
 * applications may want to suppress them, but not suppress other messages.
 */

MX_API void mx_scanlog_info( const char *format, ... ) MX_PRINTFLIKE( 1, 2 );

MX_API void mx_set_scanlog_enable( int enable_flag );
MX_API int  mx_get_scanlog_enable( void );

/* === Warning messages. === */

MX_API void mx_warning( const char *format, ... ) MX_PRINTFLIKE( 1, 2 );

MX_API void mx_set_warning_output_function( void (*)( char * ) );
MX_API void mx_warning_default_output_function( char *string );

/* === Error messages. === */

#define MXU_ERROR_MESSAGE_LENGTH	2000

#if ( defined(USE_STACK_BASED_MX_ERROR) && USE_STACK_BASED_MX_ERROR )

typedef struct {
	long code;		/* The error code. */
	const char *location;	/* Function name where the error occurred. */
	char message[MXU_ERROR_MESSAGE_LENGTH+1]; /* The specific error msg.*/
} mx_status_type;

#else /* not USE_STACK_BASED_MX_ERROR */

typedef struct {
	long code;		/* The error code. */
	const char *location;	/* Function name where the error occurred. */
	char *message;                            /* The specific error msg.*/
} mx_status_type;

#endif /* USE_STACK_BASED_MX_ERROR */

MX_API mx_status_type mx_error( long error_code,
				const char *location,
				const char *format, ... ) MX_PRINTFLIKE( 3, 4 );

#define MX_CHECK_FOR_ERROR( function )				\
	do { 							\
		mx_status_type mx_private_status;		\
								\
		mx_private_status = (function);			\
								\
		if ( mx_private_status.code != MXE_SUCCESS )	\
			return mx_private_status;		\
	} while(0)

MX_API char *mx_strerror(long error_code, char *buffer, size_t buffer_length);

MX_API long mx_errno_to_mx_status_code( int errno_value );

MX_API const char *mx_errno_string( long errno_value );
MX_API const char *mx_status_code_string( long mx_status_code_value );

MX_API void mx_set_error_output_function( void (*)( char * ) );
MX_API void mx_error_default_output_function( char *string );

MX_API mx_status_type mx_successful_result( void );

#define MX_SUCCESSFUL_RESULT	mx_successful_result()

#if defined(OS_WIN32)
MX_API long mx_win32_error_message( long error_code,
					char *buffer, size_t buffer_length );
#endif

/*------------------------------------------------------------------------*/

/* Setup the parts of the MX runtime environment that do not depend
 * on the presence of an MX database.
 */

MX_API mx_status_type mx_initialize_runtime( void ); 

/*------------------------------------------------------------------------*/

MX_API mx_status_type mx_get_os_version_string( char *version_string,
					size_t max_version_string_length );

MX_API mx_status_type mx_get_os_version( int *os_major,
					int *os_minor,
					int *os_update );

MX_API unsigned long mx_get_os_version_number( void );

#if defined(OS_WIN32)
MX_API mx_status_type mx_win32_get_osversioninfo( unsigned long *major,
						unsigned long *minor,
						unsigned long *platform_id,
						unsigned char *product_type );

MX_API mx_status_type mx_win32_is_windows_9x( int *is_windows_9x );
#endif /* OS_WIN32 */

MX_API mx_status_type mx_get_cpu_architecture( char *architecture_type,
					size_t max_architecture_type_length,
					char *architecture_subtype,
					size_t max_architecture_subtype_length);

MX_API mx_status_type mx_get_system_boot_time( struct timespec *boot_timespec );

MX_API mx_status_type mx_get_system_boot_time_from_ticks(
					struct timespec *boot_timespec );

/*------------------------------------------------------------------------*/

/* Flag bits for mx_copy_file(). */

#define MXF_CP_OVERWRITE		0x1

#define MXF_CP_USE_CLASSIC_COPY		0x80000000

/* mx_copy_file() copies an old file to a new file where new_file_mode
 * specifies the permissions for the new file using the same bit patterns
 * for the mode as the Posix open() and creat() calls.
 */

MX_API mx_status_type mx_copy_file( char *original_filename,
				char *new_filename,
				int new_file_mode,
				unsigned long copy_flags );

MX_API mx_status_type mx_copy_file_classic( char *original_filename,
				char *new_filename,
				int new_file_mode );

MX_API mx_status_type mx_show_file_metadata( char *filename );

MX_API mx_status_type mx_show_fd_metadata( int file_descriptor );

#if defined(OS_WIN32)
MX_API mx_status_type mx_show_handle_metadata( void *win32_handle );
#endif

MX_API mx_status_type mx_get_num_lines_in_file( char *filename,
						size_t *num_lines_in_file );

MX_API mx_status_type mx_skip_num_lines_in_file( FILE *file,
						size_t num_lines_to_skip );

MX_API mx_status_type mx_get_current_directory_name( char *filename_buffer,
						size_t max_filename_length );

MX_API mx_status_type mx_change_filename_prefix( const char *old_filename,
						const char *old_prefix,
						const char *new_prefix,
						char *new_filename,
						size_t max_new_filename_length);

MX_API mx_status_type mx_is_subdirectory( const char *parent_directory,
					const char *possible_subdirectory,
					int *is_subdirectory );

MX_API mx_status_type mx_construct_file_name_from_file_pattern(
						char *filename_buffer,
						size_t filename_buffer_size,
						const char file_pattern_char,
						unsigned long file_number,
						const char *file_pattern );

MX_API mx_status_type mx_make_directory_hierarchy( char *directory_name );

MX_API int mx_command_found( char *command_name );

/*--- Flag bits used by mx_find_file_in_path() ---*/

#define MXF_FPATH_LOOK_IN_CURRENT_DIRECTORY	0x1
#define MXF_FPATH_TRY_WITHOUT_EXTENSION		0x2

MX_API mx_status_type mx_find_file_in_path( const char *original_filename,
					char *full_filename,
					size_t full_filename_length,
					const char *path_variable_name,
					const char *extension,
					int file_access_mode,
					unsigned long flags,
					int *match_found );

/* mx_verify_directory() verifies the existence of the specified directory
 * and optionally creates it if it does not already exist.
 */

MX_API mx_status_type mx_verify_directory( char *directory_name,
					int create_flag );

/* Converts a filename into the canonical form for that operating system. */

MX_API mx_status_type mx_canonicalize_filename( char *original_name,
						char *canonical_name,
					size_t max_canonical_name_length );

/*------------------------------------------------------------------------*/

MX_API mx_status_type mx_get_filesystem_root_name( char *filename,
						char *root_name,
						size_t max_root_name_length );

/* The following flags are used to report the filesystem type below. */

#define MXF_FST_NOT_FOUND	1
#define MXF_FST_NOT_MOUNTED	2

#define MXF_FST_LOCAL		0x100000
#define MXF_FST_REMOTE		0x200000

#define MXF_FST_ISO9660		(MXF_FST_LOCAL + 1)
#define MXF_FST_UDF		(MXF_FST_LOCAL + 2)

#define MXF_FST_FAT		(MXF_FST_LOCAL + 1001)
#define MXF_FST_EXFAT		(MXF_FST_LOCAL + 1002)
#define MXF_FST_NTFS		(MXF_FST_LOCAL + 1003)

#define MXF_FST_EXT2		(MXF_FST_LOCAL + 2002)
#define MXF_FST_EXT3		(MXF_FST_LOCAL + 2003)
#define MXF_FST_EXT4		(MXF_FST_LOCAL + 2004)

#define MXF_FST_NFS		(MXF_FST_REMOTE + 3001)
#define MXF_FST_SMB		(MXF_FST_REMOTE + 3002)
#define MXF_FST_AFP		(MXF_FST_REMOTE + 3003)

MX_API mx_status_type mx_get_filesystem_type( char *filename,
					unsigned long *filesystem_type );

/*------------------------------------------------------------------------*/

/* Flags for mx_spawn() */

#define MXF_SPAWN_NEW_SESSION		0x1

#define MXF_SPAWN_NO_PRELOAD		0x10000000

MX_API mx_status_type mx_spawn( char *command_line,
				unsigned long flags,
				unsigned long *process_id );

MX_API int mx_process_id_exists( unsigned long process_id );

MX_API mx_status_type mx_kill_process_id( unsigned long process_id );

MX_API unsigned long mx_get_process_id( void );

MX_API unsigned long mx_get_parent_process_id( unsigned long process_id );

MX_API mx_status_type mx_get_process_name_from_process_id(
						unsigned long process_id,
						char *name_buffer,
						size_t name_buffer_length );

MX_API mx_status_type mx_wait_for_process_id( unsigned long process_id,
						long *process_status );

MX_API void mx_abort_after_timeout( double timeout_seconds );

/*------------------------------------------------------------------------*/

MX_API char *mx_username( char *buffer, size_t buffer_length );

MX_API mx_status_type mx_get_number_of_cpu_cores( unsigned long *num_cores );

MX_API unsigned long mx_get_current_cpu_number( void );

MX_API mx_status_type mx_get_process_affinity_mask( unsigned long process_id,
							unsigned long *mask );

MX_API mx_status_type mx_set_process_affinity_mask( unsigned long process_id,
							unsigned long mask );

/*----*/

MX_API int mx_get_true_or_false( char *true_false_string );

/*----*/

MX_API char *mx_skip_string_fields( char *buffer, int num_fields );

/* mx_string_token() extracts the next token from a string using the
 * characters in 'delim' as token separators.  It is similar to strsep()
 * except for the fact that it treats a string of several delimiters in
 * a row as being only one delimiter.  By contrast, strsep() would say
 * that there were empty tokens between each of the delimiter characters.
 * 
 * Please note that the original contents of *string_ptr are modified.
 */

MX_API char *mx_string_token( char **string_ptr, const char *delim );

/* mx_string_split() uses mx_string_token() to break up the contents of
 * a string into an argv style array.  The original string is modified,
 * so you should make a copy of it if you want to keep the original
 * contents.
 */

MX_API int mx_string_split( char *original_string, const char *delim,
					int *argc, char ***argv );

/*------------------------------------------------------------------------*/

/* mx_utf8_strlen() returns the number of UTF-8 characters in the
 * supplied C string buffer.
 */

MX_API size_t mx_utf8_strlen( const char *utf8_string );

/*------------------------------------------------------------------------*/

MX_API unsigned long mx_random( void );

MX_API void mx_seed_random( unsigned long seed );

MX_API unsigned long mx_get_random_max( void );

/*------------------------------------------------------------------------*/

/* === Define error message codes. === */

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "mx_error_codes.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#ifdef __cplusplus
}
#endif

#endif /* __MX_UTIL_H__ */
