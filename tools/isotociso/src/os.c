#include "os.h"

#ifdef WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#endif

HANDLE_TYPE os_open(const char *fileName, int mode)
{
	HANDLE_TYPE handle;

	switch (mode) {
	case FILE_READ:
#ifdef WIN32
		handle = CreateFile(fileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
#else
		handle = open(fileName, O_RDONLY);
#endif	
		break;
	case FILE_WRITE:
#ifdef WIN32
		handle = CreateFile(fileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
#else
		handle = open(fileName, O_CREAT | O_WRONLY, 0644);
#endif	
		break;
	default:
		handle =INVALID_HANDLE_VALUE;
	}

	return handle;
}

int os_close(HANDLE_TYPE handle)
{
	int res;
#ifdef WIN32
	res = CloseHandle(handle);
#else
	res = close(handle) == 0;
#endif
	return res;
}

int os_seek(HANDLE_TYPE handle, s64 offset )
{
	int res;
#ifdef WIN32
	LARGE_INTEGER large;
	large.QuadPart = offset;
	res = (SetFilePointerEx(handle, large, NULL, FILE_BEGIN) == TRUE);
#else
	res = (lseek(handle, offset, SEEK_SET) == offset);
#endif
	return res;
}

int os_read(HANDLE_TYPE handle, void *buffer, int size, DWORD *readBytes)
{
	int res;
#ifdef WIN32
	*readBytes = 0;
	res = ReadFile(handle, buffer, size, readBytes, NULL);
#else
	*readBytes = read(handle, buffer, size);
	res = *readBytes >= 0;
#endif
	return res;
}

int os_write(HANDLE_TYPE handle, void *buffer, int size, DWORD *written)
{
	int res;
#ifdef WIN32
	*written = 0;
	res = WriteFile(handle, buffer, size, written, NULL) == TRUE;
#else
	*written = write(handle, buffer, size);
	res = *written >= 0;
#endif
	return res;
}

int os_truncate(HANDLE_TYPE handle, u64 newSize)
{
	int res;
#ifdef WIN32
	LARGE_INTEGER large;
	large.QuadPart = newSize;
	SetFilePointerEx(handle, large, NULL, FILE_BEGIN);
	SetEndOfFile(handle);
	res = 1;
#else
	res = ftruncate(handle, newSize) < 0;
#endif
	return res;
}

