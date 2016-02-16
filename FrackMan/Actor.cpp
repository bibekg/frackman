#include <vector>

#include "Actor.h"
#include "StudentWorld.h"

using namespace std;

// Students:  Add code to this file (if you wish), Actor.h, StudentWorld.h, and StudentWorld.cpp

// ------------------------- //
// --------- ACTOR --------- //
// ------------------------- //

// constructor
Actor::Actor(int imageID, int startX, int startY, StudentWorld* studentWorld, Direction dir, double size, unsigned int depth)
: GraphObject(imageID, startX, startY, dir, size, depth) {
    m_studentWorld = studentWorld;
    setVisible(true);
    m_alive = true;
}

StudentWorld* Actor::getWorld() { return m_studentWorld; }

// ---------------------------- //
// --------- FRACKMAN --------- //
// ---------------------------- //

void FrackMan::doSomething() {
    if (health() == 0) return;  // FrackMan is dead!
    
    // Check if overlapping dirt
    StudentWorld* world = getWorld();
    int playerX = getX();
    int playerY = getY();
    bool dug = false;
    for (int x = playerX; x < playerX + 4; x++)
        for (int y = playerY; y < playerY + 4; y++)
            if (world->isThereDirt(x, y)) {
                world->destroyDirt(x, y);
                dug = true;
            }
    if (dug) world->playSound(SOUND_DIG);
    
    // Move FrackMan
    int ch;
    if (getWorld()->getKey(ch) == true) {
        switch (ch) {
            case KEY_PRESS_LEFT:
                if (getDirection() != left)
                    setDirection(left);
                else if (getX() != 0)
                    moveTo(getX() - 1, getY());
                else
                    moveTo(getX(), getY());
                break;
            case KEY_PRESS_UP:
                if (getDirection() != up)
                    setDirection(up);
                else if (getY() != 60)
                    moveTo(getX(), getY() + 1);
                else
                    moveTo(getX(), getY());
                break;
            case KEY_PRESS_RIGHT:
                if (getDirection() != right)
                    setDirection(right);
                else if (getX() != 60)
                    moveTo(getX() + 1, getY());
                else
                    moveTo(getX(), getY());
                break;
            case KEY_PRESS_DOWN:
                if (getDirection() != down)
                    setDirection(down);
                else if (getY() != 0)
                    moveTo(getX(), getY() - 1);
                else
                    moveTo(getX(), getY());
                break;
            default:
                break;
        }
    }
}

void FrackMan::getAnnoyed(int amt) {
    setHealth(health()-2);
    
    if (health() <= 0) {
        setDead();
        getWorld()->playSound(SOUND_PLAYER_GIVE_UP);
    }
}

// --------------------------- //
// --------- BOULDER --------- //
// --------------------------- //

void Boulder::doSomething() {
    if (!isAlive()) return;
    
    // STABLE STATE
    else if (m_state == stable) {
        int rockX = getX();
        int rockY = getY();
        bool dirtBelow = false;
        
        // Check for dirt beneath boulder
        for (int x = rockX; x < rockX + 4; x++) {
            if (getWorld()->isThereDirt(x, rockY - 1))
                dirtBelow = true;
        }
        
        // No dirt below, enter waiting stage
        if (!dirtBelow)
            setState(waiting);
    }
    
    // WAITING STATE: Wait for 30 ticks to transition to falling
    else if (m_state == waiting) {
        if (ticksWaited == 30) {
            setState(falling);
            getWorld()->playSound(SOUND_FALLING_ROCK);
        }
        else
            ticksWaited++;
    }
    
    // FALLING STATE
    else if (m_state == falling) {

        // if within radius of 3 to protestors or FrackMan, cause 100 points of annoyance
        
        if (crashed()) {
            setDead();
        } else {
            moveTo(getX(), getY() - 1);
        }
    }
    
}

bool Boulder::crashed() {
    
    // Hit bottom of oil field
    if (getY() == 0) return true;
    
    // Ran into other boulder
    StudentWorld* world = getWorld();
    
    Actor::Name objectBelow[4];
    
    for (int i = 0; i < 4; i++) {
        objectBelow[i] = world->whatIsHere(getX() + i, getY() - 1);
        if (objectBelow[i] == boulder || objectBelow[i] == dirt)
            return true;
    }
    
    return false;
}

// -------------------------- //
// --------- BARREL --------- //
// -------------------------- //

void Barrel::doSomething() {}

// ------------------------ //
// --------- GOLD --------- //
// ------------------------ //

void Gold::doSomething(){}

// ---------------------------- //
// --------- SONARKIT --------- //
// ---------------------------- //

void SonarKit::doSomething(){}

// ------------------------------ //
// --------- WATER POOL --------- //
// ------------------------------ //

void WaterPool::doSomething() {}
