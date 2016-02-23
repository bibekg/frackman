#include "StudentWorld.h"
#include <string>
#include <algorithm>
#include <iomanip>

using namespace std;

GameWorld* createStudentWorld(string assetDir) {
    return new StudentWorld(assetDir);
}

StudentWorld::~StudentWorld() {
    delete m_player;
    for (int x = 0; x < 64; x++)
        for (int y = 0; y < 64; y++)
            delete m_dirt[x][y];
}

// --------------------------------------- //
// --------- MAIN GAME FUNCTIONS --------- //
// --------------------------------------- //

int StudentWorld::init() {
    
    int currentLevel = getLevel();
    
    
    m_maxProtesters = min(15.0, 2 + int(getLevel()) * 1.5);
    m_TICKSBETWEENSPAWNS = max(25, 200 - int(getLevel()));
    
    // Construct/initialize FrackMan
    m_player = new FrackMan(this);
    
    // Initialize exit map
    for (int y = 60; y >= 0; y--)
        for (int x = 63; x >= 0; x--)
            m_exitMap[x][y] = (63-x) + (60-y);
    
    for (int y = 61; y < 64; y++)
        for (int x = 0; x < 64; x++)
            m_exitMap[x][y] = -1;
    
    // Construct/initialize dirt
    for (int x = 0; x < 64; x++) {
        for (int y = 0; y < 64; y++)
            
            // Omit dirt on top and mineshaft
            if (y >= 60 || isMineShaftRegion(x, y))
                m_dirt[x][y] = nullptr;
        
            else
                m_dirt[x][y] = new Dirt(x, y, this);
    }
    
    // Construct/initialize boulders
    int boulders = min(currentLevel / 2 + 2, 6);
    for (int i = 0; i < boulders;) {
        int randX = randInt(0, 60);
        int randY = randInt(20, 57);
        
        //                int randX = randInt(5, 8);
        //                int randY = randInt(20, 57);
        if (isRadiusClear(randX, randY, MAX_RADIUS) && canPlacePickupHere(randX, randY)) {
            m_actors.push_back(new Boulder(randX, randY, this));
            
            // Remove dirt behind this boulder
            for (int i = 0; i < 4; i++)
                for (int j = 0; j < 4; j++)
                    destroyDirt(randX + i, randY + j);
            i++;
        }
    }
    
    // Construct/initialize gold nuggets
    int gold = max(5 - currentLevel / 2, 2);
    for (int i = 0; i < gold;) {
        int randX = randInt(0, 60);
        int randY = randInt(20, 57);
        if (isRadiusClear(randX, randY, MAX_RADIUS) && canPlacePickupHere(randX, randY)) {
            m_actors.push_back(new Gold(randX, randY, Gold::frackman, Gold::permanent, this));
            i++;
        }
    }
    
    // Construct/initialize oil barrels
    int barrels = min(2 + currentLevel, 20);
    for (int i = 0; i < barrels;) {
        int randX = randInt(0, 60);
        int randY = randInt(20, 57);
        if (isRadiusClear(randX, randY, MAX_RADIUS) && canPlacePickupHere(randX, randY)) {
            m_actors.push_back(new Barrel(randX, randY, this));
            i++;
        }
    }
    m_barrelsLeft = barrels;
    
    // Construct/initialize one protester
    m_actors.push_back(new Protester(IID_PROTESTER, this));
    
    return GWSTATUS_CONTINUE_GAME;
}

