#ifndef TEST_TEST_CIRCULAR_BUFFER_H_
#define TEST_TEST_CIRCULAR_BUFFER_H_

#include "../notelib/util/circular_buffer.h"

#include <stdio.h>
#include <stdlib.h>

const char* hello_data = "Hello! This is some test data.";

enum {hello_extra_size = 3};
enum {hello_buffer_size = sizeof(struct circular_buffer) + hello_extra_size};
unsigned char hello_buffer[hello_buffer_size];

int test_circular_buffer(){
	const char* i = hello_data;
	struct circular_buffer* hello_queue = circular_buffer_construct(hello_buffer, hello_buffer_size, 1);
	if(hello_queue == NULL)
		return EXIT_FAILURE;
	printf("element_count: %d\n", (int)hello_queue->element_count);
	while(true){
		while(circular_buffer_write(hello_queue, i)){
			printf("Wrote '%c'!\n", *i);
			if(*i == '\0'){
				printf("Finished writing!\n");
				goto break_2_a;
			}
			++i;
		}
		printf("Couldn't write '%c'!\n", *i);
	break_2_a:;
		char r;
		while(circular_buffer_read(hello_queue, &r)){
			printf("Read '%c'!\n", r);
			if(r == '\0'){
				printf("Finished reading!\n");
				goto break_2_b;
			}
			const char* try_direct_read = circular_buffer_direct_read_commence(hello_queue);
		if(try_direct_read == NULL) break;
			r = *try_direct_read;
			circular_buffer_direct_read_commit(hello_queue);
			printf("Directly read '%c'!\n", r);
			if(r == '\0'){
				printf("Finished reading!\n");
				goto break_2_b;
			}
		}
		printf("Couldn't read!\n");
	}
	break_2_b:
	printf("Done!\n");
	return EXIT_SUCCESS;
}

#endif//#ifndef TEST_TEST_CIRCULAR_BUFFER_H_
