#include <vector>

#include <algorithm>
#include "Actor.h"
#include "StudentWorld.h"

using namespace std;

// Students:  Add code to this file (if you wish), Actor.h, StudentWorld.h, and StudentWorld.cpp

double radius(int x1, int y1, int x2, int y2) {
    double distance = pow(pow((x1-x2), 2) + pow((y1-y2), 2), 0.5);
    return distance;
}

// ------------------------- //
// --------- ACTOR --------- //
// ------------------------- //

Actor::Actor(int imageID, int startX, int startY, StudentWorld* studentWorld, Direction dir, double size, unsigned int depth)
: GraphObject(imageID, startX, startY, dir, size, depth) {
    m_studentWorld = studentWorld;
    setVisible(true);
    m_alive = true;
}

StudentWorld* Actor::getWorld() { return m_studentWorld; }

// ------------------------------ //
// --------- LIVE ACTOR --------- //
// ------------------------------ //

LiveActor::LiveActor(int imageID, int startX, int startY, StudentWorld* studentWorld, int health, Direction dir, double size, unsigned int depth)
: Actor(imageID, startX, startY, studentWorld, dir, size, depth) {
    m_health = health;
}

void LiveActor::getAnnoyed(int amt) {
    decHealth(amt);
    
    if (health() <= 0) {
        setDead();
    }
}

// -------------------------- //
// --------- PICKUP --------- //
// -------------------------- //

Pickup::Pickup(int imageID, int startX, int startY, StudentWorld* studentWorld, Direction dir, double size, unsigned int depth)
: Actor(imageID, startX, startY, studentWorld, dir, size, depth) {
    makePickupVisible(false);
}

void Pickup::makePickupVisible(bool shouldDisplay) {
    m_isVisible = shouldDisplay;
    //    setVisible(shouldDisplay);    // UNCOMMENT THIS TO HIDE PICKUPS
}

// ----------------------------------- //
// --------- TEMPORARYPICKUP --------- //
// ----------------------------------- //

TemporaryPickup::TemporaryPickup(int imageID, int startX, int startY, StudentWorld* studentWorld, int timeLeft)
: Pickup(imageID, startX, startY, studentWorld) {
    m_ticksRemaining = timeLeft;
}

void TemporaryPickup::doSomething() {
    if (!isAlive())
        return;
    
    StudentWorld* world = getWorld();
    if (world->pickupPickupIfNearby(this))
        world->frackManFoundItem(this);
    
    if (ticksRemaining() <= 0)
        setDead();
    else decrementTicks();
}

// ---------------------------- //
// --------- FRACKMAN --------- //
// ---------------------------- //

FrackMan::FrackMan(StudentWorld* studentWorld)
: LiveActor(IID_PLAYER, 30, 60, studentWorld, right) {
    m_squirts = 5;
    m_sonars = 1;
    m_nuggets = 0;
    setName(frackman);
}

void FrackMan::doSomething() {
    if (health() == 0) return;  // FrackMan is dead!
    StudentWorld* world = getWorld();
    
    // Check if overlapping dirt
    if (world->isPlayerOnDirt())
        world->playSound(SOUND_DIG);
    
    // Get user action
    world->movePlayer();
}

void FrackMan::getAnnoyed(int amt) {
    decHealth(amt);
    
    if (health() <= 0) {
        setDead();
        getWorld()->playSound(SOUND_PLAYER_GIVE_UP);
    }
}

// ------------------------ //
// --------- DIRT --------- //
// ------------------------ //

Dirt::Dirt(int startX, int startY, StudentWorld* studentWorld)
: Actor(IID_DIRT, startX, startY, studentWorld, right, 0.25, 3) {
    setName(dirt);
}

// ----------------------------- //
// --------- PROTESTOR --------- //
// ----------------------------- //

void Protestor::getAnnoyed(int amt) {
    decHealth(amt);
    
    if (health() <= 0) {
        setDead();
        getWorld()->playSound(SOUND_PROTESTER_GIVE_UP);
    }
}

// --------------------------- //
// --------- BOULDER --------- //
// --------------------------- //

