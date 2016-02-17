#include "StudentWorld.h"
#include <string>
#include <algorithm>

using namespace std;

GameWorld* createStudentWorld(string assetDir) {
    return new StudentWorld(assetDir);
}

int StudentWorld::init() {
    
    int currentLevel = getLevel();
    
    // Construct/initialize FrackMan
    m_player = new FrackMan(this);
    
    // Construct/initialize dirt
    for (int x = 0; x < 64; x++) {
        for (int y = 0; y < 64; y++)
            
            // Leave top of screen free of dirt
            if (y >= 60) m_dirt[x][y] = nullptr;
        
        // Leave mineshaft empty, fill everything else
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
    
    vector<Actor*>::iterator it = m_objects.begin();
    
    // Ask (alive) objects to do something
    while (it != m_objects.end()) {
        if ((*it) != nullptr && (*it)->isAlive())
            (*it)->doSomething();
        it++;
    }
    
    // Ask player to do something
    m_player->doSomething();
    
    // Destroy and remove from vector objects that have died
    it = m_objects.begin();
    while (it != m_objects.end()) {
        if ((*it) != nullptr && !(*it)->isAlive()) {
            delete (*it);
            (*it) = nullptr;
            it = m_objects.erase(it);
        } else it++;
    }
    
    return GWSTATUS_CONTINUE_GAME;
}

void StudentWorld::cleanUp() {
    
    delete m_player;                    // Delete player
    for (int x = 0; x < 64; x++)        // Delete dirt
        for (int y = 0; y < 64; y++)
            delete m_dirt[x][y];
    
    vector<Actor*>::iterator it;
    it = m_objects.begin();
    
    
    while (it != m_objects.end()) {     // Delete all remaining objects
        delete (*it);
        (*it) = nullptr;
    }
}

// Dirt functions

bool StudentWorld::isThereDirt(int x, int y) {
    
    if (y < 0 || y > 59 || x < 0 || x > 63) return false;
    
    return m_dirt[x][y] != nullptr;
}

void StudentWorld::destroyDirt(int x, int y) {
    delete m_dirt[x][y];
    m_dirt[x][y] = nullptr;
}

// Boulder functions

bool StudentWorld::projectileWillCrash(int x, int y) {
    Actor::Name item = whatIsHere(x, y);
    return (item == Actor::boulder || item == Actor::dirt);
}

// Squirt functions

bool StudentWorld::squirtProtestors(int x, int y) {
    
    vector<Actor*>::iterator it = m_objects.begin();
    bool protestorKilled = false;
    
    while (it != m_objects.end()) {
        if ((*it) != nullptr && ((*it)->getName() == Actor::protestor || (*it)->getName() == Actor::hardcore) && (*it)->isAlive() && distance(x, (*it)->getX(), y, (*it)->getY()) < 3) {
            (*it)->setDead();
            protestorKilled = true;
        } else it++;
    }
    
    return protestorKilled;
}

void StudentWorld::spawnSquirt() {
    int x = m_player->getX(), y = m_player->getY();
    
    playSound(SOUND_PLAYER_SQUIRT);
    
    // up and right failing
    
    
    switch (m_player->getDirection()) {
        case Actor::up:
            if (canSquirtHere(x, y + 4))
                m_objects.push_back(new Squirt(x, y + 4, m_player->getDirection(), this));
            break;
        case Actor::right:
            if (canSquirtHere(x + 4, y))
                m_objects.push_back(new Squirt(x + 4, y, m_player->getDirection(), this));
            break;
        case Actor::down:
            if (canSquirtHere(x, y - 4))
                m_objects.push_back(new Squirt(x, y - 4, m_player->getDirection(), this));
            break;
        case Actor::left:
            if (canSquirtHere(x - 4, y))
                m_objects.push_back(new Squirt(x - 4, y, m_player->getDirection(), this));
            break;
        default:
            break;
    }
}

bool StudentWorld::canSquirtHere(int x, int y) {
    // CHECK FOR DIRT IN FOUR SPACES AHEAD!!!!!
    
    bool isDirt = false;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (isThereDirt(x+i, y+j)) {
                isDirt = true;
            }
        }
    }
    
    if (isDirt)
        return false;
    
    vector<Actor*>::iterator it = m_objects.begin();
    
    bool canSquirt = true;
    
    while (it != m_objects.end()) {
        
        if ((*it)->getName() == Actor::boulder &&
            distance(x,(*it)->getX(),y,(*it)->getY()) <= 3)
        { canSquirt = false; }
        
        it++;
    }
    
    return canSquirt;
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
        double radius = distance(x, (*it)->getX(), y, (*it)->getY());
        if (radius <= MAX_RADIUS)       // found something in radius
            return false;
        
        it++;
    }
    
    return true;    // no objects found to be within radius
    
    
}

int StudentWorld::randInt(int min, int max) {
    return rand() % (max-min) + min;
}

