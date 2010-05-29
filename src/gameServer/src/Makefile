#CFLAGS = -Wall -ggdb -D_FILE_OFFSET_BITS=64
#LDFLAGS = -Wall -ggdb -D_FILE_OFFSET_BITS=64

CFLAGS = -Wall -D_FILE_OFFSET_BITS=64 -O2
LDFLAGS = -Wall -D_FILE_OFFSET_BITS=64 

gameServer: remoteGame.o main.o server.o
	g++ $(LDFLAGS) remoteGame.o main.o server.o -o gameServer

remoteGame.o: remoteGame.cpp remoteGame.h
	g++ -c $(CFLAGS) -o remoteGame.o remoteGame.cpp

main.o: main.cpp remoteGame.h server.h
	g++ -c $(CFLAGS) -o main.o main.cpp

server.o: server.cpp server.h remoteGame.h
	g++ -c $(CFLAGS) -o server.o server.cpp

clean:
	rm -f *.o gameServer 
