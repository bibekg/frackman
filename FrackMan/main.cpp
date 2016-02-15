#include "GameController.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <ctime>
using namespace std;

  // If your program is having trouble finding the Assets directory,
  // replace the string literal with a full path name to the directory,
  // e.g., "Z:/CS32/BoulderBlast/Assets" or "/Users/fred/cs32/BoulderBlast/Assets"

const string assetDirectory = "/Users/Bibek/Dropbox/School/CS 32/Projects/Project 3/FrackMan 2/DerivedData/FrackMan/Build/Products/Debug/Assets"; 

class GameWorld;

GameWorld* createStudentWorld(string assetDir = "");

int main(int argc, char* argv[])
{
	{
		string path = assetDirectory;
		if (!path.empty())
			path += '/';
		const string someAsset = "frack1.tga";
		ifstream ifs(path + someAsset);
		if (!ifs)
		{
			cout << "Cannot find " << someAsset << " in ";
			cout << (assetDirectory.empty() ? "current directory"
											: assetDirectory) << endl;
			return 1;
		}
	}

	srand(static_cast<unsigned int>(time(nullptr)));

	GameWorld* gw = createStudentWorld(assetDirectory);
	Game().run(argc, argv, gw, "FrackMan");
}
