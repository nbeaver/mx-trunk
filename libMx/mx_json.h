/*
 * Name:    mx_json.h
 *
 * Purpose: A thin wrapper around cJSON.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------
 *
 * Copyright 2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_JSON_H__
#define __MX_JSON_H__

#ifdef __cplusplus
extern "C" {
#endif

#if defined(OS_WIN32)
#  define __WINDOWS__
#  if defined(__MX_LIBRARY__)
#    define CJSON_EXPORT_SYMBOLS
#  else
#    define CJSON_IMPORT_SYMBOLS
#  endif
#endif

#if defined(__MX_LIBRARY__)
#  include "../tools/cJSON/cJSON.h"
#else
#  include "cJSON.h"
#endif


/*---*/

typedef struct {
	cJSON *cjson;
} MX_JSON;

MX_API mx_status_type mx_json_initialize( void );

MX_API mx_status_type mx_json_parse( MX_JSON **json,
	       				char *json_string );

MX_API mx_status_type mx_json_delete( MX_JSON *json );

MX_API mx_status_type mx_json_get_key( MX_JSON *json,
					char *key_name,
					long key_datatype,
					void *key_value,
					size_t max_key_value_bytes );

/*---*/

#ifdef __cplusplus
}
#endif

#endif /* __MX_JSON_H__ */

