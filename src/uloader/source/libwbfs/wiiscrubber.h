#ifndef WIISCRUBBER_H
#define WIISCRUBBER_H

#include "libwbfs.h"
#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

typedef struct _iso_files {
	char name[255];
	char path[255];
	u32 part;
	u64 offset;
	u32 size;
	u32 i;
	struct _iso_files *next;
} iso_files;

extern iso_files * CWIIDisc_first_file;
extern iso_files * CWIIDisc_last_file;

#ifdef __cplusplus
   }
#endif /* __cplusplus */

#endif
