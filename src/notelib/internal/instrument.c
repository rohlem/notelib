#include "instrument.h"

#include "alignment_utilities.h"
#include "internals.h"

#include <stddef.h>

struct notelib_instrument* notelib_internals_get_instrument(struct notelib_internals* internals, notelib_instrument_uint index)
	{return
		 NOTELIB_INTERNAL_OFFSET_AND_CAST
		 (internals,
		  notelib_internals_offsetof_instruments
		  + index * internals->instrument_size,
		  struct notelib_instrument*);}

size_t notelib_instrument_get_inline_steps_size(uint8_t inline_step_count)
	{return
		 inline_step_count
		 * NOTELIB_INTERNAL_PAD_SIZEOF
		   (struct notelib_processing_step_entry,
		    struct notelib_processing_step_entry);}

size_t notelib_internals_sizeof_instrument_inline_steps_up_to_inline_state_data(uint8_t inline_step_count)
	{return
		 NOTELIB_INTERNAL_ALIGN_TO_NEXT
		 (notelib_instrument_offsetof_steps_array
		  + notelib_instrument_get_inline_steps_size(inline_step_count),
		  NOTELIB_INSTRUMENT_STATE_DATA_ALIGNMENT);}

size_t notelib_internals_sizeof_instrument_inline_steps_up_to_external_state_data_ptr(uint8_t inline_step_count)
	{return
		 NOTELIB_INTERNAL_ALIGN_TO_NEXT_ALIGNOF
		 (notelib_instrument_offsetof_steps_array
		  + notelib_instrument_get_inline_steps_size(inline_step_count),
		  void*);}

size_t notelib_internals_sizeof_instrument(uint8_t inline_step_count, uint16_t reserved_inline_state_space)
	{return
		 NOTELIB_INTERNAL_ALIGN_TO_NEXT_ALIGNOF
		 (MAX(notelib_internals_sizeof_instrument_inline_steps_up_to_inline_state_data(inline_step_count), sizeof(struct notelib_processing_step_entry*))
		  + MAX(reserved_inline_state_space, sizeof(void*)),
		  struct notelib_instrument);}

bool notelib_instrument_are_processing_steps_inline(const struct notelib_internals* internals, const struct notelib_instrument* instrument)
	{return instrument->step_count <= internals->inline_step_count;}

struct notelib_processing_step_entry* notelib_instrument_get_processing_steps(const struct notelib_internals* internals, struct notelib_instrument* instrument){
	if(notelib_instrument_are_processing_steps_inline(internals, instrument))
		return ((struct notelib_instrument_inline_steps*)instrument)->inline_steps;
	else
		return ((struct notelib_instrument_external_steps*)instrument)->external_steps;
}

size_t notelib_internals_effective_sizeof_instrument_up_to_inline_state_data(const struct notelib_internals* internals, struct notelib_instrument* instrument){
	if(notelib_instrument_are_processing_steps_inline(internals, instrument))
		return notelib_internals_sizeof_instrument_inline_steps_up_to_inline_state_data  (instrument->step_count);
	else
		return notelib_instrument_external_offsetof_state_array;
}

size_t notelib_internals_effective_sizeof_instrument_up_to_external_state_data_ptr(const struct notelib_internals* internals, struct notelib_instrument* instrument){
	if(notelib_instrument_are_processing_steps_inline(internals, instrument))
		return notelib_internals_sizeof_instrument_inline_steps_up_to_external_state_data_ptr  (instrument->step_count);
	else
		return notelib_instrument_external_offsetof_state_ptr;
}

size_t notelib_instrument_get_effective_state_data_inline_space(const struct notelib_internals* internals, struct notelib_instrument* instrument)
	{return notelib_internals_sizeof_instrument(internals->inline_step_count, internals->reserved_inline_state_space)
		 - notelib_internals_effective_sizeof_instrument_up_to_inline_state_data(internals, instrument);}

bool notelib_instrument_is_state_data_inline(const struct notelib_internals* internals, struct notelib_instrument* instrument)
	{return (size_t)(instrument->channel_count * instrument->channel_state_size) <= notelib_instrument_get_effective_state_data_inline_space(internals, instrument);}

struct notelib_channel*  notelib_instrument_get_inline_state_data_after_inline_steps(struct notelib_instrument* instrument)
	{return
		 NOTELIB_INTERNAL_OFFSET_AND_CAST
		 (instrument,
		  notelib_internals_sizeof_instrument_inline_steps_up_to_inline_state_data(instrument->step_count),
		  struct notelib_channel*);}

struct notelib_channel** notelib_instrument_get_external_state_data_after_inline_steps(struct notelib_instrument* instrument)
	{return
		 NOTELIB_INTERNAL_OFFSET_AND_CAST
		 (instrument,
		  notelib_internals_sizeof_instrument_inline_steps_up_to_external_state_data_ptr(instrument->step_count),
		  struct notelib_channel**);}

struct notelib_channel*  notelib_instrument_get_inline_state_data_after_external_steps(struct notelib_instrument* instrument)
	{return
		 NOTELIB_INTERNAL_OFFSET_AND_CAST
		 (instrument,
		  notelib_instrument_external_offsetof_state_array,
		  struct notelib_channel*);}

struct notelib_channel* notelib_instrument_get_state_data(const struct notelib_internals* internals, struct notelib_instrument* instrument){
	if(notelib_instrument_are_processing_steps_inline(internals, instrument)){
		if(notelib_instrument_is_state_data_inline(internals, instrument))
			return  notelib_instrument_get_inline_state_data_after_inline_steps  (instrument);
		else
			return *notelib_instrument_get_external_state_data_after_inline_steps(instrument);
	}else{
		if(notelib_instrument_is_state_data_inline(internals, instrument))
			return  notelib_instrument_get_inline_state_data_after_external_steps(instrument);
		else
			return ((struct notelib_instrument_external_steps_external_state*)instrument)->external_state;
	}
}

struct notelib_channel** notelib_instrument_get_external_state_data_ptr(struct notelib_instrument* instrument, bool processing_steps_inline){
	if(processing_steps_inline)
		return notelib_instrument_get_external_state_data_after_inline_steps(instrument);
	else
		return &((struct notelib_instrument_external_steps_external_state*)instrument)->external_state;
}
