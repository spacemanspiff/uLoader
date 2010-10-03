#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
 
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif
 
#define DATATYPE_S8  0
#define DATATYPE_U8  1
#define DATATYPE_S16 2
#define DATATYPE_U16 3
#define DATATYPE_S32 4
#define DATATYPE_U32 5


int useconst = 0;
int zeroterminated = 0;
int createheader = 0;
int alignment = 0;
int datatype = DATATYPE_U8;

const char *datatype2str(int datatype)
{
	const char *result;

	switch (datatype) {
		case DATATYPE_S8:
			result = "char";
			break;
		case DATATYPE_U8:
			result = "unsigned char";
			break;
		case DATATYPE_S16:
			result = "short";
			break;
		case DATATYPE_U16:
			result = "unsigned short";
			break;
		case DATATYPE_S32:
			result = "int";
			break;
		case DATATYPE_U32:
			result = "unsigned int";
			break;
	};
	return result;
}

int myfgetc(FILE *f)
{
	int c = fgetc(f);
	if (c == EOF && zeroterminated)
	{
		zeroterminated = 0;
		return 0;
	}
	return c;
}

#define MAX_BUFFER 255

int process(const char *ifname, const char *exportname)
{
	struct stat ifstat;
	char ofname[MAX_BUFFER];
	snprintf(ofname, MAX_BUFFER, "%s.c", exportname);

	FILE *ifile, *ofile;

	if (stat(ifname, &ifstat) != 0) {
		fprintf(stderr, "cannot stat %s\n", ifname);
		exit(1);
	}
	//printf("tamaño: %d\n",ifstat.st_size);
	if (createheader) {
		char hfname[MAX_BUFFER];
		snprintf(hfname, MAX_BUFFER, "%s.h", exportname);

		FILE *hfile = fopen(hfname, "w");
		if (hfile == NULL)
		{
			fprintf(stderr, "cannot open %s for writing\n", hfname);
			exit(1);
		}

		fprintf(hfile, "#define size_%s %d\n\n", exportname, ifstat.st_size);
		fprintf(hfile, "extern %s %s %s[%d];\n", 
					    useconst ? "const " : "", 
					    datatype2str(datatype),
					    exportname, ifstat.st_size);
		fclose(hfile);
	}

	ifile = fopen(ifname, "rb");
	if (ifile == NULL)
	{
		fprintf(stderr, "cannot open %s for reading\n", ifname);
		exit(1);
	}

	ofile = fopen(ofname, "wb");
	if (ofile == NULL)
	{
		fprintf(stderr, "cannot open %s for writing\n", ofname);
		exit(1);
	}

	char align_str[40];
	if (alignment)
		snprintf(align_str, 40, "__attribute__((aligned (%d)))", 
			alignment);
	else
		*align_str = '\0';

	fprintf(ofile, "%s%s %s[%d] %s = {\n", 
			useconst ? "const " : "", 
			datatype2str(datatype), 
			exportname,
			ifstat.st_size,
			align_str);

	char buffer[128];
	char n[10];
	int c;
	int left = ifstat.st_size;
	strcpy(buffer, "     ");
	while ((c = myfgetc(ifile)) != EOF) {
		snprintf(n, 128, "%d%s", c, left ? ", " : "");

		if (strlen(buffer) + strlen(n) >= 125) {
			fprintf	(ofile, "%s\n", buffer);
			strcpy(buffer, "     ");
		}
		strcat(buffer, n);
		left--;
	}
	fprintf	(ofile, "%s\n", buffer);

	fprintf(ofile, "\n};\n");
 
	fclose(ifile);
	fclose(ofile);
	return 0;
}
 
void usage(void)
{

	fprintf(stderr, "filetochar Version 1.0\n"
		"\n"
		"Use: filetochar <file> <exportname> [options]\n"
		"\n"
		"<file>\n"
		"<exportname>\n"
		"[options]\n"
		"    -h       -> Create .h file\n"
		"    -z       -> struct zero terminated\n"
		"    -align x -> Aligned to x bytes\n"
		"    -s8      -> type char\n"
		"    -u8      -> type unsigned char (default type)\n"
/*		"    -s16     -> type short\n"
		"    -u16     -> type unsigned short\n"
		"    -s32     -> type int\n"
		"    -u32     -> type unsigned int\n" */
		"\n"
		"Example:\n"
		"\n"
		"    filetochar file.bin filedata -h\n"
		"\n"
		"    filetochar file.bin filedata -h -s8 -align 16\n");

	exit(1);
}
 
int main(int argc, char **argv)
{
	if (argc <= 2)
	{
		usage();
	}

	const char *filename = argv[1];
	const char *output = argv[2];
	argv += 2;
	argc -= 2;

	while (argc > 1)
	{
		if (!strcmp(argv[1], "-c"))
		{
			useconst = 1;
			--argc;
			++argv;
		} else if (!strcmp(argv[1], "-h"))
		{
			createheader = 1;
			--argc;
			++argv;
		} else if (!strcmp(argv[1], "-s8"))
		{
			datatype = DATATYPE_S8;
			--argc;
			++argv;
		} else if (!strcmp(argv[1], "-u8"))
		{
			datatype = DATATYPE_U8;
			--argc;
			++argv;
/*		} else if (!strcmp(argv[1], "-s16"))
		{
			datatype = DATATYPE_S16;
			--argc;
			++argv;
		} else if (!strcmp(argv[1], "-u16"))
		{
			datatype = DATATYPE_U16;
			--argc;
			++argv;
		} else if (!strcmp(argv[1], "-s32"))
		{
			datatype = DATATYPE_S32;
			--argc;
			++argv;
		} else if (!strcmp(argv[1], "-u32"))
		{
			datatype = DATATYPE_U32;
			--argc;
			++argv;
*/
		} else if (!strcmp(argv[1], "-z"))
		{
			zeroterminated = 1;
			--argc;
			++argv;
		} else if (!strcmp(argv[1], "-align"))
		{
			if (argc < 3) 
				usage();
			alignment = atoi(argv[2]);
			argc -= 2;
			argv += 2;
		} else {
			usage();
		}
	}
	int res = process(filename, output);
	return res;
}
