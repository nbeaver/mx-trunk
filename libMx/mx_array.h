/*
 * Name:    mx_array.h
 *
 * Purpose: Header for MX array handling functions and address alignment macros.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2000, 2003, 2005-2007, 2012-2017, 2019, 2021-2022
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

#elif ( defined(_MSC_VER) || defined(__BORLANDC__) )
#  define MX_MAXIMUM_ALIGNMENT		8

#elif defined(__SUNPRO_C)
#  if defined(__sparcv9)
#    define MX_MAXIMUM_ALIGNMENT	16
#  elif defined(__x86_64)
#    define MX_MAXIMUM_ALIGNMENT	16
#  else
#    define MX_MAXIMUM_ALIGNMENT	8
#  endif

#elif defined(OS_VMS) || defined(OS_UNIXWARE)
#  define MX_MAXIMUM_ALIGNMENT		8

#else
#  error This platform does not yet implement the MX_MAXIMUM_ALIGNMENT macro.
#endif

/*---------------------------------------------------------------------------*/

/* MX_ALIGNED() can be used to define variables or function return values.
 * You cannot directly use it for function arguments.  Instead, you must
 * use a typedef that defines a new datatype that includes the alignment
 * and then specify the typedeffed type in the function argument signature.
 */

#if defined(__GNUC__)
#  define MX_ALIGNED(x,a)	( (x) __attribute__((__aligned__(a))) )

#elif defined(_MSC_VER)
#  define MX_ALIGNED(x,a)	( __declspec(align(a)) (x) )

#elif defined(__SUNPRO_C)
#  define MX_ALIGNED(x,a)	( (x) __attribute__((aligned(a))))

#elif defined(__USLC__)
#  define MX_ALIGNED(x,a)	(x)

#else
#  error This platform does not yet implement the MX_ALIGNED macro.
#endif

/*---------------------------------------------------------------------------*/

#define MX_ARRAY_HEADER_WORD_TYPE	uint32_t
#define MX_ARRAY_HEADER_WORD_SIZE	sizeof(MX_ARRAY_HEADER_WORD_TYPE)

#define MX_ARRAY_HEADER_MAGIC		0xd6d8ebed

#define MX_ARRAY_HEADER_BAD_MAGIC	0xdeadbeef

#define MX_ARRAY_OFFSET_MAGIC		(-1)
#define MX_ARRAY_OFFSET_HEADER_LENGTH	(-2)
#define MX_ARRAY_OFFSET_MX_DATATYPE	(-3)
#define MX_ARRAY_OFFSET_NUM_DIMENSIONS	(-4)
#define MX_ARRAY_OFFSET_DIMENSION_ARRAY	(-5)

/* The data element size array does not appear at a fixed offset in the header.
 * It is beyond the (varying size) dimension array, so its offset depends on
 * the number of dimensions in the array.
 */

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
				long mx_datatype,
				long num_dimensions,
				long *dimension_array,
				size_t *data_element_size_array );

MX_API mx_status_type mx_setup_array_header( void *array_pointer,
					unsigned long array_header_length,
					long mx_datatype,
					long num_dimensions,
					long *dimension_array,
					size_t *data_element_size_array );

/*---*/

/* The following calls get the numer of elements and the size of the
 * array from the array header.
 */

MX_API mx_status_type mx_array_get_num_elements( void *mx_array,
	       					unsigned long *num_elements );

MX_API mx_status_type mx_array_get_num_bytes( void *mx_array,
	       					unsigned long *num_bytes );

/*---*/

MX_API mx_status_type mx_array_add_overlay( void *vector_pointer,
					long mx_datatype,
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

MX_API void *mx_allocate_array( long mx_datatype,
				long num_dimensions,
				long *dimension_array,
				size_t *data_element_size_array );

MX_API mx_status_type mx_free_array( void *array_pointer );

/*---*/

MX_API void *mx_reallocate_array( void *array_pointer,
				long num_dimensions,
				long *dimension_array );

/*---*/

MX_API void mx_show_array_info( void *array_pointer );

MX_API mx_bool_type mx_array_is_mx_style_array( void *array_pointer );

MX_API void mx_array_dump( void *array_pointer );

/*---*/

MX_API mx_status_type mx_array_copy_vector( void *dest_vector,
					unsigned long dest_mx_datatype,
					size_t dest_max_bytes,
					void *src_vector,
					unsigned long src_mx_datatype,
					size_t src_max_bytes,
					size_t *num_bytes_copied,
					mx_bool_type do_byteswap );

/*---*/

MX_API mx_status_type mx_copy_array_to_network_buffer( void *array_pointer,
		mx_bool_type array_is_dynamically_allocated,
		long mx_datatype, long num_dimensions,
		long *dimension_array, size_t *data_element_size_array,
		void *destination_buffer, size_t destination_buffer_length,
		size_t *num_network_bytes_copied,
		mx_bool_type use_64bit_network_longs,
		unsigned long remote_mx_version );

MX_API mx_status_type mx_copy_network_buffer_to_array(
		void *source_buffer, size_t source_buffer_length,
		void *array_pointer,
		mx_bool_type array_is_dynamically_allocated,
		long mx_datatype, long num_dimensions,
		long *dimension_array, size_t *data_element_size_array,
		size_t *num_native_bytes_copied,
		mx_bool_type use_64bit_network_longs,
		unsigned long remote_mx_version );

/*---*/

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

/*---*/

MX_API mx_status_type mx_convert_and_copy_array_old(
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

/*---*/

MX_API mx_status_type mx_convert_and_copy_scaled_vector(
		void *source_vector_pointer,
		long source_datatype,
		long source_num_values,
		void *destination_vector_pointer,
		long destination_datatype,
		long destination_num_values,
		double scale,
		double offset,
		double minimum_value,
		double maximum_value );

/*---*/

MX_API mx_status_type mx_copy_mx_array_to_ascii_buffer(
		long mx_datatype,
		void *array_pointer,
		char *destination_ascii_buffer,
		size_t destination_ascii_buffer_length,
		size_t *num_values_copied );

MX_API mx_status_type mx_copy_ascii_buffer_to_mx_array(
		char *source_ascii_buffer,
		long maximum_string_token_length,
		long mx_datatype,
		void *array_pointer,
		size_t *num_values_copied );

#ifdef __cplusplus
}
#endif

#endif /* __MX_ARRAY_H__ */

