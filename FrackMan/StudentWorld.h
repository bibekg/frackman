#ifndef STUDENTWORLD_H_
#define STUDENTWORLD_H_

#include "GameWorld.h"
#include "GameConstants.h"
#include "Actor.h"
#include <string>
#include <vector>

// Constants

const int MAX_RADIUS = 6;
const char DISCOVERED = 'X';
const char OPEN = '.';

class StudentWorld : public GameWorld {
public:
    StudentWorld(std::string assetDir)
    : GameWorld(assetDir) {}
    
    virtual ~StudentWorld();
    
    // Main game functions
    virtual int init();
    virtual int move();
    virtual void cleanUp();
    
    // General functions
    Actor::Name whatIsHere(int x, int y);
    Actor::Name whatIsBlockingPath(int x, int y, GraphObject::Direction dir);
    
    bool isSpotBlocked(int x, int y);
    
    // Frackman functions
    bool isPlayerOnDirt();
    void getPlayerAction();
    void dropGold();
    void useSonar();
    
    // Protester functions
    void protesterLeaveMap(Protester* protester);
    bool shoutIfPossible(Protester* protester);
    GraphObject::Direction directionToFrackMan(Protester* protester);
    bool stepTowardFrackMan(Protester* protester);
    GraphObject::Direction pickNewDirection(Protester* protester);
    bool movePerpendicularly(Protester* protester);
    bool takeAStep(Protester* protester);
    bool followFrackMan(Protester* protester);
    void annoyFrackMan();
    
    // Dirt functions
    bool isThereDirt(int x, int y);
    void destroyDirt(int x, int y);
    
    // Boulder functions
    bool willBoulderFall(Boulder* boulder);
    bool dropBoulderOrTick(Boulder * boulder);
    bool willBoulderCrash(Boulder* boulder);
    bool crushLiveActorBelow(Boulder* boulder);
    bool moveBoulder(Boulder* boulder);
    
    // Squirt functions
    bool squirtProtesters(Squirt* squirt);
    void spawnSquirt();
    
    // Gold functions
    bool getPickedUpByProtester(Gold* gold);
    
    // Pickup functions
    bool makeVisibleIfInRadius(Pickup* pickup, int rad);
    bool pickupPickupIfNearby(Pickup* pickup);
    void frackManFoundItem(Pickup* pickup);
    
private:
    
    // Housekeeping Functions
    void setDisplayText();
    bool playerDied();
    bool finishedLevel();
    int barrelsLeft();
    std::string formatDisplayText(int score, int level, int lives, int health, int squirts, int gold, int sonar, int barrelsRemaining);
    
    // World awareness functions
    bool isMineShaftRegion(int x, int y);
    bool isBoulderHere(int x, int y);
    bool canPlacePickupHere(int x, int y);
    bool isRadiusClear(int x, int y, int r);
    bool canPlaceWaterHere(int x, int y);
    int protesterCount();
//    void getShortestRoute(int &x, int &y, GraphObject::Direction &dir);
//    bool pathExists(int sr, int sc);
    void markAsOpen(int x, int y);
    GraphObject::Direction getProtesterDirectionToLeave(Protester* protester);
    
    // Auxiliary Functions
    
    // Private Member Variables
    FrackMan* m_player;
    Dirt* m_dirt[64][64];
    char m_exitMaze[64][64];
    int m_exitMap[64][64];
    std::vector<Actor*> m_actors;
    int m_barrelsLeft;
    int m_TICKSBETWEENSPAWNS;
    int m_ticksSinceProtesterSpawned;
    int m_maxProtesters;
    
    struct Coord {
        
        Coord(int x, int y, int ptl, GraphObject::Direction dir)
        : m_x(x), m_y(y), m_potential(ptl), m_dir(dir) {}
        
        int m_x;
        int m_y;
        int m_potential;
        GraphObject::Direction m_dir;
    };
    
    class Location {
    public:
        Location(int xx, int yy, GraphObject::Direction dir) : m_x(xx), m_y(yy), m_dir(dir) {}
        int x() const { return m_x; }
        int y() const { return m_y; }
        GraphObject::Direction dir() const { return m_dir; }
    private:
        int m_x;
        int m_y;
        GraphObject::Direction m_dir;
    };
};

#endif // STUDENTWORLD_H_
