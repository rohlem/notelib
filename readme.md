**[on hiatus for at least a couple of months into 2019; no upper limit]**

**Current status: R1- (second release imminent); on hiatus (it's good enough for my use case; for now every new feature is a patch-fest, if I eventually return with more time it might end in a rewrite.)**

Notelib is a C11 library realizing a basic thread-safe communication scheme between controller ("client") code issuing commands to audio producing code (which renders waveforms into buffers).

To build/package notelib into a library, simply link together all compiled .c files from src/notelib, optionally including src/back.c.

For shared library output, see src/notelib/util/shared_linkage_specifiers.h and src/back.h for possible configuration via macro definitions.

The process is really simple; simple enough to provide a Makefile, but I don't know reliable cross-platform Make well enough. PRs welcome.

---

The basic structure from a "client" thread perspective:

 - initialize a notelib instance in a provided memory buffer
 - (hook up an audio backend to regularly process/query audio: see back.c for examples implementations)
 - register "instruments" (routines generating audio waveforms)
 - start tracks (or use the always-on "immediate" track) to send commands ("notes" which instantiate instruments, tempo change or generic alter/trigger)
 - (at some point stop and deinitialize the notelib instance)

To see an example application, view main.c. It uses either of two example "backends" implemented in back.c - libsoundio or PortAudio. (You'll need to build and link one of these to compile/link/run the program.)

---

**"Cricital" bugs (things that need to be fixed to guarantee it works correctly):**
 - struct notelib_track.tempo_ceil_interval_samples (used as an "enabled"-flag) is currently accessed from the audio thread without a cache-refreshing acquire-read, which is technically totally unsafe. Just add some way to implement that; possibly a "track enabling queue" or something superfluous like that.
   (note how in contrast, everything else is only accessed when a respective command is read through a track's command queue, which involves such an acquire read operation.)

**"Non-critical" to-dos (fixes/enhancements that would seriously improve the library interface):**
 - separate all components declared and defined in notelib/internal.h (currently it contains near everything)
 - separate declarations and definitions of everything in notelib/internal.h (currently that header must be included in exactly one translation unit, which is obviously an inhuman crime)
 - clean up the "main interface header" ("notelib/notelib.h") to only include what you actually want it to (and not overlap with any "backend connectors")
 - restructure the repository (put interface headers into an "include" directory, translation units for compilation into "src" or something, separate "backend connectors" (if you even want them) and examples into an "examples" or "test" directory or similar)
 - remove or replace the random commented out debug printf calls that are still floating around
 - document the interface of components (like structs) a little better (using functions to group init - and deinit routines, interpreting state as "enabled"-flags, making naming more consistent etc.)
 - adding a way to block on interaction with notelib (my attempt would be to shod together a C11 interface for C++11's std::condition_variable)
 - add further configuration options with macros and enums and stuff (there's probably a lot you _could_ do)
 - some minor optimizations probably, though honestly I don't think that's a big worry

 - (maybe rewrite in some other language - like C++, or maybe Zig - providing easier customizability/extensibility)
