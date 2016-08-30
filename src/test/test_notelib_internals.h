#ifndef TEST_TEST_NOTELIB_INTERNALS_H_
#define TEST_TEST_NOTELIB_INTERNALS_H_

//#define NOTELIB_INTERNAL_USE_INTERMEDIATE_MIXING_BUFFER 0

#include "../notelib/notelib.h"
#include "../notelib/internal.h"

#include <stdlib.h>
#include <stdio.h>

struct notelib_params empty_params = {
	.instrument_count = 0,
	.inline_step_count = 0,
	.reserved_inline_state_space = 0,
	.internal_dual_buffer_size = 0,
	.track_count = 0,
	.queued_command_count = 0,
	.reserved_inline_initialized_channel_buffer_size = 0
};

struct notelib_params minimal_params = {
	.instrument_count = 1,
	.inline_step_count = 0,
	.reserved_inline_state_space = 0,
	.internal_dual_buffer_size = 1,
	.track_count = 1,
	.queued_command_count = 1,
	.reserved_inline_initialized_channel_buffer_size = 0
};

struct notelib_params minimal_test_params = {
	.instrument_count = 1,
	.inline_step_count = 0,
	.reserved_inline_state_space = 0,
	.internal_dual_buffer_size = 1,
	.track_count = 1,
	.queued_command_count = 2,
	.reserved_inline_initialized_channel_buffer_size = 0
};

struct notelib_params small_params = {
	.instrument_count = 1,
	.inline_step_count = 4,
	.reserved_inline_state_space = 32,
	.internal_dual_buffer_size = 128,
	.track_count = 1,
	.queued_command_count = 16,
	.reserved_inline_initialized_channel_buffer_size = 32
};

struct notelib_params standard_params = {
	.instrument_count = 1,
	.inline_step_count = 4,
	.reserved_inline_state_space = 128,
	.internal_dual_buffer_size = 256,
	.track_count = 1,
	.queued_command_count = 16,
	.reserved_inline_initialized_channel_buffer_size = 64
};

void test_notelib_internals_size_requirements(){
	printf("notelib_internals empty        size: %d\n", (int)notelib_internals_size_requirements(&        empty_params));
	printf("notelib_internals minimal      size: %d\n", (int)notelib_internals_size_requirements(&      minimal_params));
	printf("notelib_internals minimal test size: %d\n", (int)notelib_internals_size_requirements(& minimal_test_params));
	printf("notelib_internals small        size: %d\n", (int)notelib_internals_size_requirements(&        small_params));
	printf("notelib_internals standard     size: %d\n", (int)notelib_internals_size_requirements(&     standard_params));
}

void test_notelib_internals_sizes(const struct notelib_params* params){
	printf("sizeof  (struct notelib_internals):             %d\n", (int)sizeof(struct notelib_internals));
	printf("offsetof(instruments):                          %d\n", (int)notelib_internals_offsetof_instruments);
	printf(" sizeof  (struct notelib_processing_step_entry): %d\n", (int)sizeof(struct notelib_processing_step_entry));
	printf(" sizeof  (processing step entries):              %d * %d = %d\n", (int)params->inline_step_count, (int)sizeof(struct notelib_processing_step_entry), (int)notelib_instrument_get_inline_steps_size(params->inline_step_count));
	printf(" sizeof  (reserved inline state space):          %d\n", (int)params->reserved_inline_state_space);
	size_t sizeof_instrument = notelib_internals_sizeof_instrument(params->inline_step_count, params->reserved_inline_state_space);
	printf("sizeof  (struct notelib_instrument):            %d\n", (int)sizeof_instrument);
	printf("offsetof(dual_audio_buffers):                   %d\n", (int)notelib_internals_offsetof_dual_audio_buffer(params->instrument_count, sizeof_instrument));
	printf("sizeof  (dual_audio_buffers):                   %d * %d * %d = %d\n", (int)(1 + NOTELIB_INTERNAL_USE_INTERMEDIATE_MIXING_BUFFER), (int)params->internal_dual_buffer_size, (int)sizeof(notelib_sample), notelib_internals_sizeof_dual_audio_buffer(params->internal_dual_buffer_size));
	printf("offsetof(struct notelib_track):                 %d\n", (int)notelib_internals_offsetof_tracks(params->instrument_count, sizeof_instrument, params->internal_dual_buffer_size));
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
				printf("CHANGING VALUE; %d ", stair_state->value);
				stair_state->value += stair_state->step_height;
				printf("-> %d\n", stair_state->value);
			}
		}
	}
	printf("STEP RET: %d\n", (int)i);
	printf("STEP REQ. %d ; %d\n", (int)stair_state->position, (int)stair_state->value);
	return i;
}

