/*
 * Name:    mx_array.h
 *
 * Purpose: Header for MX array handling functions.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2000, 2003, 2005-2007, 2012-2015
 *    Illinois Institute of Technology
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

/*---------------------------------------------------------------------------*/

#if defined(__GNUC__)
#  if defined(__BIGGEST_ALIGNMENT__)
#    define MX_MAXIMUM_ALIGNMENT	__BIGGEST_ALIGNMENT__
#  else
#    define MX_MAXIMUM_ALIGNMENT	16
#  endif

#elif defined(_MSC_VER)
#  define MX_MAXIMUM_ALIGNMENT		8

#elif defined(__SUNPRO_C)
#  if defined(__sparcv9)
#    define MX_MAXIMUM_ALIGNMENT	16
#  elif defined(__x86_64)
#    define MX_MAXIMUM_ALIGNMENT	16
#  else
#    define MX_MAXIMUM_ALIGNMENT	8
#  endif

#elif defined(OS_VMS)
#  define MX_MAXIMUM_ALIGNMENT		8

#else
#  error This platform does not yet implement the MX_MAXIMUM_ALIGNMENT macro.
#endif

/*---------------------------------------------------------------------------*/

#define MX_ARRAY_HEADER_WORD_TYPE	uint32_t
#define MX_ARRAY_HEADER_WORD_SIZE	sizeof(MX_ARRAY_HEADER_WORD_TYPE)

#define MX_ARRAY_HEADER_MAGIC		0xd6d8ebed

#define MX_ARRAY_OFFSET_MAGIC		(-1)
#define MX_ARRAY_OFFSET_HEADER_LENGTH	(-2)
#define MX_ARRAY_OFFSET_NUM_DIMENSIONS	(-3)
#define MX_ARRAY_OFFSET_DIMENSION_ARRAY	(-4)

/*---------------------------------------------------------------------------*/

MX_API void *mx_read_void_pointer_from_memory_location(
					void *memory_location );

MX_API void mx_write_void_pointer_to_memory_location(
					void *memory_location, void *ptr );

/*---*/

MX_API mx_status_type mx_compute_array_size( long num_dimensions,
					long *dimension_array,
					size_t *data_element_size_array,
					size_t *array_size );

MX_API size_t mx_get_scalar_element_size( long mx_datatype,
					mx_bool_type longs_are_64bits );

/*---*/

MX_API mx_status_type mx_compute_array_header_length(
				unsigned long *array_header_length_in_bytes,
				long num_dimensions,
				long *dimension_array,
				size_t *data_element_size_array );

MX_API mx_status_type mx_setup_array_header( void *array_pointer,
					unsigned long array_header_length,
					long num_dimensions,
					long *dimension_array,
					size_t *data_element_size_array );

/*---*/

MX_API mx_status_type mx_array_add_overlay( void *vector_pointer,
					long num_dimensions,
					long *dimension_array,
					size_t *data_element_size_array,
					void **array_pointer );

MX_API mx_status_type mx_array_free_overlay( void *array_pointer );

/*---*/

MX_API mx_status_type mx_subarray_add_overlay( void *array_pointer,
					long num_dimensions,
					long *range_array,
					size_t *data_element_size_array,
					void **subarray_pointer );

MX_API mx_status_type mx_subarray_free_overlay( void *subarray_pointer );

/*---*/

MX_API void *mx_array_get_vector( void *array_pointer );

/*---*/

MX_API void *mx_allocate_array( long num_dimensions,
					long *dimension_array,
					size_t *data_element_size_array );

MX_API mx_status_type mx_free_array( void *array_pointer );

/*---*/

MX_API void *mx_reallocate_array( void *array_pointer,
					long num_dimensions,
					long *dimension_array );

/*---*/

MX_API void mx_show_array_info( void *array_pointer );

/*---*/

MX_API mx_status_type mx_copy_array_to_buffer( void *array_pointer,
		mx_bool_type array_is_dynamically_allocated,
		long mx_datatype, long num_dimensions,
		long *dimension_array, size_t *data_element_size_array,
		void *destination_buffer, size_t destination_buffer_length,
		size_t *num_network_bytes_copied,
		mx_bool_type use_64bit_network_longs );

MX_API mx_status_type mx_copy_buffer_to_array(
		void *source_buffer, size_t source_buffer_length,
		void *array_pointer,
		mx_bool_type array_is_dynamically_allocated,
		long mx_datatype, long num_dimensions,
		long *dimension_array, size_t *data_element_size_array,
		size_t *num_native_bytes_copied,
		mx_bool_type use_64bit_network_longs );

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