int StudentWorld::move() {
    
    // Update status text
    setDisplayText();
    
    vector<Actor*>::iterator it = m_actors.begin();
    while (it != m_actors.end()) {
        
        // Ask active actors to do something
        if ((*it) != nullptr && (*it)->isAlive())
            (*it)->doSomething();
        
        // Player died from actor action
        if (playerDied()) {
            decLives();
            return GWSTATUS_PLAYER_DIED;
        }
        
        it++;
    }
    
    // Ask player to do something
    m_player->doSomething();
    
    // Potentially spawn a protester
    if (protesterCount() < m_maxProtesters && m_ticksSinceProtesterSpawned >= m_TICKSBETWEENSPAWNS) {
        // Spawn a new protester
        m_ticksSinceProtesterSpawned = 0;
        int probabilityOfHardcore = min(90, int(getLevel()) * 10 + 30);
        int whichToSpawn = randInt(1, 101);
        if (whichToSpawn <= probabilityOfHardcore)
            // Spawn hardcore
            m_actors.push_back(new HardCoreProtester(this));
        else
            // Spawn regular
            m_actors.push_back(new Protester(IID_PROTESTER, this));
    } else
        m_ticksSinceProtesterSpawned++;
    
    // Potentially spawn a temporary pickup
    int G = (getLevel() * 25) + 300;
    if (randInt(0,G) == 1) {            // 1 in G chance of spawn
        int whatToSpawn = randInt(1, 6);
        if (whatToSpawn == 1)           // 1/5 chance of spawning Sonar Kit
            m_actors.push_back(new SonarKit(this));
        else                            // 4/5 chance of spawning Water Pool
            while (true) {
                int spawnX = randInt(0, 64), spawnY = randInt(0, 64);
                if (canPlaceWaterHere(spawnX, spawnY)) {
                    m_actors.push_back(new WaterPool(spawnX, spawnY, this));
                    break;
                }
            }
    }
    
    // Destroy and remove from vector objects that have died
    it = m_actors.begin();
    while (it != m_actors.end()) {
        if ((*it) != nullptr && !(*it)->isAlive()) {
            delete (*it);
            (*it) = nullptr;
            it = m_actors.erase(it);
        } else it++;
    }
    
    // Player died from his own action
    if (playerDied())
        return GWSTATUS_PLAYER_DIED;
    
    // No more oil barrels, FrackMan has finished the level
    if (finishedLevel()) {
        playSound(SOUND_FINISHED_LEVEL);
        return GWSTATUS_FINISHED_LEVEL;
    }
    
    return GWSTATUS_CONTINUE_GAME;
}

void StudentWorld::cleanUp() {
    
    delete m_player;                    // Delete player
    for (int x = 0; x < 64; x++)        // Delete dirt
        for (int y = 0; y < 64; y++)
            delete m_dirt[x][y];
    
    vector<Actor*>::iterator it;
    it = m_actors.begin();
    
    
    while (it != m_actors.end()) {     // Delete all remaining objects
        delete (*it);
        it = m_actors.erase(it);
    }
    
    // Testing: Print exit map
    
    for (int y = 63; y >= 0; y--) {
        for (int x = 0; x < 64; x++) {
            if (m_exitMap[x][y] == -1) {
                cerr << "dob ";
            } else {
                cerr << string(3 - to_string(m_exitMap[x][y]).length(), '0') + to_string(m_exitMap[x][y]) + " ";
            }
            
        }
        cerr << endl;
    }
}

// ------------------------------------- //
// --------- GENERAL FUNCTIONS --------- //
// ------------------------------------- //

Actor::Name StudentWorld::whatIsHere(int x, int y) {
    
    if (m_player->getX() == x && m_player->getY() == y)
        return Actor::frackman;
    
    vector<Actor*>::iterator it = m_actors.begin();
    
    while (it != m_actors.end()) {
        if ((*it)->getX() == x && (*it)->getY() == y)
            return (*it)->getName();
        it++;
    }
    
    if (m_dirt[x][y] != nullptr) return Actor::dirt;
    
    return Actor::nothing;
}

Actor::Name StudentWorld::whatIsBlockingPath(int x, int y, GraphObject::Direction dir) {
    
    vector<Actor*>::iterator it = m_actors.begin();
    
    while (it != m_actors.end()) {
        if ((*it)->getName() == Actor::boulder) {
            switch (dir) {
                case GraphObject::left:
                    if (x == 0 )
                        return Actor::wall;
                    if (isBoulderHere(x - 1, y))
                        return Actor::boulder;
                    break;
                case GraphObject::up:
                    if (y == 60)
                        return Actor::wall;
                    if (isBoulderHere(x, y + 1))
                        return Actor::boulder;
                    break;
                case GraphObject::right:
                    if (x == 60)
                        return Actor::wall;
                    if(isBoulderHere(x + 1, y))
                        return Actor::boulder;
                    break;
                case GraphObject::down:
                    if (y == 0)
                        return Actor::wall;
                    if(isBoulderHere(x, y - 1))
                        return Actor::boulder;
                    break;
                case GraphObject::none:
                    break;
            }
        }
        it++;
    }
    
    // Obstacle in path not found
    return Actor::nothing;
}

