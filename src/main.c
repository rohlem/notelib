#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#if 1

//#include "test/test_circular_buffer.h"
//#include "test/test_circular_buffer_liberal_reader_unsynchronized.h"
#include "test/test_notelib_internals.h"

int main(void)
	{return (test_notelib_internals() == EXIT_SUCCESS) ? EXIT_SUCCESS : EXIT_FAILURE;}
#else

#include "notelib/notelib.h"
#include "back.h"

#include <math.h>

/*
#ifndef M_PI
#define M_PI  3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117068l
#endif//#ifndef M_PI
*/
#ifndef M_TAU
#define M_TAU 6.283185307179586476925286766559005768394338798750211641949889184615632812572417997256069650684234136l
#endif//#ifndef M_TAU

struct limit_data{
	notelib_sample_uint lifespan;
};
#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define MIN(A, B) ((A) < (B) ? (A) : (B))
notelib_sample_uint limit_step(notelib_sample* in, notelib_sample* out, notelib_sample_uint samples_requested, void* state){
	struct limit_data* data = state;
	notelib_sample_uint lifespan = data->lifespan;
	notelib_sample_uint samples_generated;
	notelib_sample_uint dropoff = 256;
	notelib_sample_uint lifespan_before_dropoff = lifespan > dropoff ? lifespan - dropoff : 0;
	if(lifespan_before_dropoff >= samples_requested)
		samples_generated = samples_requested;
	else{
		notelib_sample_uint          lifespan_in_dropoff = MIN(dropoff, lifespan);
		notelib_sample_uint samples_requested_in_dropoff = samples_requested - lifespan_before_dropoff;
		notelib_sample_uint           samples_in_dropoff = MIN(lifespan_in_dropoff, samples_requested_in_dropoff);
		notelib_sample_uint lifespan_in_dropoff_start_offset = dropoff - lifespan_in_dropoff;
		for(notelib_sample_uint i = 0; i < samples_in_dropoff; ++i){
			notelib_sample_uint index = i + lifespan_before_dropoff;
			out[index] = (in[index]*(dropoff - (i+lifespan_in_dropoff_start_offset)))/dropoff;
		}
		samples_generated = MIN(lifespan, samples_requested);
	}
	data->lifespan -= samples_generated;
	return samples_generated;
}
notelib_sample_uint limit_step2(notelib_sample* in, notelib_sample* out, notelib_sample_uint samples_requested, void* state){
	(void)in;
	(void)out;
	struct limit_data* data = state;
	if(data->lifespan > samples_requested){
		data->lifespan -= samples_requested;
		return samples_requested;
	}else{
		notelib_sample_uint temp = data->lifespan;
		data->lifespan = 0;
		return temp;
	}
}
void limit_setup(void* trigger_data, void* state_data)
	{((struct limit_data*)state_data)->lifespan = ((struct limit_data*)trigger_data)->lifespan;}

struct sine_setup_data{
	notelib_sample amplitude;
	int pitch;
	notelib_sample_uint sample_rate;
};
struct sine_step_data{
	notelib_sample amplitude;
	notelib_sample_uint position;
	float frequency;
};
notelib_sample_uint sine_step(notelib_sample* in, notelib_sample* out, notelib_sample_uint samples_requested, void* state){
	(void)in;
	struct sine_step_data* data = state;
	notelib_sample_uint position = data->position;
	for(notelib_sample_uint i = 0; i < samples_requested; ++i){
		out[i] = (notelib_sample)(data->amplitude*sin(data->frequency*position));
		++position;
	}
	data->position = position;
//	printf("FREQ %f\namp %d\npos %d\n", (float)data->frequency, (int)data->amplitude, (int)data->position);
	return samples_requested;
}
void sine_setup(void* trigger_data, void* state_data){
	struct sine_setup_data* setup = trigger_data;
	struct sine_step_data* step = state_data;
	step->amplitude = setup->amplitude;
	step->frequency = (M_TAU * 440.0l*pow(2, setup->pitch/12.0l))/setup->sample_rate;
//	printf("F %f", step->frequency);
	step->position = 0;
}

/*
notelib_sample_uint step(notelib_sample* in, notelib_sample* out, notelib_sample_uint samples_requested, void* state);
void setup(void* trigger_data, void* state_data);
void cleanup(void* state);
*/

struct instrument_setup_data{
	struct limit_data limit;
	struct sine_setup_data sine;
};

notelib_instrument_uint instrument_id;
notelib_track_uint track_id;

