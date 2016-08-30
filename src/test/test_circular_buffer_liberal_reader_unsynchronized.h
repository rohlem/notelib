#ifndef TEST_TEST_CIRCULAR_BUFFER_LIBERAL_READER_UNSYNCHRONIZED_H_
#define TEST_TEST_CIRCULAR_BUFFER_LIBERAL_READER_UNSYNCHRONIZED_H_

#include "../notelib/util/circular_buffer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* hi_data = "Hi! Here's some more test data!";

enum {hi_extra_size = 7};
enum {hi_buffer_size = sizeof(struct circular_buffer) + hi_extra_size};
unsigned char hi_buffer[hi_buffer_size];

bool temp_reading_write(struct circular_buffer_liberal_reader_unsynchronized* queue, int size, char* dest, int* num){
	if(*num >= size){
		dest[size] = '\0';
		circular_buffer_liberal_reader_unsynchronized_read(queue, size, dest);
		printf("Read %d: \"%s\"\n", size, dest);
		*num -= size;
		if(strlen(dest) != size){
			printf("Finished reading!\n");
			return true;
		}
	}
	return false;
}

//tests writing and reading characters (sizeof which is 1)
int test_circular_buffer_liberal_reader_unsynchronized_part_1(struct circular_buffer_liberal_reader_unsynchronized* hi_queue, char* i_buffer){
	const char* i = hi_data;
	int num;
	while(true){
		num = 0;
		while(circular_buffer_liberal_reader_unsynchronized_write(hi_queue, i, 1)){
			++num;
			printf("Wrote '%c'!\n", *i);
			if(*i == '\0'){
				printf("Finished writing!\n");
				goto break_2_a;
			}
			++i;
		}
		printf("Couldn't write '%c'!\n", *i);
	break_2_a:;
		if(temp_reading_write(hi_queue, 6, i_buffer, &num))
			break;
		if(temp_reading_write(hi_queue, 3, i_buffer, &num))
			break;
		if(temp_reading_write(hi_queue, 2, i_buffer, &num))
			break;
		if(temp_reading_write(hi_queue, 1, i_buffer, &num))
			break;
	}
	printf("Finished part 1!\n");
	return EXIT_SUCCESS;
}

//tests writing a sequence of characters (at a shifted position to trigger wrapping), because that unexpectedly failed
int test_circular_buffer_liberal_reader_unsynchronized_part_2(struct circular_buffer_liberal_reader_unsynchronized* hi_queue, char* i_buffer){
	const char* encore = "1more";
	if(!circular_buffer_liberal_reader_unsynchronized_write(hi_queue, encore, 6))
		return EXIT_FAILURE;
	printf("Wrote \"%s\"!\n", encore);
	i_buffer[6] = '\0';
	circular_buffer_liberal_reader_unsynchronized_read(hi_queue, 6, i_buffer);
	printf("Read \"%s\"!\n", i_buffer);
	printf("Finished part 2!\n");
	return EXIT_SUCCESS;
}

//tests unwriting both succeeding and failing
int test_circular_buffer_liberal_reader_unsynchronized_part_3(struct circular_buffer_liberal_reader_unsynchronized* hi_queue, char* i_buffer){
	const char* encore = "2m0r3";
	if(!circular_buffer_liberal_reader_unsynchronized_write(hi_queue, encore, 6))
		return EXIT_FAILURE;
	printf("Wrote \"%s\"!\n", encore);
	if(!circular_buffer_liberal_reader_unsynchronized_unwrite(hi_queue, 6))
		return EXIT_FAILURE;
	printf("Unwrote 6!\n");
	if(!circular_buffer_liberal_reader_unsynchronized_write(hi_queue, encore, 6))
		return EXIT_FAILURE;
	printf("Wrote \"%s\" again!\n", encore);
	circular_buffer_liberal_reader_unsynchronized_read(hi_queue, 6, i_buffer);
	i_buffer[6] = '\0';
	printf("Read \"%s\"!\n", i_buffer);
	if(circular_buffer_liberal_reader_unsynchronized_unwrite(hi_queue, 6))
		return EXIT_FAILURE;
	printf("Successfully failed to unwrite 6 ... yay!\n");
	printf("Finished part 3!\n");
	return EXIT_SUCCESS;
}

int test_circular_buffer_liberal_reader_unsynchronized(){
	struct circular_buffer_liberal_reader_unsynchronized* hi_queue = circular_buffer_liberal_reader_unsynchronized_construct(hi_buffer, hi_buffer_size, hi_extra_size);
	if(hi_queue == NULL)
		return EXIT_FAILURE;
	char i_buffer[hi_extra_size];
	int ret = test_circular_buffer_liberal_reader_unsynchronized_part_1(hi_queue, i_buffer);
	if(ret != EXIT_SUCCESS)
		return ret;
	ret = test_circular_buffer_liberal_reader_unsynchronized_part_2(hi_queue, i_buffer);
	if(ret != EXIT_SUCCESS)
		return ret;
	ret = test_circular_buffer_liberal_reader_unsynchronized_part_3(hi_queue, i_buffer);
	if(ret != EXIT_SUCCESS)
		return ret;
	printf("Done!\n");
	return EXIT_SUCCESS;
}

#endif//#ifndef TEST_TEST_CIRCULAR_BUFFER_LIBERAL_READER_UNSYNCHRONIZED_H_
