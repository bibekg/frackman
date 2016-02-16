#ifndef ACTOR_H_
#define ACTOR_H_

#include "GraphObject.h"
#include "GameConstants.h"

// Students:  Add code to this file, Actor.cpp, StudentWorld.h, and StudentWorld.cpp

class StudentWorld;

// ------------------------- //
// --------- ABC's --------- //
// ------------------------- //

class Actor: public GraphObject {
public:
    
    enum Name { dirt, boulder, squirt, frackman, protestor, hardcore, barrel, gold, sonarkit, waterpool, nothing };
    
    // constructor
    Actor(int imageID, int startX, int startY, StudentWorld* studentWorld, Direction dir = right, double size = 0, unsigned int depth = 0);
    
    virtual ~Actor() { setVisible(false); }
    virtual void doSomething() = 0;
    StudentWorld* getWorld();
    
    // Setters
    void setName(Name name) { m_name = name; }
    void setAlive() { m_alive = true; }
    void setDead() { m_alive = false; }
    
    // Getters
    Name getName() { return m_name; }
    bool isAlive() { return m_alive; }

private:
    StudentWorld* m_studentWorld;
    bool m_alive;
    Name m_name;
};

class LiveActor: public Actor {
public:
    LiveActor(int imageID, int startX, int startY, StudentWorld* studentWorld, Direction dir = right, double size = 0, unsigned int depth = 0, int health = 100)
    : Actor(imageID, startX, startY, studentWorld, dir, size, depth) {}
    
    virtual ~LiveActor() {}
    
    virtual void getAnnoyed(int amt) = 0;
    
    // Getters
    int health() { return m_health; }
    
    // Setters
    void setHealth(int health) { m_health = health; }
    
private:
    int m_health;
};

class Pickup: public Actor {
public:
    Pickup(int imageID, int startX, int startY, bool frackManCanPickUp, StudentWorld* studentWorld, Direction dir = right, double size = 0, unsigned int depth = 0)
    : Actor(imageID, startX, startY, studentWorld, dir, size, depth)
    {
        setVisible(false);
        m_frackManCanPickup = frackManCanPickUp;
    }
    
    virtual ~Pickup(){}
    
private:
    
    bool m_frackManCanPickup;
    bool m_state;
    
};

// -------------------------- //
// --------- ACTORS --------- //
// -------------------------- //

class Dirt: public Actor {
public:
    Dirt(int startX, int startY, StudentWorld* studentWorld)
    : Actor(IID_DIRT, startX, startY, studentWorld, right, 0.25, 3)
    {
    }
    
    virtual ~Dirt() {}
    void doSomething() {}
};

class Boulder: public Actor {
public:
    
    enum State { stable, waiting, falling };
    
    Boulder(int startX, int startY, StudentWorld* studentWorld)
    : Actor(IID_BOULDER, startX, startY, studentWorld, down, 1.0, 1)
    {
        setState(stable);
        setAlive();
        ticksWaited = 0;
        setVisible(true);
        setName(boulder);
    }
    
    virtual ~Boulder() {}
    
    virtual void doSomething();
    
    void setState(State state) {
        m_state = state;
    }
    
private:
    
    bool crashed();
    
    State m_state;
    int ticksWaited;
};

class Squirt: public Actor {
    Squirt(int startX, int startY, StudentWorld* studentWorld)
    : Actor(IID_SONAR, startX, startY, studentWorld, right, 1.0, 2)
    {
        
    }
};

// ------------------------------- //
// --------- LIVE ACTORS --------- //
// ------------------------------- //

class FrackMan: public LiveActor {
public:
    FrackMan(StudentWorld* studentWorld)
    : LiveActor(IID_PLAYER, 30, 60, studentWorld, right, 1.0, 0, 10) {
        m_squirts = 5;
        m_sonars = 1;
        m_nuggets = 0;
        setName(frackman);
    }
    
    virtual ~FrackMan(){}
    
    virtual void doSomething();
    
    virtual void getAnnoyed(int amt);
    
    // Getters
    int squirts() { return m_squirts; }
    int sonars() { return m_sonars; }
    int nuggets() { return m_nuggets; }
    
    // Setters
    void setSquirts(int squirts) { m_squirts = squirts; };
    void setSonars(int sonars) { m_sonars = sonars; };
    void setNuggets(int nuggets) { m_nuggets = nuggets; };
    
private:
    int m_squirts;
    int m_sonars;
    int m_nuggets;
};

class Protestor: public LiveActor {
    
};

class HardCoreProtestor: public Protestor {
    
};

// --------------------------- //
// --------- PICKUPS --------- //
// --------------------------- //

class Barrel: public Pickup {
public:
    Barrel(int startX, int startY, StudentWorld* studentWorld)
    : Pickup(IID_BARREL, startX, startY, true, studentWorld, right, 1.0, 2)
    {
        
    }
    
    virtual void doSomething();
};

class Gold: public Pickup {
public:
    Gold(int startX, int startY, int frackManCanPickUp, StudentWorld* studentWorld)
    : Pickup(IID_GOLD, startX, startY, frackManCanPickUp, studentWorld, right, 1.0, 2)
    {
        
    }
    
    virtual void doSomething();
};

// SUBCLASS MAYBE?
class SonarKit: public Pickup {
    SonarKit(int startX, int startY, int frackManCanPickUp, StudentWorld* studentWorld)
    : Pickup(IID_SONAR, startX, startY, true, studentWorld, right, 1.0, 2)
    {
        
    }
    
    virtual void doSomething();
};

class WaterPool: public Pickup {
    WaterPool(int startX, int startY, int frackManCanPickUp, StudentWorld* studentWorld)
    : Pickup(IID_SONAR, startX, startY, true, studentWorld, right, 1.0, 2)
    {
        
    }
    
    virtual void doSomething();
};

// GLOBAL FUNCTIONS

//double distance(int x1, int y1, int x2, int y2) {
//    return pow(pow((x1-x2), 2) + pow((y1-y2), 2), 0.5);
//}

#endif // ACTOR_H_