bool play(struct instrument_setup_data note, notelib_position position){
	//notelib_note_id_uint note_id;
	enum notelib_status status = notelib_play(pa_back_notelib_handle, instrument_id, &note, track_id, position, NULL); //&note_id
	//printf("#%hu: %d\n", note_id, note.sine.pitch);
	if(status != notelib_status_ok){
		printf("Error playing note (pitch=%d)!\n", (int)note.sine.pitch);
		switch(status){
		case notelib_answer_failure_invalid_instrument:
			printf("~invalid instrument~\n");
			break;
		case notelib_answer_failure_invalid_track:
			printf("~invalid track~\n");
			break;
		case notelib_answer_failure_bad_alloc:
			printf("~bad alloc~\n");
			break;
		case notelib_answer_failure_insufficient_initialized_channel_buffer_space:
			printf("~initialized channel buffer too small~\n");
			break;
		case notelib_answer_failure_insufficient_command_queue_entries:
			printf("~command queue too small~\n");
			break;
		case notelib_answer_failure_unknown:
			printf("~failure \"unknown\"~\n");
			break;
		default:
			break;
		case notelib_status_not_ok:
			printf("~Something went horribly wrong!~\n");
			break;
		}
		return false;
	}
	return true;
}

notelib_position big_play(notelib_sample_uint samples_per_second, notelib_position start, unsigned int part){
	struct limit_data l8 = {.lifespan = samples_per_second/8};
	struct limit_data l4 = {.lifespan = samples_per_second/4};
	struct limit_data l2 = {.lifespan = samples_per_second/2};
	struct limit_data l1 = {.lifespan = samples_per_second/1};
	(void)l1;

	struct sine_setup_data s0 = {.sample_rate = samples_per_second, .amplitude = 0x800, .pitch = 0};
	struct sine_setup_data s1 = {.sample_rate = samples_per_second, .amplitude = 0x800, .pitch = 2};
	struct sine_setup_data s2 = {.sample_rate = samples_per_second, .amplitude = 0x800, .pitch = 4};
	struct sine_setup_data s3 = {.sample_rate = samples_per_second, .amplitude = 0x800, .pitch = 5};
	struct sine_setup_data s4 = {.sample_rate = samples_per_second, .amplitude = 0x800, .pitch = 7};
	struct sine_setup_data s5 = {.sample_rate = samples_per_second, .amplitude = 0x800, .pitch = 9};
	struct sine_setup_data s6 = {.sample_rate = samples_per_second, .amplitude = 0x800, .pitch = 11};
	struct sine_setup_data s7 = {.sample_rate = samples_per_second, .amplitude = 0x800, .pitch = 12};
	struct sine_setup_data s8 = {.sample_rate = samples_per_second, .amplitude = 0x800, .pitch = 14};

	switch(part){
	case  0:
	case  1:
	case  4:
	case  5:
	case 12:
	case 13:
		play((struct instrument_setup_data){l4, s4}, start+0);
		play((struct instrument_setup_data){l8, s0}, start+2);
		switch(part%4){
		case 0:
			play((struct instrument_setup_data){l4, s7}, start+5);
			play((struct instrument_setup_data){l8, s5}, start+7);
			break;
		case 1:
			play((struct instrument_setup_data){l4, s4}, start+5);
			play((struct instrument_setup_data){l8, s3}, start+7);
			break;
		}
		break;
	case  2:
	case  6:
	case 14:
		play((struct instrument_setup_data){l8, s2}, start+0);
		;//fallthrough
	case 15:
	case 16:
		play((struct instrument_setup_data){l8, s2}, start+1);
		play((struct instrument_setup_data){l8, s3}, start+2);
		play((struct instrument_setup_data){l8, s4}, start+3);
	if(part < 16){
		play((struct instrument_setup_data){l4, s0}, start+4);
		play((struct instrument_setup_data){l4, s1}, start+6);
	}else{
		play((struct instrument_setup_data){l4, s7}, start+4);
		play((struct instrument_setup_data){l4, s8}, start+6);
	}
		break;
	case 17:
		play((struct instrument_setup_data){l2, s7}, start+0);
		break;
	case  3:
		play((struct instrument_setup_data){l2, s2}, start+0);
		break;
	case  7:
		play((struct instrument_setup_data){l2, s0}, start+0);
		break;
	case 8:
		play((struct instrument_setup_data){l4, s6}, start+0);
		play((struct instrument_setup_data){l8, s2}, start+2);
		play((struct instrument_setup_data){l4, s7}, start+5);
		play((struct instrument_setup_data){l8, s6}, start+7);
		break;
	case 9:
		play((struct instrument_setup_data){l8, s6}, start+0);
		play((struct instrument_setup_data){l8, s5}, start+1);
		play((struct instrument_setup_data){l8, s5}, start+2);
		play((struct instrument_setup_data){l8, s6}, start+3);
		play((struct instrument_setup_data){l4, s5}, start+4);
		break;
	case 10:
		play((struct instrument_setup_data){l4, s5}, start+0);
		play((struct instrument_setup_data){l8, s1}, start+2);
		play((struct instrument_setup_data){l4, s6}, start+5);
		play((struct instrument_setup_data){l8, s5}, start+7);
		break;
	case 11:
		play((struct instrument_setup_data){l8, s5}, start+0);
		play((struct instrument_setup_data){l8, s4}, start+1);
		play((struct instrument_setup_data){l8, s4}, start+2);
		play((struct instrument_setup_data){l8, s5}, start+3);
		play((struct instrument_setup_data){l4, s4}, start+4);
		break;
	}
	return start+8;
}

