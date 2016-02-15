#ifndef ACTOR_H_
#define ACTOR_H_

#include "GraphObject.h"
#include "GameConstants.h"

// Students:  Add code to this file, Actor.cpp, StudentWorld.h, and StudentWorld.cpp

class StudentWorld;

class Actor: public GraphObject {
public:
    // constructor
    Actor(int imageID, int startX, int startY, StudentWorld* studentWorld, Direction dir = right, double size = 0, unsigned int depth = 0)
    : GraphObject(imageID, startX, startY, dir, size, depth) {
        m_studentWorld = studentWorld;
        setVisible(true);
    }
    
    virtual ~Actor() { setVisible(false); }
    virtual void doSomething() = 0;
    StudentWorld* getWorld();

private:
    StudentWorld* m_studentWorld;
};

class Dirt: public Actor {
public:
    Dirt(int startX, int startY, StudentWorld* studentWorld)
    : Actor(IID_DIRT, startX, startY, studentWorld, right, 0.25, 3)
    {}
    
    virtual ~Dirt() {}
    void doSomething() { return; }
};

class FrackMan: public Actor {
public:
    FrackMan(StudentWorld* studentWorld)
    : Actor(IID_PLAYER, 30, 60, studentWorld, right, 1.0, 0) {
        m_health = 10;
        m_squirts = 5;
        m_sonars = 1;
        m_nuggets = 0;
    }
    
    virtual void doSomething();
    
private:
    int m_health;
    int m_squirts;
    int m_sonars;
    int m_nuggets;
};

#endif // ACTOR_H_