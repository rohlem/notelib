#ifndef NOTELIB_INTERNAL_PROCESSING_STEP_ENTRY_H_
#define NOTELIB_INTERNAL_PROCESSING_STEP_ENTRY_H_

#include  "../notelib.h"

struct notelib_processing_step_entry{
	notelib_processing_step_function step;
	notelib_instrument_state_uint data_offset;
	notelib_instrument_state_uint trigger_data_offset;
	notelib_processing_step_setup_function setup;
	notelib_processing_step_cleanup_function cleanup;
};

#endif//#ifndef NOTELIB_INTERNAL_PROCESSING_STEP_ENTRY_H_
