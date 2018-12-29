#ifndef NOTELIB_UTIL_MINGW_MAX_ALIGN_T_FIX_H_
#define NOTELIB_UTIL_MINGW_MAX_ALIGN_T_FIX_H_

/* This is a workaround to a MinGW (possibly GCC?) standard library header bug.
	MinGW (the windows port of GCC) has two separate standard library include directories (as might other GCC distributions): One in "lib/gcc/<platform string>/<version string>/include" and one in "<platform string>/include".
	Apparently the first one is selected when including via system include syntax (<header.h>), but many of those files (including <stddef.h> and <stdint.h>) contain an #include_next of the file of the same name at the second path (often near the beginning).
	The two variants of <stddef.h> don't agree on max_align_t:
		The second one (lib/gcc/<ps>/<vs>/include/stddef.h) has these two members:
			long long __max_align_ll __attribute__((__aligned__(__alignof__(long long))));
			long double __max_align_ld __attribute__((__aligned__(__alignof__(long double))));
		leading on my platform to an alignment of 8, while the first one (<ps>/include/stddef.h) has this additional member:
			#ifdef __i386__
			  __float128 __max_align_f128 __attribute__((__aligned__(__alignof(__float128))));
			#endif
		(with a trustworthy explanatory comment before it) leading on my platform to an alignment of 16.
	When included normally, the macro _GCC_MAX_ALIGN_T guards these definitions so the one from the second path (alignment 8) wins out.
	<stdint.h> accesses <stddef.h> differently however, by first defining the macros __need_wint_t and __need_wchar_t. Both <stddef.h> have code to detect this, skip/ignore the definition of max_align_t and other types, and undefine the macros.
	Because this is done sequentially however, only one of the two headers (the one in the second include directory) ever gets to see them, and now the other header (in directory 1) takes its turn in defining max_align_t.

	Just as a temporary workaround, we'll just always include <stddef.h> first, because that results in alignment 8, which coincides with our malloc implementation.
	Overall though alignment 16 is probably the "correct" value (to have __float128 support on i386), and it's actually malloc that's wrong. I'm just hesitant to include a patched malloc wrapper among all the other junk/utilities in this library.*/

#include <stddef.h>

#endif//#ifndef NOTELIB_UTIL_MINGW_MAX_ALIGN_T_FIX_H_
