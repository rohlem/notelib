#ifndef NOTELIB_UTIL_SHARED_LINKAGE_SPECIFIERS_H_
#define NOTELIB_UTIL_SHARED_LINKAGE_SPECIFIERS_H_

// Generic helper definitions for shared library support - (unrightfully?) taken from https://gcc.gnu.org/wiki/Visibility
#if defined _WIN32 || defined __CYGWIN__
  #define NOTELIB_UTIL_SHARED_IMPORT __declspec(dllimport)
  #define NOTELIB_UTIL_SHARED_EXPORT __declspec(dllexport)
  #define NOTELIB_UTIL_SHARED_LOCAL
#else
  #if __GNUC__ >= 4
    #define NOTELIB_UTIL_SHARED_IMPORT __attribute__ ((visibility ("default")))
    #define NOTELIB_UTIL_SHARED_EXPORT __attribute__ ((visibility ("default")))
    #define NOTELIB_UTIL_SHARED_LOCAL  __attribute__ ((visibility ("hidden")))
  #else
    #define NOTELIB_UTIL_SHARED_IMPORT
    #define NOTELIB_UTIL_SHARED_EXPORT
    #define NOTELIB_UTIL_SHARED_LOCAL
  #endif
#endif

// NOTELIB_API is used for the public API symbols. It declares shared library interfaces to import or export (and does nothing for static build).
// NOTELIB_LOCAL could/should be used for symbol declarations not part of the API, but GCC's compile flag -fvisibility=hidden or the directive '#pragma GCC visibility push(hidden)' honestly seem like better alternatives. Also, since it doesn't matter for dynamic linking, and static linking is post-optimized anyway, it really lacks any urgency for me to decide on anything.

#if defined(NOTELIB_STATIC) && NOTELIB_STATIC // defined as 1 if notelib is to be used as a static library
#define NOTELIB_SHARED 0
#endif // NOTELIB_STATIC

#if defined(NOTELIB_SHARED) && NOTELIB_SHARED // defined as 1 if notelib is to be used as a shared library
	#if defined(BUILDING_NOTELIB_SHARED) && BUILDING_NOTELIB_SHARED // defined as 1 if we are building the shared library (instead of using it)
		#define NOTELIB_API NOTELIB_UTIL_SHARED_EXPORT
	#else
		#define NOTELIB_API NOTELIB_UTIL_SHARED_IMPORT
	#endif // BUILDING_NOTELIB_SHARED
	#define NOTELIB_LOCAL NOTELIB_UTIL_SHARED_LOCAL
#else // NOTELIB_SHARED is not defined as 1: this means notelib is a static lib.
	#define NOTELIB_API
	#define NOTELIB_LOCAL
#endif // NOTELIB_SHARED

#endif//#ifndef NOTELIB_UTIL_SHARED_LINKAGE_SPECIFIERS_H_
