#ifndef NOTELIB_INTERNAL_INTERNALS_H_
#define NOTELIB_INTERNAL_INTERNALS_H_

#include "internals_fwd.h"

#include "alignment_utilities.h"
#include "circular_buffer.h"
#include "instrument.h"

struct notelib_internals{
	ALIGNAS_MAX(struct notelib_instrument, struct circular_buffer)
	notelib_instrument_uint instrument_count;
	uint8_t inline_step_count;
	uint16_t reserved_inline_state_space;
	uint16_t dual_buffer_size;
	notelib_track_uint track_count;
	uint16_t command_queue_size;
	uint16_t reserved_inline_initialized_channel_buffer_size;
	uint16_t instrument_size; //= cached notelib_internals_sizeof_instrument(...)
	//instruments: instrument_count*instrument_size
	//circular_buffer notelib_command_queue
};

static const size_t notelib_internals_offsetof_instruments = NOTELIB_INTERNAL_PAD_SIZEOF(struct notelib_internals, struct notelib_instrument);

size_t notelib_internals_offsetof_dual_audio_buffer(notelib_instrument_uint instrument_count, uint16_t instrument_size);

notelib_sample* notelib_internals_get_audio_buffer(struct notelib_internals* internals);

size_t notelib_internals_sizeof_dual_audio_buffer(uint16_t dual_buffer_size);

size_t notelib_internals_offsetof_tracks(notelib_instrument_uint instrument_count, uint16_t instrument_size, uint16_t dual_buffer_size);

struct notelib_track* notelib_internals_get_tracks(struct notelib_internals* internals);

bool notelib_track_is_initialized_channel_buffer_internal(const struct notelib_internals* internals, const struct notelib_track* track_ptr);

size_t notelib_internals_sizeof_track_command_queue(uint16_t queued_command_count);

size_t notelib_internals_sizeof_track_initialized_channel_buffer(uint16_t initialized_channel_buffer_size);

size_t notelib_internals_sizeof_track(uint16_t queued_command_count, uint16_t reserved_inline_initialized_channel_buffer_size);

struct notelib_track* notelib_internals_get_track(struct notelib_internals* internals, notelib_track_uint track_index);

size_t notelib_internals_size_requirements(const struct notelib_params* params);

#define MIN(A, B) ((A) < (B) ? (A) : (B))

enum notelib_status notelib_internals_init(void* position, size_t space_available, const struct notelib_params* params);

enum notelib_status notelib_internals_deinit(notelib_state_handle state_handle);

void notelib_internals_execute_instrument_steps
(void* channel_state_front, void* channel_state_back, notelib_instrument_state_uint channel_state_size,
 notelib_step_uint step_count, struct notelib_processing_step_entry* steps,
 notelib_sample* instrument_mix_buffer, notelib_sample* channel_mix_buffer, notelib_sample_uint samples_requested,
 notelib_channel_uint* active_channel_count_ptr);

void cautiously_advance_track_position
(struct notelib_track* track_ptr, notelib_position* new_position, notelib_sample_uint* new_sample_offset, notelib_position old_position, notelib_sample_uint old_position_sample_offset,
 bool* should_check_for_updates, notelib_sample_uint samples_to_process_commands_for);

void notelib_internals_fill_buffer_part(struct notelib_internals* internals, notelib_sample* out, notelib_sample_uint samples_requested, notelib_channel_uint* still_active_channel_count);

void notelib_internals_fill_buffer(struct notelib_internals* internals, notelib_sample* out, notelib_sample_uint samples_required);

#endif//#ifndef NOTELIB_INTERNAL_INTERNALS_H_
