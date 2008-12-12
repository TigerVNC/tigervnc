/* common-config.win.h - Platform-specific definitions for Windows */

#define HAVE_VSNPRINTF 1
#ifdef _MSC_VER
/* Older versions of MSVC lacks vsnprintf. MinGW has it, and if this
   macro is defined before stdio.h is included, things will break. */
#define vsnprintf _vsnprintf
#endif

#define HAVE_SNPRINTF 1
#define snprintf _snprintf
