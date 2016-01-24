#ifndef IW_TRADE_H_
#define IW_TRADE_H_

#include "IngameWindow.h"

class dskGameInterface;
class nobUsual;
class GameWorldViewer;
class nobBaseWarehouse;

class iwTrade : public IngameWindow
{
        GameWorldViewer* const gwv;
        dskGameInterface* const gi; ///< Das GameInterface
        nobBaseWarehouse* const wh;              ///< Das zugehörige Gebäudeobjekt
        /// Possible wares
        std::vector<GoodType> wares;
        /// Possible figures
        std::vector<Job> jobs;
        /// Warehouses that can potentially deliver something to the selected wh
        std::vector<nobBaseWarehouse*> possibleSrcWarehouses;

    public:
        iwTrade(GameWorldViewer* const gwv, dskGameInterface* const gi, nobBaseWarehouse* const wh);

    private:

        void Msg_PaintBefore();
        void Msg_PaintAfter();
        void Msg_ButtonClick(const unsigned int ctrl_id);
        void Msg_ComboSelectItem(const unsigned ctrl_id, const unsigned short selection);
        unsigned GetPossibleTradeAmount(const Job job) const;
        unsigned GetPossibleTradeAmount(const GoodType good) const;
};

#endif // !IW_TRADE_H_
