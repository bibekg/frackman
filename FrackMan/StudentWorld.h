#ifndef STUDENTWORLD_H_
#define STUDENTWORLD_H_

#include "GameWorld.h"
#include "GameConstants.h"
#include "Actor.h"
#include <string>
#include <vector>

// Constants

const int MAX_RADIUS = 6;

class StudentWorld : public GameWorld {
public:
    StudentWorld(std::string assetDir)
    : GameWorld(assetDir)
    {}
    
    virtual ~StudentWorld();
    
    virtual int init();
    virtual int move();
    virtual void cleanUp();
    
    Actor::Name whatIsHere(int x, int y);
    bool projectileWillCrash(int x, int y);
    
    // Dirt functions
    bool isThereDirt(int x, int y);
    void destroyDirt(int x, int y);
    
    // Squirt functions
    bool squirtProtestors(int x, int y);
    void spawnSquirt();
    bool canSquirtHere(int x, int y);
    
    bool crushLiveActorBelow(int x, int y);
    bool makeVisibleIfNearby(Pickup* pickup);
    bool pickupPickupIfNearby(Pickup* pickup);
    void frackManFoundItem(Pickup* pickup);
    
private:
    
    // Private Member Functions
    
    void setDisplayText();
    bool playerDied();
    bool finishedLevel();
    int barrelsLeft();
    
    std::string formatDisplayText(int score, int level, int lives, int health, int squirts, int gold, int sonar, int barrelsRemaining);
    bool isMineShaftRegion(int x, int y);
    bool canPlacePickupHere(int x, int y);
    bool isRadiusClear(int x, int y);
    int randInt(int min, int max);  // [min, max)
    
    // Private Member Variables
    
    FrackMan* m_player;
    Dirt* m_dirt[64][64];
    std::vector<Actor*> m_actors;
    
    int m_barrelsLeft;
    
};

#endif // STUDENTWORLD_H_
