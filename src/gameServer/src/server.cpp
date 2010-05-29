#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#include "server.h"
#include "remoteGame.h"

#include <iostream>

void sigchld_handler(int s)
{
    while(wait(NULL) > 0);
}

int server(int port, RemoteGame &game)
{
        int sockfd, new_fd;  // Escuchar sobre sock_fd, nuevas conexiones sobre new_fd
        struct sockaddr_in my_addr;    // información sobre mi dirección
        struct sockaddr_in their_addr; // información sobre la dirección del cliente
        socklen_t sin_size;
        struct sigaction sa;
        int yes=1;

        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("socket");
            exit(1);
        }

        if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
        
        my_addr.sin_family = AF_INET;         // Ordenación de bytes de la máquina
        my_addr.sin_port = htons(port);     // short, Ordenación de bytes de la red
        my_addr.sin_addr.s_addr = INADDR_ANY; // Rellenar con mi dirección IP
        memset(&(my_addr.sin_zero), '\0', 8); // Poner a cero el resto de la estructura

        if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr))
                                                                       == -1) {
            perror("bind");
            exit(1);
        }

        if (listen(sockfd, 100) == -1) {
            perror("listen");
            exit(1);
        }

        sa.sa_handler = sigchld_handler; // Eliminar procesos muertos
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;
        if (sigaction(SIGCHLD, &sa, NULL) == -1) {
            perror("sigaction");
            exit(1);
        }

        while(1) {  // main accept() loop
            sin_size = sizeof(struct sockaddr_in);
            if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr,
                                                           &sin_size)) == -1) {
                perror("accept");
                continue;
            }
            printf("server: got connection from %s\n",
                                               inet_ntoa(their_addr.sin_addr));
            if (!fork()) { // Este es el proceso hijo
                close(sockfd); // El hijo no necesita este descriptor
		handleChild(new_fd, game);
                exit(0);
            }
            close(new_fd);  // El proceso padre no lo necesita
        }

   return 0;
} 

#define BUFFER_SIZE 256
int handleChild(int socket, RemoteGame &g)
{
   char readBuf[32768];
   char buffer[BUFFER_SIZE];
   //snprintf(buffer,BUFFER_SIZE,"WII Game Server", g.gameTitle().c_str());
   //if (send(socket, buffer, strlen(buffer), 0) == -1)
   //    perror("send");
   int keepGoing = 1;
   while (keepGoing) {
     memset(buffer,0, BUFFER_SIZE);
     int len = recv(socket, buffer, BUFFER_SIZE, 0);
#ifdef DEBUG
     std::cout << "cmd: "<< buffer << "len: " << len << std::endl;
#endif 
     if (len>3)  {
   	if (strncmp(buffer,"READ",4) == 0) {
		char char_len[BUFFER_SIZE];
		char char_offset[BUFFER_SIZE];
		int i,j;
		j = 0; 
		i = 4;
		while (buffer[i] != ',' && i < len)
			char_len[j++] = buffer[i++];
		char_len[j] = 0;
		if (buffer[i] == ',')
			i++;
		j = 0; 
		while ((buffer[i] != '.' && buffer[i] != '\n' && buffer[i] != '\0') && i < len)
			char_offset[j++] = buffer[i++];
		char_offset[j] = 0;
		int length = atol(char_len);
		long long offset = atoll(char_offset);
#ifdef DEBUG
		std::cout << "Offset: " << char_offset << std::endl << "Len: " << char_len << std::endl;
#endif
		if (length <= 32768) {
			g.UnencryptedRead(readBuf, length, offset);
			send(socket, readBuf, length, 0);
		}
	}
     } else
   	keepGoing =0;
   }
   close(socket);
#ifdef DEBUG
   std::cout << "goodbye" << std::endl;
#endif
   return 0;
}

