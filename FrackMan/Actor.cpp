#include <vector>

#include <algorithm>
#include "Actor.h"
#include "StudentWorld.h"

using namespace std;

double radius(int x1, int y1, int x2, int y2) {
    double distance = pow(pow((x1-x2), 2) + pow((y1-y2), 2), 0.5);
    return distance;
}

int randInt(int min, int max) {
    return rand() % (max-min) + min;
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

// -------------------------- //
// --------- PICKUP --------- //
// -------------------------- //

Pickup::Pickup(int imageID, int startX, int startY, StudentWorld* studentWorld, Direction dir, double size, unsigned int depth)
: Actor(imageID, startX, startY, studentWorld, dir, size, depth) {
    makePickupVisible(false);
}

void Pickup::makePickupVisible(bool shouldDisplay) {
    m_isVisible = shouldDisplay;
    setVisible(shouldDisplay);    // UNCOMMENT THIS TO HIDE PICKUPS
}

// ------------------------------------ //
// --------- TEMPORARY PICKUP --------- //
// ------------------------------------ //

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
: LiveActor(IID_PLAYER, 30, 60, studentWorld, 10, right) {
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
    world->getPlayerAction();
}

bool FrackMan::getAnnoyed(int amt) {
    decHealth(amt);
    
    if (health() <= 0) {
        setDead();
        getWorld()->playSound(SOUND_PLAYER_GIVE_UP);
        return true;
    }
    return false;
}

// ------------------------ //
// --------- DIRT --------- //
// ------------------------ //

Dirt::Dirt(int startX, int startY, StudentWorld* studentWorld)
: Actor(IID_DIRT, startX, startY, studentWorld, right, 0.25, 3) {
    setName(dirt);
}

// ----------------------------- //
// --------- Protester --------- //
// ----------------------------- //

Protester::Protester(int x, int y, int imageID, StudentWorld* studentWorld, int health)
: LiveActor(imageID, x, y, studentWorld, health) {
    m_state = normal;
    m_leaving = false;
    resetRestTicks();
    m_ticksSinceLastShout = 15;
    resetTurnTicks();
    reRandomizeMoveSquares();
    m_defaultTicksToWait = max(0, 3 - int(studentWorld->getLevel())/4);
    m_stunnedTickstoWait = max(50, 100 - int(studentWorld->getLevel()) * 10);
    setName(protester);
}

void Protester::reRandomizeMoveSquares() {
    m_numSquaresToMoveInCurrDir = randInt(8, 61);
}

void Protester::getBribed() {
    getWorld()->playSound(SOUND_PROTESTER_FOUND_GOLD);
    getWorld()->increaseScore(25);
    m_leaving = true;
    m_state = leaving;
}

void Protester::doSomething() {
    
    if (!isAlive())
        return;
    
    // In resting state
    else if (m_state == resting) {
        
        // ticksToWait was set to 0 to expediate leaving process
        // re-set waiting ticks so protester doesn't speedwalk away after dying
        if (m_leaving) {
            m_defaultTicksToWait = max(0, 3 - int(getWorld()->getLevel())/4);
        }
        
        if (m_ticksRested >= m_defaultTicksToWait) {
            resetRestTicks();
            m_ticksSinceLastTurn++;
            m_ticksSinceLastShout++;
            if (m_leaving)
                m_state = leaving;
            else
                m_state = normal;
        } else {
            m_ticksRested++;
            return;
        }
    }
    
    // In leaving state
    if (m_state == leaving) {
        if (getX() == 60 && getY() == 60) {
            setDead();
        } else {
            getWorld()->protesterLeaveMap(this);
            setToRest();
        }
        return;
    }
    
    // In stunned state
    if (m_state == stunned) {
        if (m_ticksRested >= m_stunnedTickstoWait) {
            resetRestTicks();
            m_ticksSinceLastTurn++;
            m_ticksSinceLastShout++;
            m_state = normal;
        } else {
            m_ticksRested++;
            return;
        }
    }
    
    // In normal state
    if (m_state == normal) {
        if (getWorld()->shoutIfPossible(this)) {
            m_state = stunned;
            resetRestTicks();
            return;
        }
        
        else if (isHardcore())
            getWorld()->trackFrackMan(this);
        
        else if (getWorld()->stepTowardFrackMan(this))
            return;
        
        m_numSquaresToMoveInCurrDir--;
        
        if (m_numSquaresToMoveInCurrDir <= 0) {
            setDirection(getWorld()->pickNewDirection(this));
            reRandomizeMoveSquares();
        }
        
        else if (getWorld()->movePerpendicularly(this))
                resetTurnTicks();
        
        // Move one step in current direction
        // If blocked, set numSquaresToMove to 0
        if(!getWorld()->takeAStep(this))
            resetNumSquaresToMove();
    }
    m_state = resting;
}

bool Protester::getAnnoyed(int amt) {
    decHealth(amt);
    m_state = stunned;
    
    if (health() <= 0) {
        m_leaving = true;
        m_state = leaving;
        m_defaultTicksToWait = 0;
        m_stunnedTickstoWait = 0;
        getWorld()->playSound(SOUND_PROTESTER_GIVE_UP);
        return true;
    }
    return false;
}

HardCoreProtester::HardCoreProtester(int x, int y, StudentWorld* studentWorld)
: Protester(x, y, IID_HARD_CORE_PROTESTER, studentWorld, 20) {
    setTrackingRange(16 + int(studentWorld->getLevel()) * 2);
}

// --------------------------- //
// --------- BOULDER --------- //
// --------------------------- //

Boulder::Boulder(int startX, int startY, StudentWorld* studentWorld)
: Actor(IID_BOULDER, startX, startY, studentWorld, down, 1.0, 1) {
    setState(stable);
    setAlive();
    m_ticksWaited = 0;
    setVisible(true);
    setName(boulder);
}

void Boulder::doSomething() {
    if (!isAlive()) return;
    
    // STABLE STATE
    else if (m_state == stable) {
        
        // If boulder will fall now, enter waiting stage
        if (getWorld()->willBoulderFall(this))
            setState(waiting);
    }
    
    // WAITING STATE: Wait for 30 ticks to transition to falling
    else if (m_state == waiting)
        getWorld()->dropBoulderOrTick(this);
    
    // FALLING STATE
    else if (m_state == falling) {
        getWorld()->crushLiveActorBelow(this);
        getWorld()->moveBoulder(this);
    }
}

bool Boulder::crashed() {
    
    // Hit bottom of oil field
    if (getY() == 0) return true;
    
    // Crashed into another boulder or dirt
    if (getWorld()->willBoulderCrash(this))
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
    
    // If near Protester, kill it and remove squirt
    if(world->squirtProtesters(this))
        setDead();
    
    // Squirt done moving, set it to dead
    if (m_remainingDistance == 0)
        setDead();
    else
        m_remainingDistance--;
    
    // Move squirt unless it will crash
    switch (getDirection()) {
        case GraphObject::up:
            if (!world->isSpotBlocked(getX(), getY() + 4))
                moveTo(getX(), getY() + 1);
            else
                setDead();
            break;
        case GraphObject::right:
            if (!world->isSpotBlocked(getX() + 4, getY()))
                moveTo(getX() + 1, getY());
            else
                setDead();
            break;
        case GraphObject::down:
            if (!world->isSpotBlocked(getX(), getY() - 1))
                moveTo(getX(), getY() - 1);
            else
                setDead();
            break;
        case GraphObject::left:
            if (!world->isSpotBlocked(getX() - 1, getY()))
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
    
    if (getWorld()->makeVisibleIfInRadius(this, 4))
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
    
    if (getWorld()->makeVisibleIfInRadius(this, 4))
        return;
    
    StudentWorld* world = getWorld();
    if (m_whoCanPickUp == frackman && world->pickupPickupIfNearby(this))
        world->frackManFoundItem(this);
    
    else if (m_whoCanPickUp == protester) {
        // Protester needs to pick up gold
        if(getWorld()->getPickedUpByProtester(this)) {
            setDead();
            return;
        }
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

SonarKit::SonarKit(StudentWorld* studentWorld)
: TemporaryPickup(IID_SONAR, 0, 60, studentWorld, max(100, 300 - 10 * int(studentWorld->getLevel()))) {
    setName(sonarkit);
    setVisible(true);
}

// ------------------------------ //
// --------- WATER POOL --------- //
// ------------------------------ //

WaterPool::WaterPool(int startX, int startY, StudentWorld* studentWorld)
: TemporaryPickup(IID_WATER_POOL, startX, startY, studentWorld, min(100, 300 - 10 * int(studentWorld->getLevel()))) {
    setName(waterpool);
    setVisible(true);
}