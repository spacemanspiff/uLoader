PROGS = nandMan
COMMON = endian.o fat.o main.o rijndael.o shared.o utils.o wads.o wiidisc.o

CFLAGS = -Wall -m32 -W -ggdb 

ifeq ($(OS),linux)
LDFLAGS = -m32
#LIBS =  -lcrypto
else
#HOST = i686-pc-mingw32-
LDFLAGS = -m32 -static
#LIBS = C:/Dev-Cpp/lib/libwsock32.a C:/Dev-Cpp/lib/libcrypto.a C:/Dev-Cpp/lib/libcomdlg32.a
EXT=.exe
endif
CC = $(HOST)gcc

#OBJS =  $(patsubst %,%.o,$(PROGS)) $(COMMON)
OBJS =  $(COMMON)

all: $(PROGS)

$(PROGS): %: $(COMMON) Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) $(COMMON) $(LIBS) -o $@$(EXT)

$(OBJS): %.o: %.c Makefile
	$(CC) $(CFLAGS) $(DEFINES) -c $< -o $@ 

clean:
	-rm -f $(OBJS) $(PROGS)$(EXT)

