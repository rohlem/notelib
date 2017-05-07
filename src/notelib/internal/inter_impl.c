#include "../notelib.h"

#include "alignment_utilities.h"
#include "channel.h"
#include "circular_buffer.h"
#include "command.h"
#include "instrument.h"
#include "internals.h"
#include "processing_step_entry.h"
#include "track.h"

#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "../util/eclipse_codan_fix.hpp"

//could possibly optimize to single realloc call in certain cases
enum notelib_status notelib_register_instrument
(notelib_state_handle notelib_state,
 notelib_instrument_uint* instrument_index_dest,
 notelib_step_uint step_count,
 const struct notelib_processing_step_spec* steps,
 notelib_channel_uint channel_count){
	struct notelib_instrument* instrument;
	for(notelib_instrument_uint instrument_index = 0; instrument_index < ((struct notelib_internals*)notelib_state)->instrument_count; ++instrument_index){
		instrument = notelib_internals_get_instrument(notelib_state, instrument_index);
		if(instrument->step_count == 0){
			*instrument_index_dest = instrument_index;
			goto instrument_index_found;
		}
	}
	return notelib_answer_failure_unknown;
instrument_index_found:
	atomic_store_explicit(&instrument->active_channel_count, 0, memory_order_relaxed);
	instrument->step_count = step_count;
	struct notelib_processing_step_entry* steps_ptr;
	void* steps_ptr_to_free_at_failure;
	bool processing_steps_inline = notelib_instrument_are_processing_steps_inline(notelib_state, instrument);
	if(processing_steps_inline){
		steps_ptr = ((struct notelib_instrument_inline_steps*)instrument)->inline_steps;
		steps_ptr_to_free_at_failure = NULL;
	}else{
		steps_ptr = malloc(step_count*(sizeof(struct notelib_processing_step_entry)));
		if(steps_ptr == NULL){
			instrument->step_count = 0;
			return notelib_answer_failure_unknown;
		}
		steps_ptr_to_free_at_failure = steps_ptr;
		((struct notelib_instrument_external_steps*)instrument)->external_steps = steps_ptr;
	}
	notelib_instrument_state_uint state_offset = 0;
	for(notelib_step_uint step_index = 0; step_index < step_count; ++step_index){
		steps_ptr[step_index].step = steps[step_index].step;
		steps_ptr[step_index].data_offset = state_offset;
		state_offset += steps[step_index].state_data_size;
		steps_ptr[step_index].trigger_data_offset = steps[step_index].trigger_data_offset;
		steps_ptr[step_index].setup = steps[step_index].setup;
		steps_ptr[step_index].cleanup = steps[step_index].cleanup;
	}
	instrument->channel_data_size = state_offset;
	size_t channel_state_size = NOTELIB_CHANNEL_SIZEOF_SINGLE(state_offset);
	instrument->channel_count = channel_count;
	if(!notelib_instrument_is_state_data_inline(notelib_state, instrument)){
		void* state_data = malloc(channel_count * channel_state_size);
		if(state_data == NULL){
			//if(steps_ptr_to_free_at_failure != NULL)
			free(steps_ptr_to_free_at_failure);
			instrument->step_count = 0;
			return notelib_answer_failure_unknown;
		}
		*notelib_instrument_get_external_state_data_ptr_ptr(instrument, processing_steps_inline) = state_data;
	}
	return notelib_answer_success;
}
//could possibly optimize to single realloc call in certain cases
enum notelib_status notelib_set_instrument_channel_count(notelib_state_handle notelib_state, notelib_instrument_uint instrument_index, notelib_channel_uint channel_count){
	struct notelib_instrument* instrument_ptr = notelib_internals_get_instrument(notelib_state, instrument_index);
	if(atomic_load_explicit(&instrument_ptr->active_channel_count, memory_order_acquire) != 0) //Would optimally not actually be required if the user is careful enough
		return notelib_answer_failure_unknown;
	//Another failure would be to have a note command in the command queue (which we can't check for here) and not push any new commands before it is activated (missing the synchronizing read).
	bool processing_steps_inline = notelib_instrument_are_processing_steps_inline(notelib_state, instrument_ptr);
	void* ptr_to_free;
	if(!notelib_instrument_is_state_data_inline(notelib_state, instrument_ptr))
		ptr_to_free = *notelib_instrument_get_external_state_data_ptr_ptr(instrument_ptr, processing_steps_inline);
	else
		ptr_to_free = NULL;
	notelib_channel_uint channel_count_before = instrument_ptr->channel_count;
	instrument_ptr->channel_count = channel_count;
	if(!notelib_instrument_is_state_data_inline(notelib_state, instrument_ptr)){
		void* state_data = malloc(channel_count * NOTELIB_CHANNEL_SIZEOF_SINGLE(instrument_ptr->channel_data_size));
		if(state_data == NULL){
			instrument_ptr->channel_count = channel_count_before;
			return notelib_answer_failure_unknown;
		}
		*notelib_instrument_get_external_state_data_ptr_ptr(instrument_ptr, processing_steps_inline) = state_data;
	}
	//if(ptr_to_free != NULL)
	free(ptr_to_free);
	return notelib_answer_success;
}
enum notelib_status notelib_unregister_instrument(notelib_state_handle notelib_state, notelib_instrument_uint instrument_index){
	struct notelib_instrument* instrument_ptr = notelib_internals_get_instrument(notelib_state, instrument_index);
	if(atomic_load_explicit(&instrument_ptr->active_channel_count, memory_order_acquire) != 0) //Would optimally not actually be required if the user is careful enough
		return notelib_answer_failure_unknown;
	//Another failure would be to have a note command in the command queue (which we can't check for here), unregister the instrument and not register another one on time.
	bool processing_steps_inline = notelib_instrument_are_processing_steps_inline(notelib_state, instrument_ptr);
	if(!processing_steps_inline)
		free(((struct notelib_instrument_external_steps*)instrument_ptr)->external_steps);
	if(!notelib_instrument_is_state_data_inline(notelib_state, instrument_ptr))
		free(*notelib_instrument_get_external_state_data_ptr_ptr(instrument_ptr, processing_steps_inline));
	instrument_ptr->step_count = 0;
	return notelib_answer_success;
}

