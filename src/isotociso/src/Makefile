PROGS = isotociso
COMMON = tools.o bn.o ec.o wiidisc.o rijndael.o os.o
DEFINES = -DLARGE_FILES -D_FILE_OFFSET_BITS=64

#CFLAGS = -Wall -W -Os -Ilibwbfs -I.
CFLAGS = -Wall -m32 -W  -ggdb -Ilibwbfs -I.

ifeq ($(OS),linux)
LDFLAGS = -m32
LIBS =  -lcrypto
else
#HOST = i686-pc-mingw32-
LDFLAGS = -m32 -static
LIBS = C:/Dev-Cpp/lib/libwsock32.a C:/Dev-Cpp/lib/libcrypto.a C:/Dev-Cpp/lib/libcomdlg32.a
EXT=.exe
endif
CC = $(HOST)gcc

VPATH+=libwbfs
OBJS = $(patsubst %,%.o,$(PROGS)) $(COMMON)

all: $(PROGS)

$(PROGS): %: %.o $(COMMON) Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) $< $(COMMON) $(LIBS) -o $@$(EXT)

$(OBJS): %.o: %.c tools.h Makefile
	$(CC) $(CFLAGS) $(DEFINES) -c $< -o $@ 

clean:
	-rm -f $(OBJS) $(PROGS)
