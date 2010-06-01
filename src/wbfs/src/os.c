#include "os.h"

#ifdef WIN32

#include <windows.h>

#include "xgetopt.h"
#include <conio.h>

#else

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/fs.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sendfile.h>

#include <ncurses.h>
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

	case FILE_READWRITE:
#ifdef WIN32
		handle = CreateFile(fileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
#else
		handle = open(fileName, O_RDWR, 0644);
#endif	
	default:
		handle =INVALID_HANDLE_VALUE;
	}

	return handle;
}

int os_close(HANDLE_TYPE handle)
{
	int res;
#ifdef WIN32
	CloseHandle(handle);
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

int os_read(HANDLE_TYPE handle, void *buffer, int size, int *readBytes)
{
	int res;
#ifdef WIN32
	*readBytes = 0;
	res = ReadFile(handle, buf, count, &readBytes, NULL);
#else
	*readBytes = read(handle, buffer, size);
	res = *readBytes >= 0;
#endif
	return res;
}

int os_write(HANDLE_TYPE handle, void *buffer, int size, int *written)
{
	int res;
#ifdef WIN32
	*written = 0;
	res = (WriteFile(handle, buffer, size, written, NULL) == TRUE)
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
	large2.QuadPart = newSize;
	SetFilePointerEx(handle, large2, NULL, FILE_BEGIN);
	SetEndOfFile(handle);
	res = 1;
#else
	res = ftruncate(handle, newSize) < 0;
#endif
	return res;
}

int os_get_capacity(const char *file, u32 *sector_size, u32 *n_sector)
{
#ifdef WIN32
	DISK_GEOMETRY dg;
	PARTITION_INFORMATION pi;

	DWORD bytes;
	HANDLE *handle = CreateFile(fileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);

	if (handle == INVALID_HANDLE_VALUE)
	{
		wbfs_error("could not open drive");
		return 0;
	}
	
	if (DeviceIoControl(handle, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &dg, sizeof(DISK_GEOMETRY), &bytes, NULL) == FALSE)
	{
		CloseHandle(handle);
		wbfs_error("could not get drive geometry");
		return 0;
	}

	*sector_size = dg.BytesPerSector;

	if (DeviceIoControl(handle, IOCTL_DISK_GET_PARTITION_INFO, NULL, 0, &pi, sizeof(PARTITION_INFORMATION), &bytes, NULL) == FALSE)
	{
		CloseHandle(handle);
		wbfs_error("could not get partition info");
		return 0;
	}

	*sector_count = (u32)(pi.PartitionLength.QuadPart / dg.BytesPerSector);
	
	CloseHandle(handle);
	return 1;
#else
        int fd = open(file,O_RDONLY);
        int ret;
        if(fd<0){
                return 0;
        }
        ret = ioctl(fd,BLKSSZGET,sector_size);
        if(ret<0)
        {
                FILE *f;
                close(fd);
                f = fopen(file,"r");
                fseeko(f,0,SEEK_END);
                *n_sector = ftello(f)/512;
                *sector_size = 512;
                fclose(f);
                return 1;
        }
        ret = ioctl(fd,BLKGETSIZE,n_sector);
        close(fd);
        if(*sector_size>512)
                *n_sector*=*sector_size/512;
        if(*sector_size<512)
                *n_sector/=512/ *sector_size;
        return 1;
#endif
}


int os_createDir(const char *dirName)
{
#ifdef WIN32
	return CreateDirectory(dirName, NULL);
#else
	return mkdir(dirName, 0755) == 0;
#endif
}

void os_clearScreen(void)
{
#ifdef WIN32
	system("cls");
#else
	system("clear");
#endif
}

int os_chooseFile(char *buffer, int size, const char *filter, const char *defaultExtension)
{
#ifdef WIN32
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn); 
	ofn.hwndOwner =NULL;
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = buffer;
	ofn.nMaxFile = size;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_SHAREAWARE ;
	ofn.lpstrDefExt = defaultExtension;
	return (GetOpenFileName(&ofn));
#else
	printf("Escriba el nombre de archivo (extension %s): ", defaultExtension);
	return fgets(buffer, size - 1,stdin) != NULL;
#endif
}

int os_copyFile(const char *src, const char *dst)
{
#ifdef WIN32
	return CopyFile(src, dst);
#else
	ssize_t bytesCopied;
	off_t offset = 0;
	off_t fileSize;
	int fdDst, fdSrc;


	fdSrc = os_open(src, FILE_READ);
	if (fdSrc == INVALID_HANDLE_VALUE)
		return 0;

	fileSize = lseek(fdSrc, 0, SEEK_END);

	fdDst = os_open(dst, FILE_WRITE);
	if (fdDst == INVALID_HANDLE_VALUE)
		return 0;

	bytesCopied = sendfile(fdDst, fdSrc, &offset, fileSize);

	os_close(fdDst);
	os_close(fdSrc);

	return bytesCopied == fileSize;
#endif
}

void os_waitChar(void)
{
#ifdef WIN32
	while(1) {
		if(!kbhit()) 
			break;
		getch();
	}
#else
	getch();      // wait for a character
#endif
}

int os_getchar(void)
{
#ifdef WIN32
	return getchar();
#else
#endif
}

int os_getch(void)
{
#ifdef WIN32
	return getch();
#else
#endif
}