//TODO: For concurrency to be safe there has to be some sort of trigger for starting to query a track coming from the client thread. Implement that somehow!
enum notelib_status notelib_start_track
(notelib_state_handle notelib_state, notelib_track_uint* track_index_dest, uint32_t initialized_channel_buffer_size, notelib_position tempo_interval, notelib_sample_uint tempo_interval_samples){
	if(tempo_interval_samples == 0)
		return notelib_answer_failure_invalid_track_parameters_stopped;
	struct notelib_internals* internals = (struct notelib_internals*)notelib_state;
	notelib_track_uint regular_track_count = internals->track_count;
	struct notelib_track* track_ptr;
	notelib_track_uint track_index;
	for(track_index = 0; track_index < regular_track_count; ++track_index){
		track_ptr = notelib_internals_get_regular_track(notelib_state, track_index);
		if(notelib_track_is_disabled(track_ptr))
			goto track_found;
	}
	return notelib_answer_failure_no_track_available;
track_found:;
	enum notelib_status base_setup_status = notelib_track_regular_data_setup(internals, track_ptr, initialized_channel_buffer_size);
	if(base_setup_status != notelib_answer_success){
		track_ptr->position_sample_offset = 0;
		track_ptr->position = 0;
		track_ptr->tempo_ceil_interval = tempo_interval;
		track_ptr->tempo_ceil_interval_samples = tempo_interval_samples;
		*track_index_dest = track_index;
		return notelib_answer_success;
	}else{
		//notelib_track_disable(track_ptr); - no-op, because track was disabled before and track_ptr->tempo_ceil_interval_samples is only ever assigned in the other branch
		return base_setup_status;
	}
}
enum notelib_status notelib_reset_track_position(notelib_state_handle notelib_state, notelib_track_uint track_index, notelib_position position){
	struct notelib_command reset_command;
	reset_command.data.type = notelib_command_type_reset;
	reset_command.position = position;
	if(circular_buffer_write(notelib_track_get_command_queue(notelib_internals_get_regular_track(notelib_state, track_index)), &reset_command))
		return notelib_answer_success;
	else
		return notelib_answer_failure_unknown;
}
enum notelib_status notelib_set_track_tempo
(notelib_state_handle notelib_state,
 notelib_track_uint track_index, notelib_position position,
 notelib_position tempo_interval,
 notelib_sample_uint tempo_interval_samples){
	//TODO: to consider as future extension: command_tempo_interval_samples == 0 could signal unchanged interval samples (to only scale the position base tempo)
	if(tempo_interval_samples == 0)
		return notelib_answer_failure_unknown;
	struct notelib_command set_tempo_command;
	set_tempo_command.data.type = notelib_command_type_set_tempo;
	set_tempo_command.data.tempo.position_interval = tempo_interval;
	set_tempo_command.data.tempo.interval = tempo_interval_samples;
	set_tempo_command.position = position;
	if(circular_buffer_write(notelib_track_get_command_queue(notelib_internals_get_regular_track(notelib_state, track_index)), &set_tempo_command))
		return notelib_answer_success;
	else
		return notelib_answer_failure_unknown;
}
enum notelib_status notelib_set_track_initialized_channel_buffer_size
(notelib_state_handle notelib_state,
 notelib_track_uint track_index,
 uint32_t initialized_channel_buffer_size){
	struct notelib_track* track_ptr = notelib_internals_get_regular_track(notelib_state, track_index);
	void* ptr_to_free;
	uint16_t command_queue_size = ((struct notelib_internals*)notelib_state)->command_queue_size;
	if(notelib_track_is_initialized_channel_buffer_internal(notelib_state, track_ptr))
		ptr_to_free = NULL;
	else
		ptr_to_free = notelib_track_get_external_initialized_channel_buffer_ptr(track_ptr, command_queue_size);
	uint32_t initialized_channel_buffer_size_before = track_ptr->initialized_channel_buffer_size;
	track_ptr->initialized_channel_buffer_size = initialized_channel_buffer_size;
	if(notelib_track_is_initialized_channel_buffer_internal(notelib_state, track_ptr)){
		size_t circular_initialized_channel_buffer_size = notelib_internals_sizeof_track_initialized_channel_buffer(initialized_channel_buffer_size);
		void* allocated_initialized_channel_buffer = malloc(circular_initialized_channel_buffer_size);
		if(allocated_initialized_channel_buffer == NULL){
			track_ptr->initialized_channel_buffer_size = initialized_channel_buffer_size_before;
			return notelib_answer_failure_unknown;
		}
		struct circular_buffer_liberal_reader_unsynchronized* constructed_initialized_channel_buffer =
			circular_buffer_liberal_reader_unsynchronized_construct(allocated_initialized_channel_buffer, circular_initialized_channel_buffer_size, initialized_channel_buffer_size);
		if(constructed_initialized_channel_buffer == NULL){
			free(allocated_initialized_channel_buffer);
			track_ptr->initialized_channel_buffer_size = initialized_channel_buffer_size_before;
			return notelib_status_not_ok;
		}
		*notelib_track_get_external_initialized_channel_buffer_ptr_ptr(track_ptr, command_queue_size) = constructed_initialized_channel_buffer;
	}
	free(ptr_to_free);
	return notelib_answer_success;
}
enum notelib_status notelib_stop_track(notelib_state_handle notelib_state, notelib_track_uint track_index){
	struct notelib_track* track_ptr = notelib_internals_get_regular_track(notelib_state, track_index);
	if(!notelib_track_is_disabled(track_ptr)){
		notelib_track_disable(track_ptr);
		if(!notelib_track_is_initialized_channel_buffer_internal(notelib_state, track_ptr))
			free(notelib_track_get_external_initialized_channel_buffer_ptr(track_ptr, ((struct notelib_internals*)notelib_state)->command_queue_size));
	}
	return notelib_answer_success;
}