bool StudentWorld::isSpotBlocked(int x, int y) {
    
    if (x == -1 || x == 61 || y == -1 || y == 61) {
        return true;
    }
    
    for (int i = x; i < x + 4; i++) {
        for (int j = y; j < y + 4; j++) {
            if (isThereDirt(i, j)) {
                return true;
            }
        }
    }
    
    if (isBoulderHere(x, y))
        return true;
    
    return false;
}

// ---------------------------- //
// --------- FRACKMAN --------- //
// ---------------------------- //

bool StudentWorld::isPlayerOnDirt() {
    int playerX = m_player->getX();
    int playerY = m_player->getY();
    bool dug = false;
    for (int x = playerX; x < playerX + 4; x++)
        for (int y = playerY; y < playerY + 4; y++)
            if (isThereDirt(x, y)) {
                destroyDirt(x, y);
                dug = true;
            }
    return dug;
}

void StudentWorld::getPlayerAction() {
    int ch;
    if (getKey(ch) == true) {
        int pX = m_player->getX();
        int pY = m_player->getY();
        GraphObject::Direction dir = m_player->getDirection();
        switch (ch) {
            case KEY_PRESS_LEFT:
                if (dir != GraphObject::left)
                    m_player->setDirection(GraphObject::left);
                else if (whatIsBlockingPath(pX, pY, dir) == Actor::nothing) {
                    m_player->moveTo(pX - 1, pY);
                    markMap(pX-1, pY, m_exitMap[pX][pY]+1);
                }
                else if (whatIsBlockingPath(pX, pY, dir) == Actor::wall)
                    m_player->moveTo(pX, pY);
                break;
            case KEY_PRESS_UP:
                if (dir != GraphObject::up)
                    m_player->setDirection(GraphObject::up);
                else if (whatIsBlockingPath(pX, pY, dir) == Actor::nothing) {
                    m_player->moveTo(pX, pY + 1);
                    markMap(pX, pY + 1, m_exitMap[pX][pY]+1);
                }
                else if (whatIsBlockingPath(pX, pY, dir) == Actor::wall)
                    m_player->moveTo(pX, pY);
                break;
            case KEY_PRESS_RIGHT:
                if (dir != GraphObject::right)
                    m_player->setDirection(GraphObject::right);
                else if (whatIsBlockingPath(pX, pY, dir) == Actor::nothing) {
                    m_player->moveTo(pX + 1, pY);
                    markMap(pX + 1, pY, m_exitMap[pX][pY]+1);
                }
                else if (whatIsBlockingPath(pX, pY, dir) == Actor::wall)
                    m_player->moveTo(pX, pY);
                break;
            case KEY_PRESS_DOWN:
                if (dir != GraphObject::down)
                    m_player->setDirection(GraphObject::down);
                else if (whatIsBlockingPath(pX, pY, dir) == Actor::nothing) {
                    m_player->moveTo(pX, pY - 1);
                    markMap(pX, pY-1, m_exitMap[pX][pY]+1);
                }
                else if (whatIsBlockingPath(pX, pY, dir) == Actor::wall)
                    m_player->moveTo(pX, pY);
                break;
            case KEY_PRESS_ESCAPE:
                m_player->getAnnoyed(10);
                m_player->setDead();
                break;
            case KEY_PRESS_SPACE:
                if (m_player->squirts() > 0) {
                    spawnSquirt();
                    m_player->decSquirts();
                }
                break;
            case KEY_PRESS_TAB:
                dropGold();
                break;
            case 'Z':
            case 'z':
                useSonar();
                break;
            default:
                break;
        }
    }
}

