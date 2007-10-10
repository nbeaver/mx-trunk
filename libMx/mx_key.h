/*
 * Name:    mx_key.h
 *
 * Purpose: MSDOS style keyboard handling functions.  
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2003, 2005, 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_KEY_H__
#define __MX_KEY_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

MX_API int mx_getch( void );

MX_API int mx_kbhit( void );

MX_API void mx_key_echo_off( void );

MX_API void mx_key_echo_on( void );

MX_API void mx_key_getline( char *buffer, size_t max_buffer_length );

#ifdef __cplusplus
}
#endif

#endif /* __MX_KEY_H__ */

