//
// Created by pavel on 01.05.25.
//

#ifndef DATAMINER_H
#define DATAMINER_H

#include "PlayerInfo.h"
#include "GamePlayer.h"
#include "Game.h"

class DataMiner {
public:
    DataMiner(unsigned gf, Game& game);
    void Run();

private:
    void mineBuildings(GamePlayer& player);
    void mineBuildingsSites(GamePlayer& player);
    void mineStats(GamePlayer& player);

    unsigned gf_;
    Game& game_;
    GameWorld& world_;
};



#endif //DATAMINER_H