void StudentWorld::dropGold(){
    if (m_player->nuggets() > 0) {
        m_actors.push_back(new Gold(m_player->getX(), m_player->getY(), Gold::protester, Gold::temporary, this));
        m_player->decNuggets();
    }
}

void StudentWorld::useSonar() {
    if (m_player->sonars() > 0) {
        m_player->decSonars();
        
        int x = m_player->getX();
        int y = m_player->getY();
        vector<Actor*>::iterator it = m_actors.begin();
        
        while (it != m_actors.end()) {
            if ((*it)->sonarMakesVisible() && radius(x, y, (*it)->getX(), (*it)->getY()) <= 12) {
                Pickup* p = dynamic_cast<Pickup*>((*it));
                if (p != nullptr)       // dynamic cast successful
                    p->makePickupVisible(true);
            }
            it++;
        }
    }
}

// ----------------------------- //
// --------- PROTESTER --------- //
// ----------------------------- //

void StudentWorld::protesterLeaveMap(Protester* protester) {
    int x = protester->getX(), y = protester->getY();
    GraphObject::Direction dir;
    
    getShortestRoute(x, y, dir);
    
    if (dir != protester->getDirection())
        protester->setDirection(dir);
    else
        protester->moveTo(x, y);
}

bool StudentWorld::shoutIfPossible(Protester* protester) {
    int playerX = m_player->getX(), playerY = m_player->getY();
    int protX = protester->getX(), protY = protester->getY();
    bool canShout = false;
    
    switch (protester->getDirection()) {
        case GraphObject::up:
            if (protX == playerX && protY == playerY - 4)
                canShout = true;
            break;
        case GraphObject::right:
            if (protX == playerX - 4 && protY == playerY)
                canShout = true;
            break;
        case GraphObject::down:
            if (protX == playerX && protY == playerY + 4)
                canShout = true;
            break;
        case GraphObject::left:
            if (protX == playerX + 4 && protY == playerY)
                canShout = true;
            break;
        default:
            break;
    }
    
    if (protester->ticksSinceLastShout() <= 15)
        canShout = false;
    
    if (canShout) {
        playSound(SOUND_PROTESTER_YELL);
        m_player->getAnnoyed(2);
        protester->resetShoutTicks();
    }
    
    return canShout;
}

GraphObject::Direction StudentWorld::directionToFrackMan(Protester *protester) {
    
    int x = protester->getX(), y = protester->getY();
    int pX = m_player->getX(), pY = m_player->getY();
    
    if (pX == x && pY < y) {    // FrackMan is below
        while (true) {
            if (pY == y)        // found FrackMan
                return GraphObject::down;
            if (isSpotBlocked(x, y - 1))    // path blocked
                break;
            y--;
        }
    }
    
    if (pX == x && pY > y) {    // FrackMan is above
        while (true) {
            if (pY == y)        // found FrackMan
                return GraphObject::up;
            if (isSpotBlocked(x, y + 1))    // path blocked
                break;
            y++;
        }
    }
    
    if (pY == y && pX > x) {    // FrackMan is to right
        while (true) {
            if (pX == x)        // found FrackMan
                return GraphObject::right;
            if (isSpotBlocked(x + 1, y))    // path blocked
                break;
            x++;
        }
    }
    
    if (pY == y && pX < x) {    // FrackMan is to left
        while (true) {
            if (pX == x)        // found FrackMan
                return GraphObject::left;
            if (isSpotBlocked(x - 1, y))    // path blocked
                break;
            x--;
        }
    }
    
    // no straight-line path to FrackMan
    return GraphObject::none;
    
}

bool StudentWorld::stepTowardFrackMan(Protester* protester) {
    GraphObject::Direction dir = directionToFrackMan(protester);
    int x = protester->getX(), y = protester->getY();
    bool canStep = false;
    
    // Can step toward FrackMan
    if (dir != GraphObject::none && radius(x, y, m_player->getX(), m_player->getY()) > 4) {
        canStep = true;
        protester->setDirection(dir);
        switch (dir) {
            case GraphObject::up:
                protester->moveTo(x, y+1);
                markMap(x, y+1, m_exitMap[x][y]+1);
                break;
            case GraphObject::right:
                protester->moveTo(x+1, y);
                markMap(x+1, y, m_exitMap[x][y]+1);
                break;
            case GraphObject::down:
                protester->moveTo(x, y-1);
                markMap(x, y-1, m_exitMap[x][y]+1);
                break;
            case GraphObject::left:
                protester->moveTo(x-1, y);
                markMap(x-1, y, m_exitMap[x][y]+1);
                break;
            case GraphObject::none:
                break;
        }
    }
    
    protester->setToRest();
    return canStep;
}

