#include "circular_buffer.h"

#include <stdatomic.h>
#include <stddef.h>
#include "stdint.h"
#include <stdio.h>
#include <string.h>

size_t circular_buffer_sizeof(uint16_t element_size, uint16_t element_count)
	{return circular_buffer_header_size + ((size_t)element_size)*element_count;}
size_t circular_buffer_dynamic_sizeof(const struct circular_buffer* object)
	{return circular_buffer_sizeof(object->element_size, object->element_count);}

size_t circular_buffer_liberal_reader_unsynchronized_sizeof(uint32_t data_size)
	{return circular_buffer_liberal_reader_unsynchronized_header_size + ((size_t)data_size);}
size_t circular_buffer_liberal_reader_unsynchronized_dynamic_sizeof(const struct circular_buffer_liberal_reader_unsynchronized* object)
	{return circular_buffer_liberal_reader_unsynchronized_sizeof(object->data_size);}

struct circular_buffer* circular_buffer_construct(void* position, uint32_t space, uint16_t element_size){
//	printf("CB_CONSTR:\n space: %d\n hsize: %d\n esize: %d\n", (int)space, (int)circular_buffer_header_size, (int)element_size);
	if(space < circular_buffer_header_size + ((uint32_t)element_size))
		return NULL;
	struct circular_buffer* buffer_ptr = position;
	atomic_store_explicit(&buffer_ptr->reader_position, 0, memory_order_relaxed);
	atomic_store_explicit(&buffer_ptr->writer_position, 0, memory_order_relaxed);
	buffer_ptr->element_size = element_size;
	size_t data_space = space - circular_buffer_header_size;
	if(data_space > ((uint32_t)element_size)*UINT16_MAX){
//		printf(" count: 0\n");
		buffer_ptr->element_count = 0;
	}else{
		uint16_t element_count = data_space/element_size;
//		printf(" count: %d\n", (int)element_count);
		buffer_ptr->element_count = element_count;
	}
	return buffer_ptr;
}
struct circular_buffer_liberal_reader_unsynchronized* circular_buffer_liberal_reader_unsynchronized_construct
(void* position, size_t space, uint32_t data_size){
//	printf("CBUCONSTR:\n space: %d\n hsize: %d\n dsize: %d\n", (int)space, (int)circular_buffer_liberal_reader_unsynchronized_header_size, (int)data_size);
	if(space < circular_buffer_header_size + data_size)
		return NULL;
	struct circular_buffer_liberal_reader_unsynchronized* buffer_ptr = position;
	atomic_store_explicit(&buffer_ptr->reader_position, 0, memory_order_relaxed);
	buffer_ptr->writer_position = 0;
	buffer_ptr->data_size = data_size;
	return buffer_ptr;
}

bool  circular_buffer_read(struct circular_buffer* object,       void* dest){
	_Atomic uint16_t* object_reader_position_addr = &object->reader_position;
	uint16_t reader_position = atomic_load_explicit(object_reader_position_addr, memory_order_relaxed);
	if(reader_position == atomic_load_explicit(&object->writer_position, memory_order_acquire))
		return false;

	size_t element_size = object->element_size;
	memcpy(dest, object->data + (element_size*reader_position), element_size);
	++reader_position;
	if(reader_position == object->element_count)
		reader_position = 0;
	atomic_store_explicit(object_reader_position_addr, reader_position, memory_order_release);
	return true;
}
void circular_buffer_liberal_reader_unsynchronized_read
(struct circular_buffer_liberal_reader_unsynchronized* object, uint32_t size, void* dest){
	_Atomic uint32_t* object_reader_position_addr = &object->reader_position;
	uint32_t reader_position = atomic_load_explicit(object_reader_position_addr, memory_order_relaxed);

	uint32_t unwrapped_size = object->data_size - reader_position;
	unsigned char* object_data = object->data;
	void* initial_copy_src = object_data + reader_position;
	uint32_t initial_copy_size;
	if(unwrapped_size > size){
		reader_position += size;
		initial_copy_size = size;
	}else if(unwrapped_size == size){
		reader_position = 0;
		initial_copy_size = size;
	}else{
		uint32_t remaining_size = size - unwrapped_size;
		reader_position = remaining_size;
		memcpy((void*)(((unsigned char*)dest) + unwrapped_size), object_data, remaining_size);
		initial_copy_size = unwrapped_size;
	}
	memcpy(dest, initial_copy_src, initial_copy_size);
	atomic_store_explicit(object_reader_position_addr, reader_position, memory_order_release);
}

