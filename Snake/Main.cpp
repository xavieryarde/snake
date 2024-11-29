#include <iostream>
#include "Game.hpp"

int main(int argc, char* argv[]) {
	Game* game = new Game();

	if (!game->init()) {
		std::cerr << "Game could not be initialized" << std::endl;
	}

	game->run();

	return 0;
}

