#include "remoteGame.h"
#include "server.h"

#include<iostream>

int main(int argc, char **argv)
{
	if (argc < 2) {
		std::cout << "Falta parametro necesario." << std::endl;
		return -1;
	}
	RemoteGame g(argv[1]);

	std::cout << "GameId:" << g.gameId() << std::endl;
	std::cout << "Game:" << g.gameTitle() << std::endl;

	server(5000, g);
	return 0;
}