void sample_stair_cleanup(void* state){
	struct sample_stair_state_data* data = state;
	printf(" Cleaned up stair channel;\n  final position: %d\n  final value: %d\n", data->position, data->value);
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
	return notelib_start_track(notelib_buffer, &only_track_index, 64, only_track_tempo_interval, only_track_tempo_interval_samples);
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
	return notelib_play(notelib_buffer, sample_stair_instrument_index, &setup_data, only_track_index, only_track_next_position);
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
	return notelib_play(notelib_buffer, sample_stair_instrument_index, &setup_data, only_track_index, only_track_next_position);
}

enum notelib_status test_notelib_track_stop(){
	only_track_tempo_interval = 0;
	only_track_tempo_interval_samples = 0;
	return notelib_stop_track (notelib_buffer, only_track_index);
}

enum notelib_status test_notelib_instrument_unregistration()
	{return notelib_unregister_instrument(notelib_buffer, sample_stair_instrument_index);}

int handle_notelib_status(enum notelib_status status, const char* failtext, const char* succtext){
	if(status == notelib_status_not_ok){
		printf("Something went horribly wrong!\n");
		fflush(stdout);
		return EXIT_FAILURE;
	}else if(status == notelib_answer_failure_unknown){
		printf(failtext);
		fflush(stdout);
		return EXIT_FAILURE;
	}else{
		printf(succtext);
		fflush(stdout);
		return EXIT_SUCCESS;
	}
}

int test_notelib_internals(){
	test_notelib_internals_size_requirements();
	test_notelib_internals_sizes(&minimal_test_params);
	int ret = handle_notelib_status(test_notelib_init(&minimal_test_params), "Failed to init notelib!\n", "Notelib initialized successfully!\n");
	if(ret != EXIT_SUCCESS)
		return ret;
	ret = handle_notelib_status(test_notelib_instrument_registration(), "Failed to   register sample_stair instrument!\n", "Successfully   registered sample_stairs instrument!\n");
	if(ret != EXIT_SUCCESS)
		return ret;
	test_notelib_fill_buffer();
	ret = handle_notelib_status(test_notelib_track_start(), "Failed to start a track!\n", "Successfully started a track!\n");
	if(ret != EXIT_SUCCESS)
		return ret;
	ret = handle_notelib_status(test_notelib_instrument_play(), "Failed to play!\n", "Successfully played!\n");
		if(ret != EXIT_SUCCESS)
			return ret;
	test_notelib_fill_buffer();
	ret = handle_notelib_status(test_notelib_instrument_concurrent_play(), "Failed to play concurrent notes!\n", "Successfully played concurrent notes!\n");
	if(ret != EXIT_SUCCESS)
		return ret;
	test_notelib_fill_buffer();
	ret = handle_notelib_status(test_notelib_track_stop (), "Failed to stop  a track!\n", "Successfully stopped a track!\n");
	if(ret != EXIT_SUCCESS)
		return ret;
	ret = handle_notelib_status(test_notelib_instrument_unregistration(), "Failed to unregister sample_stair instrument!\n", "Successfully unregistered sample_stairs instrument!\n");
	if(ret != EXIT_SUCCESS)
		return ret;
	ret = handle_notelib_status(test_notelib_instrument_unregistration(), "Failed to deinit notelib!\n", "Notelib deinitialized successfully!\n");
	if(ret != EXIT_SUCCESS)
		return ret;
	return EXIT_SUCCESS;
}

#endif//#ifndef TEST_TEST_NOTELIB_INTERNALS_H_