GraphObject::Direction StudentWorld::pickNewDirection(Protester* protester) {
    
    // Randomly pick a new direction that doesn't have its spot blocked
    while (true) {
        GraphObject::Direction newDir = GraphObject::Direction(randInt(1, 5));
        
        int x = protester->getX(), y = protester->getY();
        switch (newDir) {
            case GraphObject::up:
                if(!isSpotBlocked(x, y+1))
                    return newDir;
                break;
            case GraphObject::right:
                if(!isSpotBlocked(x+1, y))
                    return newDir;
                break;
            case GraphObject::down:
                if(!isSpotBlocked(x, y-1))
                    return newDir;
                break;
            case GraphObject::left:
                if(!isSpotBlocked(x-1, y))
                    return newDir;
                break;
            case GraphObject::none:
                break;
        }
    }
}

bool StudentWorld::movePerpendicularly(Protester* protester) {
    GraphObject::Direction dir = protester->getDirection();
    int x = protester->getX(), y = protester->getY();
    bool canTurn = false;
    bool currentlyVertical = true;
    
    // Check if the protester can turn perpendicularly
    switch (dir) {
        case GraphObject::up:
        case GraphObject::down:
            if (!isSpotBlocked(x-1, y) || !isSpotBlocked(x+1, y))
                canTurn = true;
            break;
        case GraphObject::right:
        case GraphObject::left:
            if (!isSpotBlocked(x, y-1) || !(isSpotBlocked(x, y+1))) {
                canTurn = true;
                currentlyVertical = false;
            }
            break;
        default:
            break;
    }
    
    if (protester->ticksSinceLastTurn() <= 200)
        canTurn = false;
    
    if (canTurn) {
        if (currentlyVertical) {        // move left or right now
            if (randInt(0, 2))
                protester->setDirection(GraphObject::right);
            else
                protester->setDirection(GraphObject::left);
        } else {                        // move up or down now
            if (randInt(0, 2))
                protester->setDirection(GraphObject::up);
            else
                protester->setDirection(GraphObject::down);
        }
        protester->reRandomizeMoveSquares();
        protester->resetTurnTicks();
    }
    
    return canTurn;
}

bool StudentWorld::takeAStep(Protester* protester) {
    int x = protester->getX(), y = protester->getY();
    GraphObject::Direction dir = protester->getDirection();
    
    switch (dir) {
        case GraphObject::up:
            if (!isSpotBlocked(x, y+1)) {
                protester->moveTo(x, y+1);
                markMap(x, y+1, m_exitMap[x][y]+1);
                // Mark exit map
                return true;
            }
            break;
        case GraphObject::right:
            if (!isSpotBlocked(x+1, y)) {
                protester->moveTo(x+1, y);
                markMap(x+1, y, m_exitMap[x][y]+1);
                // Mark exit map
                return true;
            }
            break;
        case GraphObject::down:
            if (!isSpotBlocked(x, y-1)) {
                protester->moveTo(x, y-1);
                markMap(x, y-1, m_exitMap[x][y]+1);
                // Mark exit map
                return true;
            }
            break;
        case GraphObject::left:
            if (!isSpotBlocked(x-1, y)) {
                protester->moveTo(x-1, y);
                markMap(x-1, y, m_exitMap[x][y]+1);
                // Mark exit map
                return true;
            }
            break;
        default:
            break;
    }
    return false;
}

bool StudentWorld::followFrackMan(Protester *protester) {
    
    // Implement maze search with FrackMan as end point
    return true;
}

void StudentWorld::annoyFrackMan() {
    m_player->getAnnoyed(2);
}

// ------------------------ //
// --------- DIRT --------- //
// ------------------------ //

