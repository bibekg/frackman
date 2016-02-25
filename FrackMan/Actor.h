#ifndef ACTOR_H_
#define ACTOR_H_

#include "GraphObject.h"
#include "GameConstants.h"
#include <queue>

// ------------------------------------ //
// --------- GLOBAL FUNCTIONS --------- //
// ------------------------------------ //

double radius(int x1, int y1, int x2, int y2);
int randInt(int min, int max);  // [min, max)

// ------------------------------------ //

class StudentWorld;

// ------------------------- //
// --------- ABC's --------- //
// ------------------------- //

class Actor: public GraphObject {
public:
    
    enum Name { dirt, boulder, squirt, frackman, protester, hardcore, barrel, gold, sonarkit, waterpool, wall, nothing};
    
    // Constructor/Destructor
    Actor(int imageID, int startX, int startY, StudentWorld* studentWorld, Direction dir = right, double size = 0, unsigned int depth = 0);
    virtual ~Actor() {};
    
    StudentWorld* getWorld();
    
    // Identifiers
    virtual bool canGetCrushed() { return false; }
    virtual bool sonarMakesVisible() { return false; }
    virtual bool canGetSquirted() { return false; }
    virtual bool breaksBoulder() { return false; }
    
    virtual void doSomething() = 0;
    virtual bool getAnnoyed(int amt) {return false; }
    
    // Getters
    Name getName() { return m_name; }
    bool isAlive() { return m_alive; }
    
    // Setters
    void setName(Name name) { m_name = name; }
    void setAlive() { m_alive = true; }
    void setDead() { m_alive = false; }
    
private:
    StudentWorld* m_studentWorld;
    bool m_alive;
    Name m_name;
};

class LiveActor: public Actor {
public:
    LiveActor(int imageID, int startX, int startY, StudentWorld* studentWorld, int health, Direction dir = left, double size = 1, unsigned int depth = 0);
    virtual ~LiveActor() {}
    
    virtual bool canGetCrushed() { return true; }
    
    int health() { return m_health; }
    
protected:
    void decHealth(int amt) { m_health -= amt; }
    
private:
    int m_health;
};

class Pickup: public Actor {
public:
    
    Pickup(int imageID, int startX, int startY, StudentWorld* studentWorld, Direction dir = right, double size = 1, unsigned int depth = 2);
    
    virtual ~Pickup(){}
    
    void makePickupVisible(bool shouldDisplay);
    bool isPickupVisible() { return m_isVisible; }
    
private:
    bool m_isVisible;
    bool m_state;
    
};

class TemporaryPickup: public Pickup {
public:
    TemporaryPickup(int imageID, int startX, int startY, StudentWorld* studentWorld, int timeLeft);
    
    void doSomething();
    void decrementTicks() { m_ticksRemaining--; }
    int ticksRemaining() { return m_ticksRemaining; }

private:
    int m_ticksRemaining;
};

// -------------------------- //
// --------- ACTORS --------- //
// -------------------------- //

class Dirt: public Actor {
public:
    Dirt(int startX, int startY, StudentWorld* studentWorld);
    
    virtual ~Dirt() {}
    void doSomething() {}
};

class Boulder: public Actor {
public:
    
    enum State { stable, waiting, falling };
    
    Boulder(int startX, int startY, StudentWorld* studentWorld);
    
    virtual ~Boulder() {}
    
    // Identifiers
    virtual bool breaksBoulder() { return false; }
    
    virtual void doSomething();
    
    int ticksWaited() {return m_ticksWaited; }
    void incTicks() { m_ticksWaited++; }
    
    void setState(State state) { m_state = state; }
    bool crashed();
private:
    
    State m_state;
    int m_ticksWaited;
    
};

class Squirt: public Actor {
public:
    Squirt(int startX, int startY, Direction dir, StudentWorld* studentWorld);
    
    virtual ~Squirt() {}
    
    virtual void doSomething();
    
private:
    int m_remainingDistance;
};

// ------------------------------- //
// --------- LIVE ACTORS --------- //
// ------------------------------- //

class FrackMan: public LiveActor {
public:
    FrackMan(StudentWorld* studentWorld);
    
    virtual ~FrackMan(){}
    
    virtual void doSomething();
    
    virtual bool getAnnoyed(int amt);
    
    // Getters
    int squirts() { return m_squirts; }
    int sonars() { return m_sonars; }
    int nuggets() { return m_nuggets; }
    
    // Setters
    void incSonars() { m_sonars++; }
    void decSonars() { m_sonars--; }              // MAY NOT NEED
    void incNuggets() { m_nuggets++; }
    void decNuggets() { m_nuggets--; }
    void refillSquirtGun() { m_squirts += 5; }
    void decSquirts() { m_squirts--; }
    
private:
    int m_squirts;
    int m_sonars;
    int m_nuggets;
};

class Protester: public LiveActor {
public:
    
    Protester(int x, int y, int imageID, StudentWorld* studentWorld, int health = 5);
    
    virtual void doSomething();
    bool getAnnoyed(int amt);
    
    // Setters
    void setToRest() { m_state = resting; }
    void setStunned() { m_state = stunned; }
    void resetShoutTicks() { m_ticksSinceLastShout = 0; }
    void resetTurnTicks() { m_ticksSinceLastTurn = 0; }
    void resetRestTicks() { m_ticksRested = 0; }
    void resetNumSquaresToMove() { m_numSquaresToMoveInCurrDir = 0; }
    void reRandomizeMoveSquares();
    void setTrackingRange(int range) { m_trackingRange = range; }
    
    // Getters
    int ticksSinceLastShout() { return m_ticksSinceLastShout; }
    int ticksSinceLastTurn() { return m_ticksSinceLastTurn; }
    int trackingRange() { return m_trackingRange; }
    
    virtual bool isHardcore() { return false; }
    virtual bool canGetCrushed() { return true; }
    virtual bool canGetSquirted() { return true; }
    virtual void getBribed();
    
private:
    
    enum State { normal, resting, leaving, stunned };
    
    // Member variables
    State m_state;
    bool m_leaving;
    int m_defaultTicksToWait;
    int m_stunnedTickstoWait;
    int m_ticksRested;
    int m_ticksSinceLastShout;
    int m_ticksSinceLastTurn;
    int m_numSquaresToMoveInCurrDir;
    int m_trackingRange;
};

class HardCoreProtester: public Protester {
public:
    HardCoreProtester(int x, int y, StudentWorld* studentWorld);
    
    virtual bool isHardcore() { return true; }
    virtual void getBribed();
    
private:
    
};

// --------------------------- //
// --------- PICKUPS --------- //
// --------------------------- //

class Barrel: public Pickup {
public:
    Barrel(int startX, int startY, StudentWorld* studentWorld);
    
    virtual void doSomething();
    
    virtual bool sonarMakesVisible() { return true; }
};

class Gold: public Pickup {
public:
    
    enum PickupableBy {frackman, protester};
    enum TimeLimit {permanent, temporary};
    
    Gold(int startX, int startY, PickupableBy whoCanPickUp, TimeLimit timeLimit, StudentWorld* studentWorld);
    
    virtual void doSomething();
    
    virtual bool sonarMakesVisible() { return true; }
private:
    PickupableBy m_whoCanPickUp;
    TimeLimit m_timeLimit;
    int ticksRemaining;
};

class SonarKit: public TemporaryPickup {
public:
    SonarKit(StudentWorld* studentWorld);
};

class WaterPool: public TemporaryPickup {
public:
    WaterPool(int startX, int startY, StudentWorld* studentWorld);

private:
};

#endif // ACTOR_H_