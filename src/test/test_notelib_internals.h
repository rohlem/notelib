#ifndef TEST_TEST_NOTELIB_INTERNALS_H_
#define TEST_TEST_NOTELIB_INTERNALS_H_

//#define NOTELIB_INTERNAL_USE_INTERMEDIATE_MIXING_BUFFER 0

#include "../notelib/notelib.h"
#include "../notelib/internal.h"

#include <stdlib.h>
#include <stdio.h>

#define TEST_TEST_NOTELIB_SAMPLE_FORMAT_CODE "%f"

struct notelib_params empty_params = {
	.instrument_count = 0,
	.inline_step_count = 0,
	.reserved_inline_state_space = 0,
	.internal_dual_buffer_size = 0,
#ifndef NOTELIB_NO_IMMEDIATE_TRACK
	.queued_immediate_command_count = 0,
	.reserved_inline_immediate_initialized_channel_buffer_size = 0,
	.initial_immediate_initialized_channel_buffer_size = 0,
#endif//#ifndef NOTELIB_NO_IMMEDIATE_TRACK
	.regular_track_count = 0,
	.queued_command_count = 0,
	.reserved_inline_initialized_channel_buffer_size = 0
};

struct notelib_params minimal_params = {
	.instrument_count = 1,
	.inline_step_count = 0,
	.reserved_inline_state_space = 0,
	.internal_dual_buffer_size = 1,
#ifndef NOTELIB_NO_IMMEDIATE_TRACK
	.queued_immediate_command_count = 0,
	.reserved_inline_immediate_initialized_channel_buffer_size = 0,
	.initial_immediate_initialized_channel_buffer_size = 0,
#endif//#ifndef NOTELIB_NO_IMMEDIATE_TRACK
	.regular_track_count = 1,
	.queued_command_count = 1,
	.reserved_inline_initialized_channel_buffer_size = 0
};

struct notelib_params minimal_test_params = {
	.instrument_count = 1,
	.inline_step_count = 0,
	.reserved_inline_state_space = 0,
	.internal_dual_buffer_size = 1,
#ifndef NOTELIB_NO_IMMEDIATE_TRACK
	.queued_immediate_command_count = 0,
	.reserved_inline_immediate_initialized_channel_buffer_size = 0,
	.initial_immediate_initialized_channel_buffer_size = 0,
#endif//#ifndef NOTELIB_NO_IMMEDIATE_TRACK
	.regular_track_count = 1,
	.queued_command_count = 2+1,
	.reserved_inline_initialized_channel_buffer_size = 0
};

struct notelib_params small_params = {
	.instrument_count = 1,
	.inline_step_count = 4,
	.reserved_inline_state_space = 2*NOTELIB_CHANNEL_SIZEOF_SINGLE(16),
	.internal_dual_buffer_size = 128,
#ifndef NOTELIB_NO_IMMEDIATE_TRACK
	.queued_immediate_command_count = 4,
	.reserved_inline_immediate_initialized_channel_buffer_size = 2*NOTELIB_CHANNEL_SIZEOF_SINGLE(16),
	.initial_immediate_initialized_channel_buffer_size = 0,
#endif//#ifndef NOTELIB_NO_IMMEDIATE_TRACK
	.regular_track_count = 1,
	.queued_command_count = 16,
	.reserved_inline_initialized_channel_buffer_size = 2*NOTELIB_CHANNEL_SIZEOF_SINGLE(16)
};

struct notelib_params standard_params = {
	.instrument_count = 1,
	.inline_step_count = 4,
	.reserved_inline_state_space = 8*NOTELIB_CHANNEL_SIZEOF_SINGLE(16),
	.internal_dual_buffer_size = 256,
#ifndef NOTELIB_NO_IMMEDIATE_TRACK
	.queued_immediate_command_count = 16,
	.reserved_inline_immediate_initialized_channel_buffer_size = 4*NOTELIB_CHANNEL_SIZEOF_SINGLE(16),
	.initial_immediate_initialized_channel_buffer_size = 0,
#endif//#ifndef NOTELIB_NO_IMMEDIATE_TRACK
	.regular_track_count = 1,
	.queued_command_count = 16,
	.reserved_inline_initialized_channel_buffer_size = 4*NOTELIB_CHANNEL_SIZEOF_SINGLE(16)
};

