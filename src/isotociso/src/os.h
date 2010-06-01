#ifndef __OS_H__
#define __OS_H__

#include <tools.h>

#ifdef WIN32

#include "windows.h"

typedef HANDLE *HANDLE_TYPE;

#define DWORDFORMAT "lu"
#define HANDLEFORMAT "p"
#define snprintf _snprintf

#else

typedef int HANDLE_TYPE;
typedef long long LARGE_INTEGER;
typedef int DWORD;
#define DWORDFORMAT "ul"
#define HANDLEFORMAT "d"
#define INVALID_HANDLE_VALUE  (-1)

#endif

#define FILE_READ     1
#define FILE_WRITE    2

HANDLE_TYPE os_open(const char *fileName, int mode);
int os_close(HANDLE_TYPE handle);
int os_seek(HANDLE_TYPE handle, s64 offset );
int os_read(HANDLE_TYPE handle, void *buffer, int size, DWORD *readBytes);
int os_write(HANDLE_TYPE handle, void *buffer, int size, DWORD *written);
int os_truncate(HANDLE_TYPE handle, u64 newSize);

#define GCC_VERSION (__GNUC__ * 10000 \
                  + __GNUC_MINOR__ * 100 \
                  + __GNUC_PATCHLEVEL__)

#if GCC_VERSION >= 40400
#define FORMAT64BITS     "I64"
#else
#define FORMAT64BITS     "llu"
#endif

#endif