bool StudentWorld::isThereDirt(int x, int y) {
    
    if (y < 0 || y > 59 || x < 0 || x > 63) return false;
    
    return m_dirt[x][y] != nullptr;
}

void StudentWorld::destroyDirt(int x, int y) {
    delete m_dirt[x][y];
    m_dirt[x][y] = nullptr;
}

// --------------------------- //
// --------- BOULDER --------- //
// --------------------------- //

bool StudentWorld::willBoulderFall(Boulder *boulder) {
    int x = boulder->getX(), y = boulder->getY();
    bool dirtBelow = false;
    
    // Check for dirt beneath boulder
    for (int i = x; i < x + 4; i++) {
        if (isThereDirt(i, y - 1))
            dirtBelow = true;
    }
    return !dirtBelow;
}

bool StudentWorld::dropBoulderOrTick(Boulder * boulder) {
    if (boulder->ticksWaited() == 30) {
        boulder->setState(Boulder::falling);
        playSound(SOUND_FALLING_ROCK);
        return true;
    } else {
        boulder->incTicks();
        return false;
    }
}

bool StudentWorld::willBoulderCrash(Boulder* boulder) {
    
    int x = boulder->getX(), y = boulder->getY();
    
    // Will crash into boulder
    for (int i = x - 3; i < x + 7; i++) {
        for (int j = y - 4; j < y; j++) {
            if (whatIsHere(i, j) == Actor::boulder)
                return true;
        }
    }
    
    // Will crash into dirt
    for (int i = 0; i < 4; i++) {
        if (whatIsHere(x + i, y) == Actor::dirt) {
            return true;
        }
    }
    
    return false;
}

bool StudentWorld::moveBoulder(Boulder* boulder) {
    if (boulder->crashed()) {
        boulder->setDead();
        return false;
    } else {
        boulder->moveTo(boulder->getX(), boulder->getY() - 1);
        return true;
    }
}

bool StudentWorld::crushLiveActorBelow(Boulder* boulder) {
    
    int x = boulder->getX(), y = boulder->getY();
    vector<Actor*>::iterator it = m_actors.begin();
    bool actorCrushed = false;
    
    // Check if FrackMan will get crushed
    int playerX = m_player->getX(), playerY = m_player->getY();
    if (radius(x, y, playerX, playerY) <= 3) {
        m_player->getAnnoyed(100);
        actorCrushed = true;
    }
    
    else {
        // Check if protesters will get crushed
        while (it != m_actors.end()) {
            if ((*it)->isAlive() && (*it)->canGetCrushed() && radius(x, y, (*it)->getX(), (*it)->getY()) <= 3) {
                if((*it)->getAnnoyed(100)) // protester died from boulder
                    increaseScore(500);
                actorCrushed = true;
            }
            
            it++;
        }
    }
    
    return actorCrushed;
}

// -------------------------- //
// --------- SQUIRT --------- //
// -------------------------- //

bool StudentWorld::squirtProtesters(Squirt* squirt) {
    
    int x = squirt->getX(),y = squirt->getY();
    vector<Actor*>::iterator it = m_actors.begin();
    bool protesterAnnoyed = false;
    
    while (it != m_actors.end()) {
        if ((*it) != nullptr && ((*it)->getName() == Actor::protester || (*it)->getName() == Actor::hardcore) && (*it)->isAlive() && radius(x, y, (*it)->getX(), (*it)->getY()) <= 3) {
            if((*it)->getAnnoyed(2))
                increaseScore(100);
            protesterAnnoyed = true;
        } it++;
    }
    
    return protesterAnnoyed;
}

void StudentWorld::spawnSquirt() {
    int x = m_player->getX(), y = m_player->getY();
    
    playSound(SOUND_PLAYER_SQUIRT);
    
    switch (m_player->getDirection()) {
        case Actor::up:
            if (!isSpotBlocked(x, y + 4))
                m_actors.push_back(new Squirt(x, y + 4, m_player->getDirection(), this));
            break;
        case Actor::right:
            if (!isSpotBlocked(x + 4, y))
                m_actors.push_back(new Squirt(x + 4, y, m_player->getDirection(), this));
            break;
        case Actor::down:
            if (!isSpotBlocked(x, y - 4))
                m_actors.push_back(new Squirt(x, y - 4, m_player->getDirection(), this));
            break;
        case Actor::left:
            if (!isSpotBlocked(x - 4, y))
                m_actors.push_back(new Squirt(x - 4, y, m_player->getDirection(), this));
            break;
        default:
            break;
    }
}