void test_notelib_internals_sizes(const struct notelib_params* params){
	printf("sizeof  (struct notelib_internals):             %d\n", (int)sizeof(struct notelib_internals));
	printf("offsetof(instruments):                          %d\n", (int)notelib_internals_offsetof_instruments);
	size_t sizeof_instrument = notelib_internals_sizeof_instrument(params->inline_step_count, params->reserved_inline_state_space);
	size_t instrument_count = params->instrument_count;
	printf(" sizeof  (struct notelib_instrument):            %d * %d = %d\n", (int)sizeof_instrument, (int)instrument_count, (int)(sizeof_instrument*instrument_count));
	printf("  sizeof (struct notelib_processing_step_entry): %d\n", (int)sizeof(struct notelib_processing_step_entry));
	printf("  sizeof (processing step entries):              %d * %d = %d (min %d)\n", (int)params->inline_step_count, (int)sizeof(struct notelib_processing_step_entry), (int)notelib_instrument_get_inline_steps_size(params->inline_step_count), (int)sizeof(void*));
	printf("  sizeof (reserved inline state space):          %d          (min %d)\n", (int)params->reserved_inline_state_space, (int)sizeof(void*));
	printf("offsetof(dual_audio_buffers):                   %d\n", (int)notelib_internals_offsetof_dual_audio_buffer(params->instrument_count, sizeof_instrument));
	printf(" sizeof (dual_audio_buffers):                   %d * %d * %d = %d\n", (int)(1 + NOTELIB_INTERNAL_USE_INTERMEDIATE_MIXING_BUFFER), (int)params->internal_dual_buffer_size, (int)sizeof(notelib_sample), (int)notelib_internals_sizeof_dual_audio_buffer(params->internal_dual_buffer_size));
#ifndef NOTELIB_NO_IMMEDIATE_TRACK
	printf("offsetof(struct notelib_track_immediate):       %d\n",  (int)notelib_internals_offsetof_track_immediate(params->instrument_count, sizeof_instrument, params->internal_dual_buffer_size));
	const size_t queued_immediate_command_count = params->queued_immediate_command_count+1;
	const size_t inline_immediate_initialized_channel_buffer_size = params->reserved_inline_immediate_initialized_channel_buffer_size+(params->reserved_inline_immediate_initialized_channel_buffer_size>0);
	printf(" sizeof  (struct notelib_track_immediate):       %d\n", (int)notelib_internals_sizeof_track_immediate(queued_immediate_command_count, inline_immediate_initialized_channel_buffer_size));
	printf("  sizeof (members):                              %d\n", (int)sizeof(struct notelib_track_immediate));
	printf("  sizeof (immediate_command_queue):              %d\n", (int)notelib_internals_sizeof_track_immediate_command_queue(queued_immediate_command_count));
	printf("  sizeof (inline_immediate_i._channel_buffer):   %d\n", (int)notelib_internals_sizeof_track_initialized_channel_buffer(inline_immediate_initialized_channel_buffer_size));
#endif//#ifndef NOTELIB_NO_IMMEDIATE_TRACK
	printf("offsetof(struct notelib_track):                 %d\n",  (int)notelib_internals_offsetof_regular_tracks(params->instrument_count, sizeof_instrument, params->internal_dual_buffer_size
#ifndef NOTELIB_NO_IMMEDIATE_TRACK
	  , queued_immediate_command_count, inline_immediate_initialized_channel_buffer_size
#endif//#ifndef NOTELIB_NO_IMMEDIATE_TRACK
	 ));
	const size_t rtc = params->regular_track_count;
	const size_t queued_command_count = params->queued_command_count+1;
	const size_t inline_initialized_channel_buffer_size = params->reserved_inline_initialized_channel_buffer_size + (params->reserved_inline_initialized_channel_buffer_size>0);
	const size_t sizeof_track = notelib_internals_sizeof_track(queued_command_count, inline_initialized_channel_buffer_size);
	printf(" sizeof  (tracks):                                %d * %d = %d\n", (int)rtc, (int)sizeof_track, (int)(rtc*sizeof_track));
	printf("  sizeof (members):                               %d\n", (int)sizeof(struct notelib_track));
	printf("  sizeof (command_queue):                         %d\n", (int)notelib_internals_sizeof_track_command_queue(queued_command_count));
	printf("  sizeof (inline_initialized_channel_buffer):  %d\n", (int)notelib_internals_sizeof_track_initialized_channel_buffer(inline_initialized_channel_buffer_size));
	printf("total size requirement: %d\n\n", (int)notelib_internals_size_requirements(params));
	fflush(stdout);
}

