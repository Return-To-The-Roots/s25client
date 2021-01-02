#pragma once

#include "IngameWindow.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/JobTypes.h"

class nobBaseWarehouse;
class GameWorldViewer;
class GameCommandFactory;

class iwTrade final : public IngameWindow
{
    const nobBaseWarehouse& wh; /// Das zugehörige Gebäudeobjekt
    const GameWorldViewer& gwv;
    GameCommandFactory& gcFactory;
    /// Possible wares
    std::vector<GoodType> wares;
    /// Possible figures
    std::vector<Job> jobs;
    /// Warehouses that can potentially deliver something to the selected wh
    std::vector<nobBaseWarehouse*> possibleSrcWarehouses;

public:
    iwTrade(const nobBaseWarehouse& wh, const GameWorldViewer& gwv, GameCommandFactory& gcFactory);

private:
    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_ComboSelectItem(unsigned ctrl_id, unsigned selection) override;
    unsigned GetPossibleTradeAmount(Job job) const;
    unsigned GetPossibleTradeAmount(GoodType good) const;
};
