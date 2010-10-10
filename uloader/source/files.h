#ifndef _FILES_H_
#define _FILES_H_

extern char path_file[258];
extern int nfiles;
extern int ndirs;


extern int have_device; //(1->SD | 2 -> USB)
extern int current_device;

#define MAX_ENTRY 2048



struct _files
{
	int size;
	char name[258];
	char is_directory;
	char is_selected;
};

extern struct _files files[MAX_ENTRY];

int is_ext(char *a,char *b);
char *get_name_short(char *name);
char *get_name_short_fromUTF8(char *name);
void char_to_utf8(char *s, char *d);
void utf8_to_char(char *s, char *d);
char *get_name(char *name);
char *get_name_from_UTF8(char *name);

void read_list_file(unsigned char *dirname, int flag);

#endif