void test_notelib_internals_size_requirements(){
	printf("notelib_internals empty        size: %d\n", (int)notelib_internals_size_requirements(&        empty_params));
	test_notelib_internals_sizes                                                                (&        empty_params);
	printf("notelib_internals minimal      size: %d\n", (int)notelib_internals_size_requirements(&      minimal_params));
	test_notelib_internals_sizes                                                                (&      minimal_params);
	printf("notelib_internals minimal test size: %d\n", (int)notelib_internals_size_requirements(& minimal_test_params));
	test_notelib_internals_sizes                                                                (& minimal_test_params);
	printf("notelib_internals small        size: %d\n", (int)notelib_internals_size_requirements(&        small_params));
	test_notelib_internals_sizes                                                                (&        small_params);
	printf("notelib_internals standard     size: %d\n", (int)notelib_internals_size_requirements(&     standard_params));
	test_notelib_internals_sizes                                                                (&     standard_params);
}

enum {notelib_buffer_size = 0x1000};
unsigned char notelib_buffer[notelib_buffer_size];
notelib_track_uint only_track_index;
notelib_position only_track_tempo_interval;
notelib_sample_uint only_track_tempo_interval_samples;

notelib_position only_track_next_position = 0;
notelib_position only_track_position = 0;
notelib_sample_uint only_track_sample_offset = 0;

enum {little_sample_buffer_size = 0x80};
notelib_sample little_sample_buffer[little_sample_buffer_size];

enum notelib_status test_notelib_init(const struct notelib_params* params){
	size_t required_size = notelib_internals_size_requirements(params);
	if(notelib_buffer_size < required_size){
		printf("Buffer too small!\nsize: %d ; required: %d\n", (int)notelib_buffer_size, (int)required_size);
		return EXIT_FAILURE;
	}
	return notelib_internals_init(notelib_buffer, notelib_buffer_size, params);
}

struct sample_stair_setup_data{
	notelib_sample start;
	notelib_sample step_height;
	notelib_sample_uint step_width;
	notelib_sample_uint lifetime;
};

struct sample_stair_state_data{
	notelib_sample value;
	notelib_sample_uint position;
	notelib_sample step_height;
	notelib_sample_uint step_width;
	notelib_sample_uint ttl;
};

void sample_stair_setup(void* trigger_data, void* state_data){
	struct sample_stair_setup_data* trigger = ((struct sample_stair_setup_data*)trigger_data);
	struct sample_stair_state_data* start_state = ((struct sample_stair_state_data*)state_data);
	start_state->value = trigger->start;
	start_state->position = 0;
	start_state->step_height = trigger->step_height;
	start_state->step_width = trigger->step_width;
	start_state->ttl = trigger->lifetime;
}

