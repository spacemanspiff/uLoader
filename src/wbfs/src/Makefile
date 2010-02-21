PROGS = wbfs_win
COMMON = tools.o bn.o ec.o disc_usage_table.o libwbfs.o libwbfs_win32.o wiidisc.o rijndael.o
DEFINES = -DLARGE_FILES -D_FILE_OFFSET_BITS=64
LIBS = C:/Dev-Cpp/lib/libwsock32.a C:/Dev-Cpp/lib/libcrypto.a C:/Dev-Cpp/lib/libcomdlg32.a

CC = gcc
#CFLAGS = -Wall -W -Os -Ilibwbfs -I.
CFLAGS = -Wall -m32 -W  -ggdb -Ilibwbfs -I.
LDFLAGS = -m32 -static

VPATH+=libwbfs
OBJS = $(patsubst %,%.o,$(PROGS)) $(COMMON)

all: $(PROGS)

$(PROGS): %: %.o $(COMMON) Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) $< $(COMMON) $(LIBS) -o $@

$(OBJS): %.o: %.c tools.h Makefile
	$(CC) $(CFLAGS) $(DEFINES) -c $< -o $@ 

clean:
	-rm -f $(OBJS) $(PROGS)
