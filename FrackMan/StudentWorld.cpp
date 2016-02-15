#include "StudentWorld.h"
#include <string>
using namespace std;

GameWorld* createStudentWorld(string assetDir)
{
	return new StudentWorld(assetDir);
}

int StudentWorld::init() {
    
    // Construct/initialize FrackMan
    m_player = new FrackMan(this);
    m_player->setVisible(true);
    
    // Construct/initialize dirt
    for (int x = 0; x < 64; x++)
        for (int y = 0; y < 64; y++)
            if (y >= 60) m_dirt[x][y] = nullptr;
            else if (!isMineShaftRegion(x, y)) {
                m_dirt[x][y] = new Dirt(x, y, this);
                m_dirt[x][y]->setVisible(true);
            }
    
    // Construct/initialize boulders
    
    
    return GWSTATUS_CONTINUE_GAME;
}

int StudentWorld::move() {
    
    // FRACKMAN ACTIONS
    m_player->doSomething();
    
    // Check if dead!!!!!!!!!!!!!!!!!
    
    return GWSTATUS_CONTINUE_GAME;
}

void StudentWorld::cleanUp() {
    delete m_player;
    for (int x = 0; x < 64; x++)
        for (int y = 0; y < 64; y++)
            delete m_dirt[x][y];

}

// PRIVATE MEMBER FUNCTIONS

bool StudentWorld::isMineShaftRegion(int x, int y) {
    if ((x >= 30 && x <= 33) && (y >= 4 && y <= 59)) return true;
    return false;
}

bool StudentWorld::isThereDirt(int x, int y) {
    
    if (y < 0 || y > 59 || x < 0 || x > 63) return false;
    
    return m_dirt[x][y] != nullptr;
}

void StudentWorld::destroyDirt(int x, int y) {
//    m_dirt[x][y]->setDead();
    delete m_dirt[x][y];
}