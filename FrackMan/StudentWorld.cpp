#include "StudentWorld.h"
#include <string>
#include <algorithm>
#include <iomanip>

using namespace std;

GameWorld* createStudentWorld(string assetDir) {
    return new StudentWorld(assetDir);
}

StudentWorld::~StudentWorld() {
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
}

// --------------------------------------- //
// --------- MAIN GAME FUNCTIONS --------- //
// --------------------------------------- //

int StudentWorld::init() {
    
    int currentLevel = getLevel();
    
    m_maxProtesters = min(15.0, 2 + int(currentLevel) * 1.5);
    m_TICKSBETWEENSPAWNS = max(25, 200 - int(currentLevel));
    
    // Construct/initialize FrackMan
    m_player = new FrackMan(this);
    
    // Construct/initialize dirt
    for (int x = 0; x < 64; x++) {
        for (int y = 0; y < 64; y++) {
            
            // Omit dirt on top and mineshaft
            if (y >= 60 || isMineShaftRegion(x, y)) {
                m_dirt[x][y] = nullptr;
                
                if (y >= 61) {
                    m_exitMaze[x][y] = DISCOVERED;
                }
                
                else if (x > 60) {
                    m_exitMaze[x][y] = DISCOVERED;
                } else {
                    m_exitMaze[x][y] = OPEN;
                }
                
                
            }
            else {
                m_dirt[x][y] = new Dirt(x, y, this);
                m_exitMaze[x][y] = DISCOVERED;
            }
        }
    }
    
    // Construct/initialize boulders
    int boulders = min(currentLevel / 2 + 2, 6);
    for (int k = 0; k < boulders;) {
        int randX = randInt(0, 60);
        int randY = randInt(20, 57);
        if (isRadiusClear(randX, randY, MAX_RADIUS) && canPlacePickupHere(randX, randY)) {
            m_actors.push_back(new Boulder(randX, randY, this));
            
            // Remove dirt behind this boulder
            for (int i = 0; i < 4; i++)
                for (int j = 0; j < 4; j++)
                    destroyDirt(randX + i, randY + j);
            k++;
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
//    m_actors.push_back(new Protester(60, 60, IID_PROTESTER, this));
    m_actors.push_back(new HardCoreProtester(60, 60, this));
    
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
            m_actors.push_back(new HardCoreProtester(60, 60, this));
        else
            // Spawn regular
            m_actors.push_back(new Protester(60, 60, IID_PROTESTER, this));
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
    
    // For testing purposes only: print exit maze
    for (int y = 63; y >= 0; y--) {
        for (int x = 0; x < 64; x++) {
            cerr << m_exitMaze[x][y];
        }
        cerr << endl;
    }
    cerr << endl;
}

// ------------------------------------- //
// --------- GENERAL FUNCTIONS --------- //
// ------------------------------------- //

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

bool StudentWorld::destroyDirtUnderPlayer() {
    int playerX = m_player->getX(), playerY = m_player->getY();
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
                }
                else if (whatIsBlockingPath(pX, pY, dir) == Actor::wall)
                    m_player->moveTo(pX, pY);
                break;
            case KEY_PRESS_UP:
                if (dir != GraphObject::up)
                    m_player->setDirection(GraphObject::up);
                else if (whatIsBlockingPath(pX, pY, dir) == Actor::nothing) {
                    m_player->moveTo(pX, pY + 1);
                }
                else if (whatIsBlockingPath(pX, pY, dir) == Actor::wall)
                    m_player->moveTo(pX, pY);
                break;
            case KEY_PRESS_RIGHT:
                if (dir != GraphObject::right)
                    m_player->setDirection(GraphObject::right);
                else if (whatIsBlockingPath(pX, pY, dir) == Actor::nothing) {
                    m_player->moveTo(pX + 1, pY);
                }
                else if (whatIsBlockingPath(pX, pY, dir) == Actor::wall)
                    m_player->moveTo(pX, pY);
                break;
            case KEY_PRESS_DOWN:
                if (dir != GraphObject::down)
                    m_player->setDirection(GraphObject::down);
                else if (whatIsBlockingPath(pX, pY, dir) == Actor::nothing) {
                    m_player->moveTo(pX, pY - 1);
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
        for (int i = m_player->getX(); i < m_player->getX() + 4; i++) {
            for (int j = m_player->getY(); j < m_player->getY() + 4; j++) {
                markAsOpen(i, j);
            }
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

//    int x = protester->getX(), y = protester->getY();
    
    GraphObject::Direction dir = getProtesterDirectionTo(protester, 60, 60);
    
    if (protester->getDirection() != dir) {
        protester->setDirection(dir);
    }
    else
        takeAStep(protester);
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
    
    switch (protester->getDirection()) {
        case GraphObject::up:
            if (pX == x && pY == y + 4)
                return GraphObject::none;
            break;
        case GraphObject::right:
            if (pX == x + 4 && pY == y)
                return GraphObject::none;
            break;
        case GraphObject::left:
            if (pX == x - 4 && pY == y)
                return GraphObject::none;
            break;
        case GraphObject::down:
            if (pX == x && pY == y - 4)
                return GraphObject::none;
            break;
            
        default:
            break;
    }
    
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
        protester->resetNumSquaresToMove();
        protester->setDirection(dir);
        switch (dir) {
            case GraphObject::up:
                protester->moveTo(x, y+1);
                break;
            case GraphObject::right:
                protester->moveTo(x+1, y);
                break;
            case GraphObject::down:
                protester->moveTo(x, y-1);
                break;
            case GraphObject::left:
                protester->moveTo(x-1, y);
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
                // Mark exit map
                return true;
            }
            break;
        case GraphObject::right:
            if (!isSpotBlocked(x+1, y)) {
                protester->moveTo(x+1, y);
                // Mark exit map
                return true;
            }
            break;
        case GraphObject::down:
            if (!isSpotBlocked(x, y-1)) {
                protester->moveTo(x, y-1);
                // Mark exit map
                return true;
            }
            break;
        case GraphObject::left:
            if (!isSpotBlocked(x-1, y)) {
                protester->moveTo(x-1, y);
                // Mark exit map
                return true;
            }
            break;
        default:
            break;
    }
    return false;
}

bool StudentWorld::trackFrackMan(Protester *protester) {
    
    int stepsToFrackMan = 0;
    
    Protester p(protester->getX(), protester->getY(), IID_PROTESTER,  this);
    p.setDirection(protester->getDirection());
    
    while (!(p.getX() == m_player->getX() && p.getY() == m_player->getY())) {
        GraphObject::Direction dir = getProtesterDirectionTo(&p, m_player->getX(), m_player->getY());
        if (p.getDirection() != dir) {
            p.setDirection(dir);
        } else {
            takeAStep(&p);
            stepsToFrackMan++;
        }
    }
    
    if (stepsToFrackMan <= protester->trackingRange()) {
        GraphObject::Direction dir = getProtesterDirectionTo(protester, m_player->getX(), m_player->getY());
        if (protester->getDirection() != dir) {
            protester->setDirection(dir);
        } else {
            takeAStep(protester);
            protester->setToRest();
        }
        return true;
    }
    return false;
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
    
//    // Will crash into boulder
//    for (int i = x - 3; i < x + 7; i++)
//        for (int j = y - 4; j < y; j++)
////            if (whatIsHere(i, j) == Actor::boulder)
//            if (
//                return true;
//    
//    // Will crash into dirt
//    for (int i = 0; i < 4; i++)
//        if (whatIsHere(x + i, y) == Actor::dirt)
//            return true;
    vector<Actor*>::iterator it = m_actors.begin();
    
    while (it != m_actors.end()) {
        if ((*it)->breaksBoulder()) {
            for (int i = x - 3; i < x + 7; i++)
                for (int j = y - 4; j < y; j++)
                    if ((*it)->getX() == i && (*it)->getY() == j)
                        return true;
        }
        it++;
    }
    // Will crash into dirt
    for (int i = 0; i < 4; i++)
        if (isThereDirt(x + i, y))
            return true;
    
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
    
    else
        // Check if protesters will get crushed
        while (it != m_actors.end()) {
            if ((*it)->isAlive() && (*it)->canGetCrushed() && radius(x, y, (*it)->getX(), (*it)->getY()) <= 3) {
                if((*it)->getAnnoyed(100)) // protester died from boulder
                    increaseScore(500);
                actorCrushed = true;
            }
            
            it++;
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
        if ((*it) != nullptr && (*it)->canGetSquirted() && (*it)->isAlive() && radius(x, y, (*it)->getX(), (*it)->getY()) <= 3) {
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

// ------------------------------------------------------- //
// ------------------ PRIVATE FUNCTIONS ------------------ //
// ------------------------------------------------------- //

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
    
    // Don't spawn too high
    if (y > 60)
        return false;
    
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

GraphObject::Direction StudentWorld::getProtesterDirectionTo(Protester* protester, int ex, int ey) {
    int endX = protester->getX(), endY = protester->getY();
    
    queue<Location> mapQueue;
    
    char maze[64][64];
    
    // Re-make temporary maze for this tick (inefficient)
    for (int i = 0; i < 64; i++)
        for (int j = 0; j < 64; j++)
            maze[i][j] = m_exitMaze[i][j];
    
    Location start(ex, ey, GraphObject::none);
    mapQueue.push(start);
    
    while (!mapQueue.empty()) {
        Location curr = mapQueue.front();
        mapQueue.pop();
        
        int currX = curr.x();
        int currY = curr.y();
        
        if (isSpotBlocked(currX, currY))
            continue;
        
        if (currX == endX && currY == endY)
            return curr.dir();
        
        // Check up
        const char up = maze[currX][currY+1];
        if (up == OPEN) {
            mapQueue.push(Location(currX, currY+1, GraphObject::down));
            maze[currX][currY+1] = DISCOVERED;
        }
        
        // Check right
        const char right = maze[currX+1][currY];
        if (right == OPEN) {
            mapQueue.push(Location(currX+1, currY, GraphObject::left));
            maze[currX+1][currY] = DISCOVERED;
        }
        
        // Check down
        const char down = maze[currX][currY-1];
        if (down == OPEN) {
            mapQueue.push(Location(currX, currY-1, GraphObject::up));
            maze[currX][currY-1] = DISCOVERED;
        }
        
        // Check left
        const char left = maze[currX-1][currY];
        if (left == OPEN) {
            mapQueue.push(Location(currX-1, currY, GraphObject::right));
            maze[currX-1][currY] = DISCOVERED;
        }
    }
    return GraphObject::none;

}

void StudentWorld::markAsOpen(int x, int y) {
    m_exitMaze[x][y] = OPEN;
}