struct gen_data {
	notelib_state_handle notelib_state;
	notelib_position position;
	notelib_sample_uint samples_per_second;
	unsigned int part;
} gen;

void gen_play(void* gen_data_ptr){
	struct gen_data* gen = (struct gen_data*) gen_data_ptr;
	unsigned int part = gen->part;
	gen->position = big_play(gen->samples_per_second, gen->position, part);
	if(part < 17){
		notelib_enqueue_trigger(gen->notelib_state, gen_play, gen_data_ptr, track_id, gen->position);
		++gen->part;
	}
}

struct combined_step_data{
	struct limit_data;
	struct sine_step_data;
};

#include "notelib/internal/internals.h"

int main(){
	//return test_circular_buffer();
	//return test_circular_buffer_liberal_reader_unsynchronized();
	//return test_notelib_internals();

	const size_t channel_state_size = NOTELIB_SIZEOF_SINGLE_CHANNEL_STATE(sizeof(struct combined_step_data));
	//printf("#RealExpense %d + %d => %d\n", (int)sizeof(struct limit_data), (int)sizeof(struct sine_step_data), (int)channel_state_size);

	struct notelib_params params = {
		.instrument_count = 1,
		.inline_step_count = 2,
		.reserved_inline_state_space = 30,
		.internal_dual_buffer_size = 576,
#ifndef NOTELIB_NO_IMMEDIATE_TRACK
		.queued_immediate_command_count = 0,
		.reserved_inline_immediate_initialized_channel_buffer_size = 0,
		.initial_immediate_initialized_channel_buffer_size = 0,
#endif//#ifndef NOTELIB_NO_IMMEDIATE_TRACK
		.regular_track_count = 1,
		.queued_command_count = 8,
		.reserved_inline_initialized_channel_buffer_size = 12*channel_state_size
	};

//	printf("size requirement: %X", (unsigned int)notelib_internals_size_requirements(&params));

	struct pa_back_init_data init_ret = pa_back_notelib_initialize(&params);
	switch(init_ret.error.error_type){
	case pa_back_error_notelib:
		printf("Aborting due to notelib error!\n");
		return EXIT_FAILURE;
	case pa_back_error_backend:
		printf("Aborting due to backend (PortAudio) error!\n");
		return EXIT_FAILURE;
	default:
		printf("Aborting due to unknown error (wtf?)!\n");
		return EXIT_FAILURE;
	case pa_back_error_none: break;
	}

	notelib_sample_uint samples_per_second = (notelib_sample_uint)(init_ret.sample_rate + 0.5); //http://stackoverflow.com/a/7563694 if you value non-integer sample rate preservation

	struct notelib_processing_step_spec instrument_steps[2] = {
		{
			.setup = sine_setup,
			.trigger_data_offset = offsetof(struct instrument_setup_data, sine),
			.step = sine_step,
			.state_data_size = sizeof(struct sine_step_data),
			.cleanup = NULL
		},
		{
			.setup = limit_setup,
			.trigger_data_offset = offsetof(struct instrument_setup_data, limit),
			.step = limit_step,
			.state_data_size = sizeof(struct limit_data),
			.cleanup = NULL
		}
	};
	enum notelib_status ns = notelib_register_instrument(pa_back_notelib_handle, &instrument_id, 2, instrument_steps, channel_state_size);
	if(ns != notelib_status_ok){
		printf("Failed to register instrument!\n");
		goto end;
	}


//	printf("SAMP %d\n", (int)samples_per_second);

	ns = notelib_start_track(pa_back_notelib_handle, &track_id, 12*channel_state_size, 8, samples_per_second);
	if(ns != notelib_status_ok){
		printf("Failed to start track!\n");
		goto end;
	}

/*	play((struct instrument_setup_data){l8, s0}, 3);
	play((struct instrument_setup_data){l8, s0}, 5);
	play((struct instrument_setup_data){l8, s0}, 8);
*/

	gen.notelib_state = pa_back_notelib_handle;
	gen.position = 10;
	gen.samples_per_second = samples_per_second;
	gen.part = 0;

	gen_play(&gen);

	system("pause");
end:;
	printf("Deinitializing...\n");
	struct pa_back_error deinit_error = pa_back_notelib_deinitialize();
	switch(deinit_error.error_type){
	case pa_back_error_notelib:
		printf("Notelib error when deinitializing!\n");
		return EXIT_FAILURE;
	case pa_back_error_backend:
		printf("Backend (PortAudio) error when deinitializing!\n");
		return EXIT_FAILURE;
	default:
		printf("Unknown error when deinitializing (wtf?)!\n");
		return EXIT_FAILURE;
	case pa_back_error_none: break;
	}
	return EXIT_SUCCESS;
}
#endif