const void* circular_buffer_direct_read_commence(const struct circular_buffer* object){
	const _Atomic uint16_t* object_reader_position_addr = &object->reader_position;
	uint16_t reader_position = atomic_load_explicit(object_reader_position_addr, memory_order_relaxed);
	uint16_t writer_position = atomic_load_explicit(&object->writer_position, memory_order_acquire);
//	printf("CMN\n reader: %d\n writer: %d\n", (int)reader_position, (int)writer_position);
	if(reader_position == writer_position)
		return NULL;
//	printf("CMN SUCC\n");
	return object->data + (((uint32_t)object->element_size)*reader_position);
}
void  circular_buffer_direct_read_commit(struct circular_buffer* object){
	_Atomic uint16_t* object_reader_position_addr = &object->reader_position;
	uint16_t reader_position_before = atomic_load_explicit(object_reader_position_addr, memory_order_relaxed) + 1;
	uint16_t reader_position = reader_position_before;
	if(reader_position == object->element_count)
		reader_position = 0;
//	printf("CMT\n before: %d\n after:  %d\n", (int)reader_position_before, (int)reader_position);
	atomic_store_explicit(object_reader_position_addr, reader_position, memory_order_release);
}

bool  circular_buffer_write(struct circular_buffer* object, const void* src){
	_Atomic uint16_t* object_writer_position_addr = &object->writer_position;
	uint16_t writer_position = atomic_load_explicit(object_writer_position_addr, memory_order_relaxed);
	uint16_t next_writer_position = writer_position + 1;
	if(next_writer_position == object->element_count)
		next_writer_position = 0;
	if(next_writer_position == atomic_load_explicit(&object->reader_position, memory_order_acquire))
		return false;

	size_t element_size = object->element_size;
	memcpy(object->data + (element_size*writer_position), src, element_size);
	atomic_store_explicit(object_writer_position_addr, next_writer_position, memory_order_release);
	return true;
}
bool circular_buffer_liberal_reader_unsynchronized_write
(struct circular_buffer_liberal_reader_unsynchronized* object, const void* src, uint32_t size){
	uint32_t reader_position = atomic_load_explicit(&object->reader_position, memory_order_acquire);
	uint32_t writer_position = object->writer_position;
	uint32_t data_size = object->data_size;
	uint32_t space = reader_position - writer_position;
//	printf("WRT:\n R: %d\n W: %d\n", (int)reader_position, (int)writer_position);
	if(reader_position <= writer_position) /* rewritable as ... + (r <= w)*data_size */
		space += data_size;
	if(space <= size) //also if == because reader == writer indicates nothing written
		return false;

	uint32_t unwrapped_size = data_size - writer_position;
	unsigned char* object_data = object->data;
	void* initial_copy_dest = object_data + writer_position;
	uint32_t initial_copy_size;
	if(unwrapped_size > size){
		writer_position += size;
		initial_copy_size = size;
	}else if(unwrapped_size == size){
		writer_position = 0;
		initial_copy_size = size;
	}else{
		uint32_t remaining_size = size - unwrapped_size;
		writer_position = remaining_size;
		memcpy(object_data, ((char*)src)+unwrapped_size, remaining_size);
//		printf("-Write #2: '%c'\n", *(((char*)src)+unwrapped_size));
		initial_copy_size = unwrapped_size;
	}
	memcpy(initial_copy_dest, src, initial_copy_size);
//	printf("-Write #1: '%c' to %d\n", *((char*)src), (int)(((unsigned char*)initial_copy_dest) - object_data));
	object->writer_position = writer_position;
	return true;
}
bool circular_buffer_liberal_reader_unsynchronized_unwrite
(struct circular_buffer_liberal_reader_unsynchronized* object, uint32_t size){
	uint32_t reader_position = atomic_load_explicit(&object->reader_position, memory_order_acquire);
	uint32_t writer_position = object->writer_position;
	uint32_t data_size = object->data_size;
	uint32_t unspace = writer_position - reader_position;
	if(writer_position < reader_position) /* rewritable as ... + (w < r)*data_size */
		unspace += data_size;
	if(unspace < size) //not if == because reader == writer indicates nothing written (which is acceptable state)
		return false;

//	printf("writer_position before unwrite: %d\n", writer_position);
	if(writer_position >= size)
		writer_position -= size;
	else
		writer_position = data_size - size + writer_position;
//	printf("writer_position after  unwrite: %d\n", writer_position);
	object->writer_position = writer_position;
	return true;
}
