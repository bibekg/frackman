#ifndef STUDENTWORLD_H_
#define STUDENTWORLD_H_

#include "GameWorld.h"
#include "GameConstants.h"
#include "Actor.h"
#include <string>
#include <vector>

// Students:  Add code to this file, StudentWorld.cpp, Actor.h, and Actor.cpp

class StudentWorld : public GameWorld
{
public:
    StudentWorld(std::string assetDir)
    : GameWorld(assetDir)
    {
        
    }
    
    virtual ~StudentWorld() {
        delete m_player;
        for (int x = 0; x < 64; x++)
            for (int y = 0; y < 64; y++)
                delete m_dirt[x][y];
    }
    
    virtual int init();
    
    virtual int move();
    
    virtual void cleanUp();
    
    bool isThereDirt(int x, int y);
    
    void destroyDirt(int x, int y);
    
private:
    
    bool isMineShaftRegion(int x, int y);
    
    
    FrackMan* m_player;
    Dirt* m_dirt[64][64];
    
    std::vector<Actor*> m_objects;
    
};

#endif // STUDENTWORLD_H_
