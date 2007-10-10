/*
 * Name:    mx_array.h
 *
 * Purpose: Header for MX array handling functions.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2000, 2003, 2005-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_ARRAY_H__
#define __MX_ARRAY_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#define mx_allocate_array	mx_allocate_array_old
#define mx_free_array		mx_free_array_old

MX_API void *mx_allocate_array_old( long num_dimensions,
					long *dimension_array,
					size_t *data_element_size_array );

MX_API mx_status_type mx_free_array_old( void *array_pointer,
					long num_dimensions,
					long *dimension_array,
					size_t *data_element_size_array );

MX_API void *mx_read_void_pointer_from_memory_location(
					void *memory_location );

MX_API void mx_write_void_pointer_to_memory_location(
					void *memory_location, void *ptr );

MX_API mx_status_type mx_get_array_size( long num_dimensions,
					long *dimension_array,
					size_t *data_element_size_array,
					size_t *array_size );

MX_API size_t mx_get_scalar_element_size( long mx_datatype,
					mx_bool_type truncate_64bit_longs );

/*---*/

MX_API mx_status_type mx_array_add_overlay( void *vector_pointer,
					long num_dimensions,
					long *dimension_array,
					size_t *data_element_size_array,
					void **array_pointer );

MX_API mx_status_type mx_array_free_overlay( void *array_pointer,
					long num_dimensions );

MX_API void *mx_array_get_vector( void *array_pointer,
					long num_dimensions );

MX_API void *mx_allocate_array_new( long num_dimensions,
					long *dimension_array,
					size_t *data_element_size_array );

MX_API mx_status_type mx_free_array_new( void *array_pointer,
					long num_dimensions,
					long *dimension_array,
					size_t *data_element_size_array );

/*---*/

MX_API mx_status_type mx_copy_array_to_buffer( void *array_pointer,
		mx_bool_type array_is_dynamically_allocated,
		long mx_datatype, long num_dimensions,
		long *dimension_array, size_t *data_element_size_array,
		void *destination_buffer, size_t destination_buffer_length,
		size_t *num_bytes_copied,
		mx_bool_type truncate_64bit_longs );

MX_API mx_status_type mx_copy_buffer_to_array(
		void *source_buffer, size_t source_buffer_length,
		void *array_pointer,
		mx_bool_type array_is_dynamically_allocated,
		long mx_datatype, long num_dimensions,
		long *dimension_array, size_t *data_element_size_array,
		size_t *num_bytes_copied,
		mx_bool_type truncate_64bit_longs );

#define MX_XDR_ENCODE	0
#define MX_XDR_DECODE	1

MX_API size_t mx_xdr_get_scalar_element_size( long mx_datatype );

MX_API mx_status_type mx_xdr_data_transfer( int direction,
		void *array_pointer,
		int array_is_dynamically_allocated,
		long mx_datatype, long num_dimensions,
		long *dimension_array, size_t *data_element_size_array,
		void *destination_buffer, size_t destination_buffer_length,
		size_t *num_bytes_copied );

MX_API mx_status_type mx_convert_and_copy_array(
		void *source_array_pointer,
		long source_datatype,
		long source_num_dimensions,
		long *source_dimension_array,
		size_t *source_data_element_size_array,
		void *destination_array_pointer,
		long destination_datatype,
		long destination_num_dimensions,
		long *destination_dimension_array,
		size_t *destination_data_element_size_array );

#ifdef __cplusplus
}
#endif

#endif /* __MX_ARRAY_H__ */

