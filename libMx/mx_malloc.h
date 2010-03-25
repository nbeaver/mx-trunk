/*
 * Name:    mx_malloc.h
 *
 * Purpose: Memory allocation related utility functions.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_MALLOC_H__
#define __MX_MALLOC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#if defined(DEBUG_DMALLOC)

#  define DMALLOC		1
#  define DMALLOC_FUNC_CHECK	1

#  include "dmalloc.h"

#  define MX_MALLOC_REDIRECT	TRUE

#  if defined(DEBUG_MPATROL) || defined(DEBUG_DUMA)
#    error You must not define more than one of DEBUG_DMALLOC, DEBUG_MPATROL, and DEBUG_DUMA at the same time!
#  endif

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#elif defined(DEBUG_MPATROL)

#  include "mpatrol.h"

#  define MX_MALLOC_REDIRECT	TRUE

#  if defined(DEBUG_DMALLOC) || defined(DEBUG_DUMA)
#    error You must not define more than one of DEBUG_DMALLOC, DEBUG_MPATROL, and DEBUG_DUMA at the same time!
#  endif

/*------------------------------------------------------------------------*/

#elif defined(DEBUG_DUMA)

#  define MX_MALLOC_REDIRECT	TRUE

#  if defined(DEBUG_DMALLOC) || defined(DEBUG_MPATROL)
#    error You must not define more than one of DEBUG_DMALLOC, DEBUG_MPATROL, and DEBUG_DUMA at the same time!
#  endif

/*------------------------------------------------------------------------*/

#else
#  define MX_MALLOC_REDIRECT	FALSE
#endif

/*------------------------------------------------------------------------*/

#if defined(OS_WIN32) && ( MX_MALLOC_REDIRECT == FALSE )

   /* We need to make sure that MX DLLs and EXEs are all using the same heap,
    * so we define replacements for the malloc(), etc. functions that use
    * HeapAlloc(), etc. on the heap returned by GetProcessHeap().
    */

   MX_API void *mx_win32_calloc( size_t, size_t );
   MX_API void  mx_win32_free( void * );
   MX_API void *mx_win32_malloc( size_t );
   MX_API void *mx_win32_realloc( void *, size_t );

#  define calloc(x,y)  mx_win32_calloc((x),(y))
#  define free(x)      mx_win32_free(x)
#  define malloc(x)    mx_win32_malloc(x)
#  define realloc(x,y) mx_win32_realloc((x),(y))

#endif

/*------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __MX_MALLOC_H__ */