// ------------------------ //
// --------- GOLD --------- //
// ------------------------ //

bool StudentWorld::getPickedUpByProtester(Gold* gold) {
    vector<Actor*>::iterator it = m_actors.begin();
    
    while (it != m_actors.end()) {
        if ((*it)->getName() == Actor::protester && radius(gold->getX(), gold->getY(), (*it)->getX(), (*it)->getY()) <= 3) {
            Protester* a = dynamic_cast<Protester*>((*it));
            if (a != nullptr)
                a->getBribed();
            return true;
        }
        it++;
    }
    return false;
}

// --------------------------- //
// --------- PICKUPS --------- //
// --------------------------- //

bool StudentWorld::makeVisibleIfInRadius(Pickup *pickup, int rad) {
    
    if (!pickup->isPickupVisible() &&
        radius(pickup->getX(), pickup->getY(), m_player->getX(), m_player->getY()) <= rad) {
        pickup->makePickupVisible(true);
        return true;
    } return false;
}

bool StudentWorld::pickupPickupIfNearby(Pickup* pickup) {
    if(radius(pickup->getX(), pickup->getY(), m_player->getX(), m_player->getY()) <= 3) {
        pickup->setDead();
        return true;
    }
    return false;
}

void StudentWorld::frackManFoundItem(Pickup *pickup) {
    switch (pickup->getName()) {
        case Actor::barrel:
            m_barrelsLeft--;
            increaseScore(1000);
            playSound(SOUND_FOUND_OIL);
            break;
        case Actor::gold:
            m_player->incNuggets();
            increaseScore(10);
            playSound(SOUND_GOT_GOODIE);
            break;
        case Actor::sonarkit:
            m_player->incSonars();
            increaseScore(75);
            playSound(SOUND_GOT_GOODIE);
            break;
        case Actor::waterpool:
            m_player->refillSquirtGun();
            break;
        default:
            break;
    }
}

// ------------------------------------- //
// ------------------------------------- //
// --------- PRIVATE FUNCTIONS --------- //
// ------------------------------------- //
// ------------------------------------- //

// -------------------------------- //
// --------- HOUSEKEEPING --------- //
// -------------------------------- //

void StudentWorld::setDisplayText() {
    int score = getScore();
    int level = getLevel();
    int lives = getLives();
    int health = m_player->health();
    int squirts = m_player->squirts();
    int gold = m_player->nuggets();
    int sonar = m_player->sonars();
    int barrelsRemaining = barrelsLeft();
    // Next, create a string from your statistics, of the form:
    // “Scr: 0321000 Lvl: 52 Lives: 3 Hlth: 80% Water: 20 Gld: 3 Sonar: 1 Oil Left: 2”
    string s = formatDisplayText(score, level, lives, health, squirts, gold, sonar, barrelsRemaining);
    // Finally, update the display text at the top of the screen with your // newly created stats
    setGameStatText(s); // calls our provided GameWorld::setGameStatText
}

bool StudentWorld::playerDied() {
    return !(m_player->isAlive());
}

bool StudentWorld::finishedLevel() {
    return barrelsLeft() == 0;
}

int StudentWorld::barrelsLeft() {
    return m_barrelsLeft;
}