notelib_sample_uint sample_stair_step(notelib_sample* in, notelib_sample* out, notelib_sample_uint samples_requested, void* state){
	(void)in;
	notelib_sample_uint i = 0;
	struct sample_stair_state_data* stair_state = state;
	if(stair_state->ttl > 0){
		while(i < samples_requested){
			out[i] = stair_state->value;
			++i;
			++stair_state->position;
			--stair_state->ttl;
		if(stair_state->ttl <= 0) break;
			if(stair_state->position == stair_state->step_width){
				stair_state->position = 0;
				printf("CHANGING VALUE; "TEST_TEST_NOTELIB_SAMPLE_FORMAT_CODE" ", stair_state->value);
				stair_state->value += stair_state->step_height;
				printf("-> "TEST_TEST_NOTELIB_SAMPLE_FORMAT_CODE"\n", stair_state->value);
			}
		}
	}
	printf("STEP RET: %d\n", (int)i);
	printf("STEP REQ. %d ; %d\n", (int)stair_state->position, (int)stair_state->value);
	return i;
}

void sample_stair_cleanup(void* state){
	struct sample_stair_state_data* data = state;
	printf(" Cleaned up stair channel;\n  final position: %d\n  final value: "TEST_TEST_NOTELIB_SAMPLE_FORMAT_CODE"\n", data->position, data->value);
}

notelib_instrument_uint sample_stair_instrument_index;

int test_notelib_instrument_registration(){
	struct notelib_processing_step_spec sample_stair_step_spec = {
		.setup = &sample_stair_setup,
		.step = &sample_stair_step,
		.cleanup = &sample_stair_cleanup,
		.trigger_data_offset = 0,
		.state_data_size = sizeof(struct sample_stair_state_data)
	};
	return notelib_register_instrument(notelib_buffer, &sample_stair_instrument_index, 1, &sample_stair_step_spec, 2);
}

enum notelib_status test_notelib_track_start(){
	only_track_tempo_interval = 4;
	only_track_tempo_interval_samples = 4;
	return notelib_start_track(notelib_buffer, &only_track_index, 2*NOTELIB_CHANNEL_SIZEOF_SINGLE(sizeof(struct sample_stair_state_data))+1, only_track_tempo_interval, only_track_tempo_interval_samples);
}

void test_notelib_fill_buffer(){
	notelib_internals_fill_buffer((void*)notelib_buffer, little_sample_buffer, little_sample_buffer_size);
	if(only_track_tempo_interval_samples != 0 && only_track_tempo_interval != 0){
		only_track_sample_offset += little_sample_buffer_size;
		only_track_position += (only_track_sample_offset/only_track_tempo_interval_samples)*only_track_tempo_interval;
		only_track_sample_offset %= only_track_tempo_interval_samples;
		only_track_next_position = only_track_position + (only_track_sample_offset > 0);
	}
	printf("buffer contents:\\");
	for(int i = 0; i < little_sample_buffer_size; ++i)
		printf(" '%d", (int)little_sample_buffer[i]);
	printf("\n");
	fflush(stdout);
}

enum notelib_status test_notelib_instrument_play(){
	struct sample_stair_setup_data setup_data = {
		.start = -20,
		.step_height = 4,
		.step_width = 3,
		.lifetime = 88
	};
	return notelib_play(notelib_buffer, sample_stair_instrument_index, &setup_data, only_track_index, only_track_next_position, NULL);
}

enum notelib_status test_notelib_instrument_concurrent_play(){
	enum notelib_status status = test_notelib_instrument_play();
	if(status != notelib_status_ok)
		return status;
//	printf("HALFWAY\n");
	struct sample_stair_setup_data setup_data = {
		.start = 30,
		.step_height = -2,
		.step_width = 1,
		.lifetime = 0x55
	};
	return notelib_play(notelib_buffer, sample_stair_instrument_index, &setup_data, only_track_index, only_track_next_position, NULL);
}

enum notelib_status test_notelib_track_stop(){
	only_track_tempo_interval = 0;
	only_track_tempo_interval_samples = 0;
	return notelib_stop_track (notelib_buffer, only_track_index);
}

enum notelib_status test_notelib_instrument_unregistration()
	{return notelib_unregister_instrument(notelib_buffer, sample_stair_instrument_index);}

enum notelib_status test_notelib_deinit()
	{return notelib_internals_deinit(notelib_buffer);}

