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
    m_actors.push_back(new Protester(60, 60, IID_PROTESTER, this));
    
    // Initialize various member variables
    m_protesterCount = 1;
    m_maxProtesters = min(15.0, 2 + int(currentLevel) * 1.5);
    m_TICKSBETWEENSPAWNS = max(25, 200 - int(currentLevel));
    m_ticksSinceProtesterSpawned = 0;
    
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

bool StudentWorld::isFourByFourTaken(int x, int y) {
    
    if (x == -1 || x == 61 || y == -1 || y == 61)
        return true;
    
    
    if (isThereDirt(x, y) || isThereBoulder(x, y))
        return true;
    
    return false;
}

// ---------------------------- //
// --------- FRACKMAN --------- //
// ---------------------------- //

bool StudentWorld::isFrackManPathBlocked() {
    
    int x = m_player->getX(), y = m_player->getY();
    GraphObject::Direction dir = m_player->getDirection();
    
    // Check for wall blocking FrackMan
    bool wallBlocking = false;
    switch (dir) {
        case GraphObject::left:
            if (x == 0 )
                wallBlocking = true;
            break;
        case GraphObject::up:
            if (y == 60)
                wallBlocking = true;
            break;
        case GraphObject::right:
            if (x == 60)
                wallBlocking = true;
            break;
        case GraphObject::down:
            if (y == 0)
                wallBlocking = true;
            break;
        case GraphObject::none:
            break;
    }
    if (wallBlocking) return true;
    
    // Check for boulder blocking FrackMan
    bool boulderBlocking = false;
    vector<Actor*>::iterator it = m_actors.begin();
    while (it != m_actors.end()) {
        if ((*it)->stopsFrackMan()) {
            switch (dir) {
                case GraphObject::up:
                    if (isThereBoulder(x, y+1))
                        return true;
                    break;
                case GraphObject::right:
                    if (isThereBoulder(x+1, y))
                        return true;
                    break;
                case GraphObject::down:
                    if (isThereBoulder(x, y-1))
                        return true;
                    break;
                case GraphObject::left:
                    if (isThereBoulder(x-1, y))
                        return true;
                    break;
                default:
                    break;
            }
        }
        it++;
    }
    if (boulderBlocking) return true;
    
    return false;
}

