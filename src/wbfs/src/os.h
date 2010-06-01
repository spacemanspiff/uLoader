#ifndef __OS_H__
#define __OS_H__

#include <tools.h>

#ifdef WIN32

typedef HANDLE *HANDLE_TYPE;

#define snprintf _snprintf

#else

typedef int HANDLE_TYPE;
typedef long long LARGE_INTEGER;
typedef int DWORD;
#define INVALID_HANDLE_VALUE  (-1)
#define BOOL int
#define FALSE 0
#define TRUE 1

#define MAX_PATH 256
#endif

#define FILE_READ       1
#define FILE_WRITE      2
#define FILE_READWRITE  3

HANDLE_TYPE os_open(const char *fileName, int mode);
int os_close(HANDLE_TYPE handle);
int os_seek(HANDLE_TYPE handle, s64 offset );
int os_read(HANDLE_TYPE handle, void *buffer, int size, int *readBytes);
int os_write(HANDLE_TYPE handle, void *buffer, int size, int *written);
int os_truncate(HANDLE_TYPE handle, u64 newSize);
int os_get_capacity(const char *file, u32 *sector_size, u32 *n_sector);

int os_createDir(const char *dirName);
int os_copyFile(const char *src, const char *dst);
int os_chooseFile(char *buffer, int size, const char *filter, const char *defaultExtension);
void os_clearScreen(void);
void os_waitChar(void);

int os_getchar(void);
int os_getch(void);

#endif