enum notelib_status notelib_play
(notelib_state_handle notelib_state,
 notelib_instrument_uint instrument_index,
 void* trigger_data,
 notelib_track_uint track_index, notelib_position position,
 notelib_note_id_uint* note_id_target){
	struct notelib_track* track_ptr = notelib_internals_get_regular_track(notelib_state, track_index);
	if(notelib_track_is_disabled(track_ptr))
		return notelib_answer_failure_invalid_track;
	struct circular_buffer_liberal_reader_unsynchronized* initialized_channel_buffer = notelib_track_get_initialized_channel_buffer_ptr(notelib_state, track_ptr);
	struct notelib_instrument* instrument_ptr = notelib_internals_get_instrument(notelib_state, instrument_index);
	notelib_step_uint step_count = instrument_ptr->step_count;
	if(step_count == 0)
		return notelib_answer_failure_invalid_instrument;

	notelib_instrument_state_uint channel_data_size = instrument_ptr->channel_data_size;
	void* channel_data_pre_write_buffer = malloc(channel_data_size);
	if(channel_data_pre_write_buffer == NULL)
		return notelib_answer_failure_bad_alloc;
	struct notelib_processing_step_entry* steps_ptr = notelib_instrument_get_processing_steps(notelib_state, instrument_ptr);
	for(notelib_step_uint i = 0; i < step_count; ++i){
		struct notelib_processing_step_entry* step = steps_ptr + i;
		notelib_processing_step_setup_function setup = step->setup;
		if(setup != NULL)
			setup
			(NOTELIB_INTERNAL_OFFSET_AND_CAST(trigger_data,                  step->trigger_data_offset, void*),
			 NOTELIB_INTERNAL_OFFSET_AND_CAST(channel_data_pre_write_buffer, step->data_offset,         void*));
	}
	if(!circular_buffer_liberal_reader_unsynchronized_write(initialized_channel_buffer, channel_data_pre_write_buffer, channel_data_size)){
		for(notelib_step_uint i = 0; i < step_count; ++i){
			struct notelib_processing_step_entry* step = steps_ptr + i;
			notelib_processing_step_cleanup_function cleanup = step->cleanup;
			if(cleanup != NULL)
				cleanup(NOTELIB_INTERNAL_OFFSET_AND_CAST(channel_data_pre_write_buffer, step->data_offset, void*));
		}
		free(channel_data_pre_write_buffer);
		return notelib_answer_failure_insufficient_initialized_channel_buffer_space;
	}

	struct notelib_command play_note_command;
	play_note_command.data.type = notelib_command_type_note;
	play_note_command.data.note.instrument_index = instrument_index;
	notelib_note_id_uint note_id = notelib_get_next_note_id(notelib_state);
	//could split up get and increment of note_id and only increment if command_queue write was successful
	 //+ help performance in the case of error (which is not a priority)
	 //- possibly hurt data access locality? Can fetch-post-increment optimization be expected?
	play_note_command.data.note.note_id = note_id;
	if(note_id_target != NULL)
		*note_id_target = note_id;
	play_note_command.position = position;
	if(!circular_buffer_write(notelib_track_get_command_queue(track_ptr), &play_note_command)){
		for(notelib_step_uint i = 0; i < step_count; ++i){
			struct notelib_processing_step_entry* step = steps_ptr + i;
			notelib_processing_step_cleanup_function cleanup = step->cleanup;
			if(cleanup != NULL)
				cleanup(NOTELIB_INTERNAL_OFFSET_AND_CAST(channel_data_pre_write_buffer, step->data_offset, void*));
		}
		if(!circular_buffer_liberal_reader_unsynchronized_unwrite(initialized_channel_buffer, channel_data_size))
			return notelib_status_not_ok;
		free(channel_data_pre_write_buffer);
		return notelib_answer_failure_insufficient_command_queue_entries;
	}
	//TODO: expand notelib_deinit (or all track stopping I guess? Because we would never know which tracks were active once...) by cleanup routine of enqueued-but-never-dequeued commands - for note commands, call the instrument cleanup routines!

	free(channel_data_pre_write_buffer);
	return notelib_answer_success;
}
enum notelib_status notelib_enqueue_trigger
(notelib_state_handle notelib_state,
 notelib_trigger_function trigger, void* userdata,
 notelib_track_uint track_index, notelib_position position){
	struct notelib_track* track_ptr = notelib_internals_get_regular_track(notelib_state, track_index);
	if(notelib_track_is_disabled(track_ptr))
		return notelib_answer_failure_invalid_track;

	struct notelib_command trigger_command;
	trigger_command.data.type = notelib_command_type_trigger;
	trigger_command.data.trigger.trigger_function = trigger;
	trigger_command.data.trigger.userdata = userdata;
	trigger_command.position = position;
	if(!circular_buffer_write(notelib_track_get_command_queue(track_ptr), &trigger_command))
		return notelib_answer_failure_insufficient_command_queue_entries;

	return notelib_answer_success;
}
enum notelib_status notelib_alter
(notelib_state_handle notelib_state,
 notelib_alter_function alter, void* userdata,
 notelib_instrument_uint note_id,
 notelib_track_uint track_index, notelib_position position){
	struct notelib_track* track_ptr = notelib_internals_get_regular_track(notelib_state, track_index);
	if(notelib_track_is_disabled(track_ptr))
		return notelib_answer_failure_invalid_track;

	struct notelib_command alter_command;
	alter_command.data.type = notelib_command_type_alter;
	alter_command.data.alter.note_id = note_id;
	alter_command.data.alter.alter_function = alter;
	alter_command.data.alter.userdata = userdata;
	alter_command.position = position;
	if(!circular_buffer_write(notelib_track_get_command_queue(track_ptr), &alter_command))
		return notelib_answer_failure_insufficient_command_queue_entries;

	return notelib_answer_success;
}
