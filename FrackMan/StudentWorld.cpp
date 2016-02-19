#include "StudentWorld.h"
#include <string>
#include <algorithm>
#include <iomanip>

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
        if (isRadiusClear(randX, randY) && canPlacePickupHere(randX, randY)) {
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
        if (isRadiusClear(randX, randY) && canPlacePickupHere(randX, randY)) {
            m_actors.push_back(new Gold(randX, randY, true, this));
            i++;
        }
    }
    
    // Construct/initialize oil barrels
    int barrels = min(2 + currentLevel, 20);
    for (int i = 0; i < barrels;) {
        int randX = randInt(0, 60);
        int randY = randInt(20, 57);
        if (isRadiusClear(randX, randY) && canPlacePickupHere(randX, randY)) {
            m_actors.push_back(new Barrel(randX, randY, this));
            i++;
        }
        
    }
    
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
}

Actor::Name StudentWorld::whatIsHere(int x, int y) {
    
    if (m_player->getX() == x && m_player->getY() == y)
        return Actor::frackman;
    
    vector<Actor*>::iterator it = m_actors.begin();
    
    while (it != m_actors.end()) {
        if ((*it)->getX() == x && (*it)->getY() == y) {
            return (*it)->getName();
        }
        it++;
    }
    
    if (m_dirt[x][y] != nullptr) return Actor::dirt;
    
    return Actor::nothing;
}

bool StudentWorld::projectileWillCrash(int x, int y) {
    Actor::Name item = whatIsHere(x, y);
    return (item == Actor::boulder || item == Actor::dirt);
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

bool StudentWorld::crushLiveActorBelow(int x, int y) {
    vector<Actor*>::iterator it = m_actors.begin();
    bool actorCrushed = false;
    
    // Check if FrackMan will get crushed
    int playerX = m_player->getX(), playerY = m_player->getY();
    if (distance(x, y, playerX, playerY) <= 3) {
        m_player->getAnnoyed(100);
        actorCrushed = true;
    }
    
    else {
        // Check if protestors will get crushed
        while (it != m_actors.end()) {
            if ((*it)->isAlive() && (*it)->canGetCrushed() && distance(x, y, (*it)->getX(), (*it)->getY()) <= 3) {
                (*it)->getAnnoyed(100);
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

bool StudentWorld::squirtProtestors(int x, int y) {
    
    vector<Actor*>::iterator it = m_actors.begin();
    bool protestorAnnoyed = false;
    
    while (it != m_actors.end()) {
        if ((*it) != nullptr && ((*it)->getName() == Actor::protestor || (*it)->getName() == Actor::hardcore) && (*it)->isAlive() && distance(x, y, (*it)->getX(), (*it)->getY()) <= 3) {
            (*it)->getAnnoyed(2);
            protestorAnnoyed = true;
        } else it++;
    }
    
    return protestorAnnoyed;
}

void StudentWorld::spawnSquirt() {
    int x = m_player->getX(), y = m_player->getY();
    
    playSound(SOUND_PLAYER_SQUIRT);
    
    // up and right failing
    
    
    switch (m_player->getDirection()) {
        case Actor::up:
            if (canSquirtHere(x, y + 4))
                m_actors.push_back(new Squirt(x, y + 4, m_player->getDirection(), this));
            break;
        case Actor::right:
            if (canSquirtHere(x + 4, y))
                m_actors.push_back(new Squirt(x + 4, y, m_player->getDirection(), this));
            break;
        case Actor::down:
            if (canSquirtHere(x, y - 4))
                m_actors.push_back(new Squirt(x, y - 4, m_player->getDirection(), this));
            break;
        case Actor::left:
            if (canSquirtHere(x - 4, y))
                m_actors.push_back(new Squirt(x - 4, y, m_player->getDirection(), this));
            break;
        default:
            break;
    }
}

bool StudentWorld::canSquirtHere(int x, int y) {
    
    bool isDirt = false;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            if (isThereDirt(x+i, y+j))
                isDirt = true;
    
    if (isDirt)
        return false;
    
    vector<Actor*>::iterator it = m_actors.begin();
    
    bool canSquirt = true;
    
    while (it != m_actors.end()) {
        if ((*it)->getName() == Actor::boulder && distance(x, y, (*it)->getX(), (*it)->getY()) <= 3) {
            canSquirt = false;
            break;
        }
        it++;
    }
    
    return canSquirt;
}

// ------------------------------------- //
// --------- PRIVATE FUNCTIONS --------- //
// ------------------------------------- //

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
    //    vector<Actor*>::iterator aBarrel;
    //    aBarrel = find_if(m_actors.begin(), m_actors.end(), [](Actor* a) {
    //        return a->getName() == Actor::barrel;
    //    });
    //
    //    // No barrels left, FrackMan finished!
    //    return aBarrel == m_actors.end();
    
    return barrelsLeft() == 0;
}

int StudentWorld::barrelsLeft() {
    int barrelCount = 0;
    
    vector<Actor*>::iterator it = m_actors.begin();
    while (it != m_actors.end()) {
        if ((*it) != nullptr && (*it)->isAlive() && (*it)->getName() == Actor::boulder) {
            barrelCount++;
        }
        it++;
    }
    
    return barrelCount;
}

string StudentWorld::formatDisplayText(int score, int level, int lives, int health, int squirts, int gold, int sonar, int barrelsRemaining) {
    string text;
    
    string scoreText = "Scr: " + std::string(6 - to_string(score).length(), '0') + to_string(score) + "  ";
    string levelText = "Lvl: " + std::string(2 - to_string(level).length(), ' ') + to_string(level) + "  ";
    string livesText = "Lives: " + to_string(lives) + "  ";
    string healthText = "Hlth: " + std::string(3 - to_string(health*10).length(), ' ') + to_string(health*10) + "%  ";
    string squirtsText = "Wtr: " + std::string(2 - to_string(squirts).length(), ' ') + to_string(squirts) + "  ";
    string goldText = "Gld: " + std::string(2 - to_string(gold).length(), ' ') + to_string(gold) + "  ";
    string sonarText = "Sonar: " + std::string(2 - to_string(sonar).length(), ' ') + to_string(sonar) + "  ";
    string barrelsText = "Oil Left: " + std::string(2 - to_string(barrelsRemaining).length(), ' ') + to_string(barrelsRemaining);
    
    text = scoreText + levelText + livesText + healthText + squirtsText + goldText + sonarText + barrelsText;
    return text;
}

bool StudentWorld::isMineShaftRegion(int x, int y) {
    if ((x >= 30 && x <= 33) && (y >= 4 && y <= 59)) return true;
    return false;
}

bool StudentWorld::canPlacePickupHere(int x, int y) {
    if (((x >= 27 && x <= 33) && (y >= 0)) || y > 56) return false;
    return true;
    
}

bool StudentWorld::isRadiusClear(int x, int y) {
    vector<Actor*>::iterator it = m_actors.begin();
    
    while (it != m_actors.end()) {
        double radius = distance(x, y, (*it)->getX(), (*it)->getY());
        if (radius <= MAX_RADIUS)       // found something in radius
            return false;
        
        it++;
    }
    
    return true;    // no objects found to be within radius
    
    
}

int StudentWorld::randInt(int min, int max) {
    return rand() % (max-min) + min;
}