string StudentWorld::formatDisplayText(int score, int level, int lives, int health, int squirts, int gold, int sonar, int barrelsRemaining) {
    string text;
    
    string scoreText = "Scr: " + string(6 - to_string(score).length(), '0') + to_string(score) + "  ";
    string levelText = "Lvl: " + string(2 - to_string(level).length(), ' ') + to_string(level) + "  ";
    string livesText = "Lives: " + to_string(lives) + "  ";
    string healthText = "Hlth: " + string(3 - to_string(health*10).length(), ' ') + to_string(health*10) + "%  ";
    string squirtsText = "Wtr: " + string(2 - to_string(squirts).length(), ' ') + to_string(squirts) + "  ";
    string goldText = "Gld: " + string(2 - to_string(gold).length(), ' ') + to_string(gold) + "  ";
    string sonarText = "Sonar: " + string(2 - to_string(sonar).length(), ' ') + to_string(sonar) + "  ";
    string barrelsText = "Oil Left: " + string(2 - to_string(barrelsRemaining).length(), ' ') + to_string(barrelsRemaining);
    
    text = scoreText + levelText + livesText + healthText + squirtsText + goldText + sonarText + barrelsText;
    return text;
}

// ----------------------------------- //
// --------- WORLD AWARENESS --------- //
// ----------------------------------- //

bool StudentWorld::isMineShaftRegion(int x, int y) {
    if ((x >= 30 && x <= 33) && (y >= 4 && y <= 59)) return true;
    return false;
}

bool StudentWorld::isBoulderHere(int x, int y) {
    vector<Actor*>::iterator it = m_actors.begin();
    
    // For each boulder
    while (it != m_actors.end()) {
        if ((*it)->getName() == Actor::boulder) {
            int bX = (*it)->getX();
            int bY = (*it)->getY();
            
            // Check if the boulders 4x4 box overlaps the target's 4x4 box
            for (int a = bX; a < bX + 4; a++)
                for (int b = bY; b < bY + 4; b++)
                    for (int i = x; i < x + 4; i++)
                        for (int j = y; j < y + 4; j++)
                            if (a == i && b == j)
                                return true;
        }
        it++;
    }
    return false;
}

bool StudentWorld::canPlacePickupHere(int x, int y) {
    if (((x >= 27 && x <= 33) && (y >= 0)) || y > 56) return false;
    return true;
    
}

bool StudentWorld::isRadiusClear(int x, int y, int r) {
    vector<Actor*>::iterator it = m_actors.begin();
    
    while (it != m_actors.end()) {
        double rad = radius(x, y, (*it)->getX(), (*it)->getY());
        if (rad <= r)       // found something in radius
            return false;
        
        it++;
    }
    return true;    // no objects found to be within radius
    
}

bool StudentWorld::canPlaceWaterHere(int x, int y) {
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            if (isThereDirt(x+i, y+j))
                return false;
    return true;
}

int StudentWorld::protesterCount() {
    vector<Actor*>::iterator it = m_actors.begin();
    int count = 0;
    
    while (it != m_actors.end()) {
        if (((*it)->getName() == Actor::protester || (*it)->getName() == Actor::hardcore) && (*it)->isAlive()) {
            count++;
        }
        it++;
    }
    return count;
}

void StudentWorld::getShortestRoute(int &x, int &y, GraphObject::Direction &dir) {
    
    vector<int> options = {m_exitMap[x][y+1], m_exitMap[x][y-1], m_exitMap[x+1][y], m_exitMap[x-1][y]};
    
    int minIndex = 0;
    vector<int>::iterator minIt;
    int i = 0;
    
    do {
        for(vector<int>::iterator it = options.begin(); it != options.end();i++)
            if ((*it) != -1) {
                
                if ((*it) < options[minIndex]) {
                    minIndex = i;
                    minIt = it;
                }
                
                
//                if (!indexSet) {
//                    index = i;
//                    indexSet = true;
//                } else {
//                    if (options[i] < options[index]) {
//                        index = i;
//                    }
//                }
            }
        
        // Remove the just chosen option in case its path is blocked
        // Will need to re-select from vector of options
        options.erase(minIt);
        
        switch (minIndex) {
            case 0:
                y = y + 1;
                dir = GraphObject::up;
                break;
            case 1:
                y = y - 1;
                dir = GraphObject::down;
                break;
            case 2:
                x = x + 1;
                dir = GraphObject::right;
                break;
            case 3:
                x = x - 1;
                dir = GraphObject::left;
                break;
            default:
                
                break;
        }
    } while (isSpotBlocked(x, y));
}
