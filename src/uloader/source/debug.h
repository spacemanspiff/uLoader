#ifndef __ULOADER_DEBUG_H__
#define __ULOADER_DEBUG_H__

#ifdef ENABLE_DEBUG
int debug_init(int log_thread);
int debug_log(char *log);
int debug_dump_mem(unsigned char *mem, int len, char  *description);
#else
#define debug_log(x) 
#define debug_init(x)
#define debug_dump_mem(x,y,z)

#endif

extern char log_buffer[4096];
#endif
