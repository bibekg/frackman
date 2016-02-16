#include "StudentWorld.h"
#include <string>
#include <algorithm>

using namespace std;

GameWorld* createStudentWorld(string assetDir)
{
    return new StudentWorld(assetDir);
}

int StudentWorld::init() {
    
    int currentLevel = getLevel();
    
    // Construct/initialize FrackMan
    m_player = new FrackMan(this);
    
    // Construct/initialize dirt
    for (int x = 0; x < 64; x++) {
        for (int y = 0; y < 64; y++)
            if (y >= 60) m_dirt[x][y] = nullptr;
    
            else if (!isMineShaftRegion(x, y))
                m_dirt[x][y] = new Dirt(x, y, this);
    
            else m_dirt[x][y] = nullptr;
    }
    
    // Construct/initialize boulders
    int boulders = min(currentLevel / 2 + 2, 6);
    for (int i = 0; i < boulders;) {
        int randX = randInt(0, 60);
        int randY = randInt(20, 57);
        if (isRadiusClear(randX, randY) && canPlacePickupHere(randX, randY)) {
            m_objects.push_back(new Boulder(randX, randY, this));
            
            // Remove dirt behind this boulder
            for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 4; j++) {
                    destroyDirt(randX + i, randY + j);
                }
            }
            
            i++;
        }
    }
    
    // Construct/initialize gold nuggets
    int gold = max(5 - currentLevel / 2, 2);
    for (int i = 0; i < gold;) {
        int randX = randInt(0, 60);
        int randY = randInt(20, 57);
        if (isRadiusClear(randX, randY) && canPlacePickupHere(randX, randY)) {
            m_objects.push_back(new Gold(randX, randY, true, this));
            i++;
        }
    }
    
    // Construct/initialize oil barrels
    int barrels = min(2 + currentLevel, 20);
    for (int i = 0; i < barrels;) {
        int randX = randInt(0, 60);
        int randY = randInt(20, 57);
        if (isRadiusClear(randX, randY) && canPlacePickupHere(randX, randY)) {
            m_objects.push_back(new Barrel(randX, randY, this));
            i++;
        }
        
    }
    
    return GWSTATUS_CONTINUE_GAME;
}

int StudentWorld::move() {
    
    m_player->doSomething();
    
    vector<Actor*>::iterator it = m_objects.begin();
    
    // ONLY BOULDERS DOING SOMETHING RIGHT NOW
    while (it != m_objects.end()) {
        if ((*it)->getName() == Actor::boulder && (*it)->isAlive()) {
            (*it)->doSomething();
        }
        it++;
    }
    
    
    
    // Check if dead!!!!!!!!!!!!!!!!!
    
    
    // CLEAN UP
    
    it = m_objects.begin();
    
    // ONLY BOULDERS DOING SOMETHING RIGHT NOW
    while (it != m_objects.end()) {
        if ((*it)->getName() == Actor::boulder) {
            if (!(*it)->isAlive() && !((*it) == nullptr)) { // is dead
                delete (*it);
                (*it) = nullptr;
            }
        }
        it++;
    }

    
    
    return GWSTATUS_CONTINUE_GAME;
}

void StudentWorld::cleanUp() {
    delete m_player;
    for (int x = 0; x < 64; x++)
        for (int y = 0; y < 64; y++)
            delete m_dirt[x][y];
    
    // Delete m_objects
    
}

// PRIVATE MEMBER FUNCTIONS

bool StudentWorld::isMineShaftRegion(int x, int y) {
    if ((x >= 30 && x <= 33) && (y >= 4 && y <= 59)) return true;
    return false;
}

bool StudentWorld::canPlacePickupHere(int x, int y) {
    if (((x >= 27 && x <= 33) && (y >= 0)) || y > 56) return false;
    return true;

}

bool StudentWorld::isThereDirt(int x, int y) {
    
    if (y < 0 || y > 59 || x < 0 || x > 63) return false;
    
    return m_dirt[x][y] != nullptr;
}

void StudentWorld::destroyDirt(int x, int y) {
    delete m_dirt[x][y];
    m_dirt[x][y] = nullptr;
}

Actor::Name StudentWorld::whatIsHere(int x, int y) {
    
    if (m_player->getX() == x && m_player->getY() == y)
        return Actor::frackman;
    
    vector<Actor*>::iterator it = m_objects.begin();
    
    while (it != m_objects.end()) {
        if ((*it)->getX() == x && (*it)->getY() == y) {
            return (*it)->getName();
        }
        it++;
    }
    
    if (m_dirt[x][y] != nullptr) return Actor::dirt;
    
    return Actor::nothing;
}

bool StudentWorld::isRadiusClear(int x, int y) {
    vector<Actor*>::iterator it = m_objects.begin();
    
    while (it != m_objects.end()) {
//        double radius = distance(x, (*it)->getX(), y, (*it)->getY());

        
        double radius = pow(pow((x-(*it)->getX()), 2) + pow((y-(*it)->getX()), 2), 0.5);
        if (radius <= MAX_RADIUS)       // found something in radius
            return false;
        
        it++;
    }
    
    return true;    // no objects found to be within radius

    
}

int StudentWorld::randInt(int min, int max) {
    return rand() % (max-min) + min;
}

