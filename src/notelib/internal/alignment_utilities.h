#ifndef NOTELIB_INTERNAL_ALIGNMENT_UTILITIES_H_
#define NOTELIB_INTERNAL_ALIGNMENT_UTILITIES_H_

#include <stdalign.h>

#define NOTELIB_INTERNAL_RETREAT_ONE_ALIGNMENT(VALUE, BASE) (((VALUE)-1) & ~((BASE)-1))
#define NOTELIB_INTERNAL_RETREAT_ONE_ALIGNOF(VALUE, BASE) NOTELIB_INTERNAL_RETREAT_ONE_ALIGNMENT((VALUE), (alignof(BASE)))
#define NOTELIB_INTERNAL_ALIGN_TO_NEXT(VALUE, BASE) (NOTELIB_INTERNAL_RETREAT_ONE_ALIGNMENT((VALUE), (BASE)) + (BASE))
#define NOTELIB_INTERNAL_ALIGN_TO_NEXT_ALIGNOF(VALUE, BASE) NOTELIB_INTERNAL_ALIGN_TO_NEXT((VALUE), (alignof(BASE)))
#define NOTELIB_INTERNAL_PAD_SIZEOF(BASE, NEXT) NOTELIB_INTERNAL_ALIGN_TO_NEXT_ALIGNOF((sizeof(BASE)), NEXT)

#define NOTELIB_INTERNAL_OFFSET_AND_CAST(PTR, OFFSET, DEST_TYPE) ((DEST_TYPE)(((unsigned char*)(PTR)) + (OFFSET)))

#endif//#ifndef NOTELIB_INTERNAL_ALIGNMENT_UTILITIES_H_