Boulder::Boulder(int startX, int startY, StudentWorld* studentWorld)
: Actor(IID_BOULDER, startX, startY, studentWorld, down, 1.0, 1) {
    setState(stable);
    setAlive();
    ticksWaited = 0;
    setVisible(true);
    setName(boulder);
}

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
        getWorld()->crushLiveActorBelow(getX(), getY());
        
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

    // Crashed into another boulder or dirt
    if (getWorld()->willBoulderCrash(getX(), getY()))
        return true;
    
    return false;
}

// -------------------------- //
// --------- SQUIRT --------- //
// -------------------------- //

Squirt::Squirt(int startX, int startY, Direction dir, StudentWorld* studentWorld)
: Actor(IID_WATER_SPURT, startX, startY, studentWorld, dir, 1.0, 1) {
    m_remainingDistance = 4;
    setName(squirt);
}

void Squirt::doSomething() {
    
    StudentWorld* world = getWorld();
    
    // If near protestor, kill it and remove squirt
    if(world->squirtProtestors(getX(), getY()))
        setDead();
    
    // Squirt done moving, set it to dead
    if (m_remainingDistance == 0)
        setDead();
    else
        m_remainingDistance--;
    
    // Move squirt unless it will crash
    switch (getDirection()) {
        case GraphObject::up:
            if (!world->willSquirtCrash(getX(), getY() + 4))
                moveTo(getX(), getY() + 1);
            else
                setDead();
            break;
        case GraphObject::right:
            if (!world->willSquirtCrash(getX() + 4, getY()))
                moveTo(getX() + 1, getY());
            else
                setDead();
            break;
        case GraphObject::down:
            if (!world->willSquirtCrash(getX(), getY() - 1))
                moveTo(getX(), getY() - 1);
            else
                setDead();
            break;
        case GraphObject::left:
            if (!world->willSquirtCrash(getX() - 1, getY()))
                moveTo(getX() - 1, getY());
            else
                setDead();
            break;
        default:
            break;
    }
}

// -------------------------- //
// --------- BARREL --------- //
// -------------------------- //

Barrel::Barrel(int startX, int startY, StudentWorld* studentWorld)
: Pickup(IID_BARREL, startX, startY, studentWorld, right, 1.0, 2) {
    setName(barrel);
}

void Barrel::doSomething() {
    if (!isAlive())
        return;
    
    if (getWorld()->makeVisibleIfNearby(this))
        return;
    
    StudentWorld* world = getWorld();
    if (world->pickupPickupIfNearby(this))
        world->frackManFoundItem(this);
}

// ------------------------ //
// --------- GOLD --------- //
// ------------------------ //

Gold::Gold(int startX, int startY, PickupableBy whoCanPickUp, TimeLimit timeLimit, StudentWorld* studentWorld)
: Pickup(IID_GOLD, startX, startY, studentWorld, right, 1.0, 2) {
    m_whoCanPickUp = whoCanPickUp;
    m_timeLimit = timeLimit;
    ticksRemaining = 100;
    setName(gold);
}

void Gold::doSomething() {
    
    if (!isAlive())
        setDead();
    
    if (getWorld()->makeVisibleIfNearby(this))
        return;
    
    StudentWorld* world = getWorld();
    if (m_whoCanPickUp == frackman && world->pickupPickupIfNearby(this))
        world->frackManFoundItem(this);
    
    else if (m_whoCanPickUp == protestor) {
        // Protestor needs to pick up gold
    }
    
    // Remove gold if it has waited 100 ticks
    if (m_timeLimit == temporary) {
        if (ticksRemaining <= 0)
            setDead();
        else ticksRemaining--;
    }
    
    
}

// ---------------------------- //
// --------- SONARKIT --------- //
// ---------------------------- //

SonarKit::SonarKit(int startX, int startY, int frackManCanPickUp, StudentWorld* studentWorld)
: TemporaryPickup(IID_SONAR, startX, startY, studentWorld, max(100, 300 - 10 * int(studentWorld->getLevel()))) {
    setName(sonarkit);
    setVisible(true);
}

// ------------------------------ //
// --------- WATER POOL --------- //
// ------------------------------ //

WaterPool::WaterPool(int startX, int startY, int frackManCanPickUp, StudentWorld* studentWorld)
: TemporaryPickup(IID_SONAR, startX, startY, studentWorld, min(100, 300 - 10 * int(studentWorld->getLevel()))) {
    setName(waterpool);
    setVisible(true);
}









