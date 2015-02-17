run:
	g++ -std=c++11 -DNDEBUG -Wall main.cpp -O3 -lSDL2 -fopenmp -o rd && ./rd
