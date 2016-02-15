#ifndef ACTOR_H_
#define ACTOR_H_

#include "GraphObject.h"
#include "GameConstants.h"

// Students:  Add code to this file, Actor.cpp, StudentWorld.h, and StudentWorld.cpp

class StudentWorld;

// -------------------------- //
// --------- ACTORS --------- //
// -------------------------- //

class Actor: public GraphObject {
public:
    // constructor
    Actor(int imageID, int startX, int startY, StudentWorld* studentWorld, Direction dir = right, double size = 0, unsigned int depth = 0)
    : GraphObject(imageID, startX, startY, dir, size, depth) {
        m_studentWorld = studentWorld;
        setVisible(true);
        m_alive = true;
    }
    
    virtual ~Actor() { setVisible(false); }
    virtual void doSomething() = 0;
    StudentWorld* getWorld();
    
    bool isAlive() { return m_alive; }
    
    void setDead() { m_alive = false; }
    
    void setAlive() { m_alive = true; }

private:
    StudentWorld* m_studentWorld;
    bool m_alive;
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

class StaticActor: public Actor {
public:
    StaticActor(int imageID, int startX, int startY, StudentWorld* studentWorld, Direction dir = right, double size = 0, unsigned int depth = 0)
    : Actor(imageID, startX, startY, studentWorld, dir, size, depth) {}
    
private:
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
    }
    
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

// --------------------------------- //
// --------- STATIC ACTORS --------- //
// --------------------------------- //

class Dirt: public StaticActor {      // change to StaticActor????
public:
    Dirt(int startX, int startY, StudentWorld* studentWorld)
    : StaticActor(IID_DIRT, startX, startY, studentWorld, right, 0.25, 3)
    {
//        setAlive();
    }
    
    virtual ~Dirt() {}
    void doSomething() {}
};

class Boulder: public StaticActor {
public:
    
    enum State { stable, waiting, falling };
    
    Boulder(int startX, int startY, StudentWorld* studentWorld)
    : StaticActor(IID_BOULDER, startX, startY, studentWorld, down, 1.0, 1)
    {
        m_state = stable;
        setAlive();
        ticksWaited = 0;
    }
    
    virtual ~Boulder() {}
    
    virtual void doSomething();
    
    void setState(State state) {
        m_state = state;
    }
    
private:
    State m_state;
    int ticksWaited;
};

class Squirt: public StaticActor {
    
};

class Oil: public StaticActor {
    
};

class Nugget: public StaticActor {
    
};

class SonarKit: public StaticActor {
    
};

class WaterPool: public StaticActor {
    
};

#endif // ACTOR_H_