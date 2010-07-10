#include "syscalls.h"
#include <stdarg.h>
#include <fcntl.h>
#include <string.h>

#define MEM_PRINT 1

#ifdef MEM_PRINT

char mem_cad[32];
#include <stdarg.h> 

#include "fat.h"
#include "fat_wrapper.h"

extern int verbose;

void int_char(int num)
{
	int sign = num<0;
	int n, m;

	if (num == 0) {
		mem_cad[0] = '0';
		mem_cad[1] = 0;
		return;
	}

	for (n = 0; n < 10; n++) {
		m = num % 10;
		num /= 10;
		if (m < 0)
			m = -m;
		mem_cad[25 - n] = 48 + m;
	}

	mem_cad[26] = 0;

	n = 0;
	m = 16;
	if (sign) {
		mem_cad[n] = '-';
		n++;
	}

	while (mem_cad[m] == '0') 
		m++;

	if (mem_cad[m] == 0)
		m--;

	while (mem_cad[m]) {
		mem_cad[n] = mem_cad[m];
		n++;
		m++;
	}
	mem_cad[n] = 0;
}

void uint_char(unsigned int num)
{
	int n,m;

	if (num == 0) {
		mem_cad[0] = '0';
		mem_cad[1] = 0;
		return;
	}

	for (n = 0; n < 10; n++) {
		m = num % 10;
		num /= 10;
		mem_cad[25 - n] = 48 + m;
	}

	mem_cad[26] = 0;

	n = 0;
	m = 16;

	while (mem_cad[m] == '0') 
		m++;

	if (mem_cad[m] == 0)
		m--;

	while (mem_cad[m]) {
		mem_cad[n] = mem_cad[m];
		n++;
		m++;
	}
	mem_cad[n] = 0;
}

void hex_char(u32 num)
{
	int n,m;

	if (num == 0) {
		mem_cad[0] = '0';
		mem_cad[1] = 0;
		return;
	}

	for (n = 0; n < 8; n++) {
		m = num & 15;
		num >>= 4;
		if (m >= 10)
			m += 7;
		mem_cad[23 - n] = 48 + m;
	}
	mem_cad[24] = 0;

	n = 0; 
	m = 16;
    
	mem_cad[n] = '0';
	n++;
	mem_cad[n] = 'x';
	n++;

	while (mem_cad[m] == '0') 
		m++;

	if (mem_cad[m] == 0) 
		m--;

	while (mem_cad[m]) {
		mem_cad[n] = mem_cad[m];
		n++;
		m++;
	}
	mem_cad[n] = 0;
}

extern s32 index_dev;

//int is_printf=0;
int internal_debug_printf=0;

static int internal=0;
void printf_write(int fd, void *data, int len)
{ 
	os_sync_after_write(data, len);

	if (internal) 
		FAT_Write(fd, data, len);
	else {
		internal |= 2;
		os_write(fd, data, len);
		internal &= 1;
	}
	// is_printf = 0;
}


void debug_printf(char *format,...)
{
#if 1
	va_list	opt;
	static char out[32] ATTRIBUTE_ALIGN(32)=" ";
	int val;

	char *s;
	va_start(opt, format);
	int fd;

	int message;
	static int one=1;
	static u32 buffer[8];
	static u32 queuehandle2=-1;

	if (one) {
		queuehandle2 = os_message_queue_create(buffer, 8);
		one = 0;
	} else
		os_message_queue_receive(queuehandle2, (void *)&message, 0);

	internal = internal_debug_printf;
 
	if (internal & 2) goto salir;

	if (index_dev & 1) {
		if(internal)
			fd = FAT_Open("usb:/ffs_log.txt" ,O_WRONLY | O_APPEND);
		else
			fd = os_open("usb:/ffs_log.txt" ,O_WRONLY | O_APPEND);
	} else {
		if (internal)
			fd = FAT_Open("sd:/ffs_log.txt" ,O_WRONLY | O_APPEND);
		else
			fd = os_open("sd:/ffs_log.txt" ,O_WRONLY | O_APPEND);
	}

	if (fd < 0)
		goto salir;

	while (format[0]) {
		if (format[0] != '%') {
			out[0] = *format++; 
			printf_write(fd, out, strlen(out));
		} else {
			format++;
			switch(format[0]) {
			case 'd':
			case 'i':
				val = va_arg(opt,int);
				int_char(val);
				printf_write(fd, mem_cad, strlen(mem_cad));
				break;
			case 'u':
				val = va_arg(opt, unsigned);
				uint_char(val);
				//printf_write(fd, mem_cad, strlen(mem_cad));
				printf_write(fd, mem_cad, strlen(mem_cad));
				break;

			case 'x':
				val = va_arg(opt,int);
				hex_char((u32) val);
				printf_write(fd, mem_cad, strlen(mem_cad));
				break;

			case 's':
				s = va_arg(opt,char *);
				printf_write(fd, s, strlen(s));
				break;
			}
			format++;
		}
	}
	va_end(opt);

	if(internal)
		FAT_Close(fd);
	else
		os_close(fd);
salir:
	os_message_queue_send(queuehandle2, 0, 0);
#endif	
}
#endif 

