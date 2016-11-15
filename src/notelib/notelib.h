#ifndef NOTELIB_NOTELIB_H_
#define NOTELIB_NOTELIB_H_

#ifdef __cplusplus
extern "C" {
#endif//#ifdef __cplusplus

#include <stdbool.h>
#include <stdint.h>

enum notelib_status{
	notelib_status_ok = 0,
	notelib_answer_success = notelib_status_ok,

	notelib_answer_failure_unknown,

	notelib_answer_failure_bad_alloc,

	notelib_answer_failure_invalid_instrument,
	notelib_answer_failure_invalid_track,

	notelib_answer_failure_insufficient_initialized_channel_buffer_space,
	notelib_answer_failure_insufficient_command_queue_entries,

	notelib_status_not_ok = ~0
};

typedef void* notelib_state_handle;

typedef uint8_t  notelib_instrument_uint;
typedef uint16_t notelib_channel_uint;
typedef uint16_t notelib_note_id_uint;
typedef uint16_t notelib_instrument_state_uint;
typedef uint16_t notelib_step_uint;
typedef  int16_t notelib_sample;
typedef uint32_t notelib_sample_uint;
#define NOTELIB_SAMPLE_UINT_MAX UINT32_MAX
typedef uint8_t notelib_track_uint;
typedef uint16_t notelib_position; //uint32_t probably won't really make sense, but feel free to change it

struct notelib_params{
	notelib_instrument_uint instrument_count;
	uint8_t inline_step_count;
	uint16_t reserved_inline_state_space;
	uint16_t internal_dual_buffer_size;
	notelib_track_uint track_count;
	uint16_t queued_command_count;
	uint16_t reserved_inline_initialized_channel_buffer_size;
};

/*
enum notelib_status notelib_init
(notelib_state_handle* state_handle_dest,
 const struct notelib_params* params, void* backend_params);
enum notelib_status notelib_deinit(notelib_state_handle state_handle);
*/

typedef notelib_sample_uint (*notelib_processing_step_function)
        (notelib_sample* in, notelib_sample* out,
         notelib_sample_uint samples_requested,
         void* state);
typedef void                (*notelib_processing_step_setup_function)
        (void* trigger_data, void* state_data);
typedef void                (*notelib_processing_step_cleanup_function)
        (void* state);

struct notelib_processing_step_spec{
	notelib_processing_step_setup_function setup;
	notelib_instrument_state_uint trigger_data_offset;
	notelib_processing_step_function step;
	notelib_instrument_state_uint state_data_size;
	notelib_processing_step_cleanup_function cleanup;
	//possible future extension: notelib_alignment_uint (uint8_t, possibly encoding a po2) state_data_alignment;
};

struct notelib_instrument;
typedef struct notelib_instrument* notelib_instrument_handle;

typedef void (*notelib_trigger_function)(void* userdata);

enum notelib_status notelib_register_instrument
(notelib_state_handle notelib_state,
 notelib_instrument_uint* instrument_index_dest,
 notelib_step_uint step_count,
 const struct notelib_processing_step_spec* steps,
 notelib_channel_uint channel_count);
enum notelib_status notelib_set_instrument_channel_count
(notelib_state_handle notelib_state,
 notelib_instrument_uint instrument_index,
 notelib_channel_uint channel_count);
enum notelib_status notelib_unregister_instrument
(notelib_state_handle notelib_state,
 notelib_instrument_uint instrument_index);

enum notelib_status notelib_start_track
(notelib_state_handle notelib_state,
 notelib_track_uint* track_index_dest,
 uint32_t initialized_channel_buffer_size,
 notelib_position tempo_interval,
 notelib_sample_uint tempo_interval_samples);
enum notelib_status notelib_reset_track_position
(notelib_state_handle notelib_state,
 notelib_track_uint track_index, notelib_position position);
enum notelib_status notelib_set_track_tempo
(notelib_state_handle notelib_state,
 notelib_track_uint track_index, notelib_position position,
 notelib_position tempo_interval, notelib_sample_uint tempo_interval_samples);
enum notelib_status notelib_set_track_initialized_channel_buffer_size
(notelib_state_handle notelib_state,
 notelib_track_uint track_index,
 uint32_t initialized_channel_buffer_size);
enum notelib_status notelib_stop_track
(notelib_state_handle notelib_state,
 notelib_track_uint track_index);

enum notelib_status notelib_play
(notelib_state_handle notelib_state,
 notelib_instrument_uint instrument_index,
 void* trigger_data,
 notelib_track_uint track_index, notelib_position position);
enum notelib_status notelib_enqueue_trigger
(notelib_state_handle notelib_state,
 notelib_trigger_function trigger, void* userdata,
 notelib_track_uint track_index, notelib_position position);

/*notelib_step_uint notelib_instrument_get_step_count
(notelib_state_handle notelib_state, notelib_instrument_handle handle);*/

#include "internal/channel.h"

#define NOTELIB_SIZEOF_SINGLE_CHANNEL_STATE(CHANNEL_DATA_SIZE) NOTELIB_CHANNEL_SIZEOF_SINGLE(CHANNEL_DATA_SIZE)

#ifdef __cplusplus
}
#endif//#ifdef __cplusplus

#endif//#ifndef NOTELIB_NOTELIB_H_
