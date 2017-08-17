#ifndef IW_TRADE_H_
#define IW_TRADE_H_

#include "IngameWindow.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/JobTypes.h"

class nobBaseWarehouse;
class GameWorldViewer;
class GameCommandFactory;

class iwTrade : public IngameWindow
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
    void Msg_PaintBefore() override;
    void Msg_PaintAfter() override;
    void Msg_ButtonClick(const unsigned ctrl_id) override;
    void Msg_ComboSelectItem(const unsigned ctrl_id, const int selection) override;
    unsigned GetPossibleTradeAmount(const Job job) const;
    unsigned GetPossibleTradeAmount(const GoodType good) const;
};

#endif // !IW_TRADE_H_
