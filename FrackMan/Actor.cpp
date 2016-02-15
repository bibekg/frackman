#include "Actor.h"
#include "StudentWorld.h"

// Students:  Add code to this file (if you wish), Actor.h, StudentWorld.h, and StudentWorld.cpp

// -------------------------- //
// --------- ACTOR  --------- //
// -------------------------- //

StudentWorld* Actor::getWorld() {
    return m_studentWorld;
}

// ---------------------------- //
// --------- FRACKMAN --------- //
// ---------------------------- //

void FrackMan::doSomething() {
    if (m_health == 0) return;  // FrackMan is dead!
    
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