bool StudentWorld::destroyDirtUnderPlayer() {
    int playerX = m_player->getX(), playerY = m_player->getY();
    bool dug = false;

    if (isThereDirt(playerX, playerY)) {
        dug = true;
        for (int x = playerX; x < playerX + 4; x++)
            for (int y = playerY; y < playerY + 4; y++) {
                destroyDirt(x, y);
                markAsOpen(x, y);
            }
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
                else if (isFrackManPathBlocked())
                    m_player->moveTo(pX, pY);
                else
                    m_player->moveTo(pX - 1, pY);
                break;
            case KEY_PRESS_UP:
                if (dir != GraphObject::up)
                    m_player->setDirection(GraphObject::up);
                else if (isFrackManPathBlocked())
                    m_player->moveTo(pX, pY);
                else
                    m_player->moveTo(pX, pY + 1);
                break;
            case KEY_PRESS_RIGHT:
                if (dir != GraphObject::right)
                    m_player->setDirection(GraphObject::right);
                else if (isFrackManPathBlocked())
                    m_player->moveTo(pX, pY);
                else
                    m_player->moveTo(pX + 1, pY);
                break;
            case KEY_PRESS_DOWN:
                if (dir != GraphObject::down)
                    m_player->setDirection(GraphObject::down);
                else if (isFrackManPathBlocked())
                    m_player->moveTo(pX, pY);
                else
                    m_player->moveTo(pX, pY - 1);
                break;
            case KEY_PRESS_ESCAPE:
                m_player->getAnnoyed(10);
                m_player->setDead();
                decLives();
                break;
            case KEY_PRESS_SPACE:
                spawnSquirt();
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

bool StudentWorld::dropGold(){
    if (m_player->nuggets() > 0) {
        m_actors.push_back(new Gold(m_player->getX(), m_player->getY(), Gold::protester, Gold::temporary, this));
        m_player->decNuggets();
        return true;
    }
    return false;
}

bool StudentWorld::useSonar() {
    if (m_player->sonars() > 0) {
        m_player->decSonars();
        playSound(SOUND_SONAR);
        
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
        return true;
    }
    return false;
}

// ----------------------------- //
// --------- PROTESTER --------- //
// ----------------------------- //

void StudentWorld::protesterLeaveMap(Protester* protester) {
    
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

GraphObject::Direction StudentWorld::getLineDirToFrackMan(Protester *protester) {
    
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
            if (isFourByFourTaken(x, y - 1))    // path blocked
                break;
            y--;
        }
    }
    
    if (pX == x && pY > y) {    // FrackMan is above
        while (true) {
            if (pY == y)        // found FrackMan
                return GraphObject::up;
            if (isFourByFourTaken(x, y + 1))    // path blocked
                break;
            y++;
        }
    }
    
    if (pY == y && pX > x) {    // FrackMan is to right
        while (true) {
            if (pX == x)        // found FrackMan
                return GraphObject::right;
            if (isFourByFourTaken(x + 1, y))    // path blocked
                break;
            x++;
        }
    }
    
    if (pY == y && pX < x) {    // FrackMan is to left
        while (true) {
            if (pX == x)        // found FrackMan
                return GraphObject::left;
            if (isFourByFourTaken(x - 1, y))    // path blocked
                break;
            x--;
        }
    }
    
    // no straight-line path to FrackMan
    return GraphObject::none;
    
}

bool StudentWorld::stepTowardFrackMan(Protester* protester) {
    GraphObject::Direction dir = getLineDirToFrackMan(protester);
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
                if(!isFourByFourTaken(x, y+1))
                    return newDir;
                break;
            case GraphObject::right:
                if(!isFourByFourTaken(x+1, y))
                    return newDir;
                break;
            case GraphObject::down:
                if(!isFourByFourTaken(x, y-1))
                    return newDir;
                break;
            case GraphObject::left:
                if(!isFourByFourTaken(x-1, y))
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
            if (!isFourByFourTaken(x-1, y) || !isFourByFourTaken(x+1, y))
                canTurn = true;
            break;
        case GraphObject::right:
        case GraphObject::left:
            if (!isFourByFourTaken(x, y-1) || !(isFourByFourTaken(x, y+1))) {
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
            if (!isFourByFourTaken(x, y+1)) {
                protester->moveTo(x, y+1);
                // Mark exit map
                return true;
            }
            break;
        case GraphObject::right:
            if (!isFourByFourTaken(x+1, y)) {
                protester->moveTo(x+1, y);
                // Mark exit map
                return true;
            }
            break;
        case GraphObject::down:
            if (!isFourByFourTaken(x, y-1)) {
                protester->moveTo(x, y-1);
                // Mark exit map
                return true;
            }
            break;
        case GraphObject::left:
            if (!isFourByFourTaken(x-1, y)) {
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

//bool StudentWorld::trackFrackMan(Protester *protester) {
//    
//    int stepsToFrackMan = 0;
//    
//    Protester p(protester->getX(), protester->getY(), IID_PROTESTER,  this);
//    p.setDirection(protester->getDirection());
//    
//    while (!(p.getX() == m_player->getX() && p.getY() == m_player->getY())) {
//        GraphObject::Direction dir = getProtesterDirectionTo(&p, m_player->getX(), m_player->getY());
//        if (p.getDirection() != dir) {
//            p.setDirection(dir);
//        } else {
//            takeAStep(&p);
//            stepsToFrackMan++;
//        }
//    }
//    
//    if (stepsToFrackMan <= protester->trackingRange()) {
//        GraphObject::Direction dir = getProtesterDirectionTo(protester, m_player->getX(), m_player->getY());
//        if (protester->getDirection() != dir) {
//            protester->setDirection(dir);
//        } else {
//            takeAStep(protester);
//            protester->setToRest();
//        }
//        return true;
//    }
//    return false;
//}

void StudentWorld::annoyFrackMan() {
    m_player->getAnnoyed(2);
}

// ------------------------ //
// --------- DIRT --------- //
// ------------------------ //

bool StudentWorld::isThereDirt(int x, int y) {
    
    if (y < 0 || y > 59 || x < 0 || x > 63) return false;
    
    // Check 4x4 box for dirt
    bool isThereDirt = false;
    
    for (int i = x; i < x + 4; i++)
        for (int j = y; j < y + 4; j++)
             if (m_dirt[i][j] != nullptr)
                 isThereDirt = true;
    
    return isThereDirt;
}

void StudentWorld::destroyDirt(int x, int y) {
    delete m_dirt[x][y];
    m_dirt[x][y] = nullptr;
}

// --------------------------- //
// --------- BOULDER --------- //
// --------------------------- //

bool StudentWorld::willBoulderFall(Boulder* boulder) {
    int x = boulder->getX(), y = boulder->getY();
//    bool dirtBelow = false;
//    
//    // Check for dirt beneath boulder
//    if (isThereDirt(x, y - 1))
//        dirtBelow = true;
    return !isThereDirt(x, y-1);
}

bool StudentWorld::dropBoulderOrTick(Boulder* boulder) {
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
    
    // Check if an actor that breaks a boulder is underneath
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

    if (isThereDirt(x, y))
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

bool StudentWorld::spawnSquirt() {
    
    if (m_player->squirts() <= 0) return false;
    
    m_player->decSquirts();
    int x = m_player->getX(), y = m_player->getY();
    
    playSound(SOUND_PLAYER_SQUIRT);
    
    switch (m_player->getDirection()) {
        case Actor::up:
            if (!isFourByFourTaken(x, y + 4))
                m_actors.push_back(new Squirt(x, y + 4, m_player->getDirection(), this));
            break;
        case Actor::right:
            if (!isFourByFourTaken(x + 4, y))
                m_actors.push_back(new Squirt(x + 4, y, m_player->getDirection(), this));
            break;
        case Actor::down:
            if (!isFourByFourTaken(x, y - 4))
                m_actors.push_back(new Squirt(x, y - 4, m_player->getDirection(), this));
            break;
        case Actor::left:
            if (!isFourByFourTaken(x - 4, y))
                m_actors.push_back(new Squirt(x - 4, y, m_player->getDirection(), this));
            break;
        default:
            break;
    }
    return true;
}

bool StudentWorld::moveSquirt(Squirt* squirt) {
    
    int x = squirt->getX(), y = squirt->getY();
    
    switch (squirt->getDirection()) {
        case GraphObject::up:
            if (!isFourByFourTaken(x, y + 4))
                squirt->moveTo(x, y + 1);
            else
                squirt->setDead();
            break;
        case GraphObject::right:
            if (!isFourByFourTaken(x + 4, y))
                squirt->moveTo(x + 1, y);
            else
                squirt->setDead();
            break;
        case GraphObject::down:
            if (!isFourByFourTaken(x, y - 1))
                squirt->moveTo(x, y - 1);
            else
                squirt->setDead();
            break;
        case GraphObject::left:
            if (!isFourByFourTaken(x - 1, y))
                squirt->moveTo(x - 1, y);
            else
                squirt->setDead();
            break;
        default:
            break;
    }
    
    // Returns true if squirt has moved
    return ((x != squirt->getX()) || y != squirt->getY());
}

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

// ------------------------ //
// --------- GOLD --------- //
// ------------------------ //

bool StudentWorld::getPickedUpByProtester(Gold* gold) {
    int goldX = gold->getX(), goldY = gold->getY();
    bool gotPickedUp = false;
    
    vector<Actor*>::iterator it = m_actors.begin();
    while (it != m_actors.end()) {
        if ((*it)->canPickUpGold() && radius(goldX, goldY, (*it)->getX(), (*it)->getY()) <= 3) {
            (*it)->getBribed();
            gotPickedUp = true;
        }
        it++;
    }
    return gotPickedUp;
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
    pickup->getPickedUp();
}

// --------------------------- //

void StudentWorld::pickUpBarrel() {
    m_barrelsLeft--;
    increaseScore(1000);
    playSound(SOUND_FOUND_OIL);
}

void StudentWorld::pickUpGold() {
    m_player->incNuggets();
    increaseScore(10);
    playSound(SOUND_GOT_GOODIE);
}

void StudentWorld::pickUpSonarKit() {
    m_player->incSonars();
    increaseScore(75);
    playSound(SOUND_GOT_GOODIE);
}

void StudentWorld::pickUpWater() {
    m_player->refillSquirtGun();
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

bool StudentWorld::isThereBoulder(int x, int y) {
    vector<Actor*>::iterator it = m_actors.begin();
    
    // For each boulder
    while (it != m_actors.end()) {
        if ((*it)->isBoulder()) {
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
        
        if (isFourByFourTaken(currX, currY))
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