int handle_notelib_status(enum notelib_status status, const char* failtext, const char* succtext){
	switch(status){
	case notelib_status_not_ok:
		printf("Something went horribly wrong!\n");
		fflush(stdout);
		return EXIT_FAILURE;
	case notelib_answer_failure_bad_alloc:
		puts("!BAD ALLOC!\n");
		goto case_failure;
	case notelib_answer_failure_no_tracks:
		puts("!NO TRACKS REQUESTED!");
		goto case_failure;
	case notelib_answer_failure_insufficient_command_queue_entries:
		puts("!INSUFFICIENT COMMAND QUEUE ENTRIES!\n");
		goto case_failure;
	case notelib_answer_failure_insufficient_initialized_channel_buffer_space:
		puts("!INSUFFICIENT INITIALIZED CHANNEL BUFFER SPACE!\n");
		goto case_failure;
	case notelib_answer_failure_invalid_instrument:
		puts("!INVALID INSTRUMENT!\n");
		goto case_failure;
	case notelib_answer_failure_invalid_track:
		puts("!INVALID TRACK!\n");
		goto case_failure;
	case notelib_answer_failure_invalid_track_parameters_stopped:
		puts("!INVALID TRACK PARAMETER (STOPPED)!\n");
		goto case_failure;
	case notelib_answer_failure_no_track_available:
		puts("!NO FREE TRACK AVAILABLE!\n");
		goto case_failure;
	case notelib_answer_failure_unknown:
		puts("!UNKNOWN!\n");
		goto case_failure;
	default:
		puts("! ???? !\n");
		goto case_failure;
	case_failure:
		printf(failtext);
		fflush(stdout);
		return EXIT_FAILURE;
	case notelib_answer_success:
		printf(succtext);
		fflush(stdout);
		return EXIT_SUCCESS;
	}
}

int test_notelib_internals(){
	test_notelib_internals_size_requirements();
	const struct notelib_params* params = &standard_params;
	printf("\nChosen:\n");
	test_notelib_internals_sizes(params);
	int ret = handle_notelib_status(test_notelib_init(params), "Failed to init notelib!\n", "Notelib initialized successfully!\n");
	if(ret != EXIT_SUCCESS)
		return ret;
	else{
		test_notelib_fill_buffer();
		ret = handle_notelib_status(test_notelib_track_start(), "Failed to start a track!\n", "Successfully started a track!\n");
		if(ret != EXIT_SUCCESS)
			goto deinit;
		ret = handle_notelib_status(test_notelib_instrument_registration(), "Failed to register an instrument!\n", "Successfully registered an instrument!\n");
		if(ret != EXIT_SUCCESS)
			goto deinit;
		ret = handle_notelib_status(test_notelib_instrument_play(), "Failed to play!\n", "Successfully played!\n");
		if(ret != EXIT_SUCCESS)
			goto deinit;
		test_notelib_fill_buffer();
		ret = handle_notelib_status(test_notelib_instrument_concurrent_play(), "Failed to play concurrent notes!\n", "Successfully played concurrent notes!\n");
		if(ret != EXIT_SUCCESS)
			goto deinit;
		test_notelib_fill_buffer();
		ret = handle_notelib_status(test_notelib_track_stop (), "Failed to stop  a track!\n", "Successfully stopped a track!\n");
		if(ret != EXIT_SUCCESS)
			goto deinit;
		ret = handle_notelib_status(test_notelib_instrument_unregistration(), "Failed to unregister sample_stair instrument!\n", "Successfully unregistered sample_stairs instrument!\n");
		if(ret != EXIT_SUCCESS)
			goto deinit;
	}
	deinit:;
		int deinit_ret = handle_notelib_status(test_notelib_deinit(), "Failed to deinit notelib!\n", "Notelib deinitialized successfully!\n");
		if(ret == EXIT_SUCCESS)
			return deinit_ret;
		else
			return ret;
		return EXIT_SUCCESS;
}

#endif//#ifndef TEST_TEST_NOTELIB_INTERNALS_H_
