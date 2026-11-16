// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwMapDebug.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "PointOutput.h"
#include "RttrForeachPt.h"
#include "ai/aijh/AIPlayerJH.h"
#include "controls/ctrlCheck.h"
#include "controls/ctrlComboBox.h"
#include "controls/ctrlEdit.h"
#include "controls/ctrlTimer.h"
#include "driver/KeyEvent.h"
#include "helpers/toString.h"
#include "notifications/NodeNote.h"
#include "ogl/glFont.h"
#include "world/GameWorldBase.h"
#include "world/GameWorldView.h"
#include "world/NodeMapBase.h"
#include "world/TerritoryRegion.h"
#include "gameTypes/GameTypesOutput.h"
#include "gameTypes/TextureColor.h"
#include "gameData/const_gui_ids.h"
#include "s25util/StringConversion.h"
#include <boost/nowide/iostream.hpp>
#include <chrono>

namespace {
enum
{
    ID_cbShowCoordinates,
    ID_cbShowWhat,
    ID_cbShowForPlayer,
    ID_cbCheckEventForPlayer,
    ID_tmrCheckEvents,
    ID_lblJump,
    ID_edtJumpX,
    ID_edtJumpY,
};
}

class iwMapDebug::DebugPrinter : public IDrawNodeCallback
{
public:
    DebugPrinter(const GameWorldBase& gwb) : showCoords(true), showDataIdx(0), playerIdx(0), gw(gwb), font(NormalFont)
    {}

    void onDraw(const MapPoint& pt, const DrawPoint& displayPt) override
    {
        std::string data;
        unsigned coordsColor = 0xFFFFFF00;
        unsigned dataColor = 0xFFFF0000;
        const MapNode& node = gw.getNode(pt);
        switch(showDataIdx)
        {
            case 1:
                if(node.reserved)
                    data = "R";
                break;
            case 2: data = helpers::toString(node.altitude); break;
            case 3: data = helpers::toString(node.resources.getValue()); break;
            case 4:
                if(node.seaId)
                    data = helpers::toString(node.seaId);
                else if(gw.getSeaFromCoastalPoint(pt))
                    data = "C";
                break;
            case 5: data = helpers::toString(node.owner); break;
            case 6:
            {
                bool isAllowed =
                  TerritoryRegion::isPointValid(gw.getSize(), gw.getPlayer(playerIdx).getRestrictedArea(), pt);
                coordsColor = dataColor = isAllowed ? 0xFF00FF00 : 0xFFFF0000;
                if(!showCoords)
                    data = isAllowed ? "y" : "n";
                break;
            }
        }

        if(showCoords)
        {
            std::string coord = helpers::toString(pt.x) + ":" + helpers::toString(pt.y);
            font->draw(displayPt, coord, FontStyle{}, coordsColor);
        }
        if(!data.empty())
            font->draw(displayPt, data, FontStyle{}, dataColor);
    }

    bool showCoords;
    unsigned showDataIdx;
    unsigned playerIdx;
    const GameWorldBase& gw;
    const glFont* font;
};

class iwMapDebug::EventChecker
{
public:
    EventChecker(const GameWorldBase& gw) : gw(gw)
    {
        bqMap.resize(gw.getSize());
        for(unsigned i = 0; i < gw.getNumPlayers(); i++)
        {
            if(gw.getPlayer(i).isUsed())
                usedPlayerIdxs.push_back(i);
        }
        RTTR_FOREACH_PT(MapPoint, bqMap.getSize())
        {
            for(const unsigned playerIdx : usedPlayerIdxs)
                bqMap[pt][playerIdx] = gw.GetBQ(pt, playerIdx);
        }
        nodeSub = AIJH::recordBQsToUpdate(this->gw, this->pointsToUpdate);
    }
    void check()
    {
        for(const MapPoint pt : pointsToUpdate)
            updateBq(pt);
        pointsToUpdate.clear();
        RTTR_FOREACH_PT(MapPoint, bqMap.getSize())
        {
            for(const unsigned playerIdx : usedPlayerIdxs)
            {
                if(bqMap[pt][playerIdx] != gw.GetBQ(pt, playerIdx))
                {
                    boost::nowide::cerr << "BQs mismatch at " << pt << " for player " << playerIdx << ": "
                                        << bqMap[pt][playerIdx] << "!=" << gw.GetBQ(pt, playerIdx) << std::endl;
                }
            }
        }
    }

private:
    void updateBq(MapPoint pt)
    {
        for(const unsigned playerIdx : usedPlayerIdxs)
            bqMap[pt][playerIdx] = gw.GetBQ(pt, playerIdx);
    }
    const GameWorldBase& gw;
    std::vector<unsigned> usedPlayerIdxs;
    using BqNode = std::array<BuildingQuality, MAX_PLAYERS>;
    NodeMapBase<BqNode> bqMap;
    std::vector<MapPoint> pointsToUpdate;
    Subscription nodeSub;
};

using namespace std::chrono_literals;
static const std::array<std::chrono::milliseconds, 6> BQ_CHECK_INTERVALS = {10s, 1s, 500ms, 250ms, 100ms, 50ms};

iwMapDebug::iwMapDebug(GameWorldView& gwv, bool allowCheating)
    : IngameWindow(CGI_MAP_DEBUG, IngameWindow::posLastOrCenter, Extent(230, 160), _("Map Debug"),
                   LOADER.getImageN("resource", 41)),
      gwv(gwv), printer(std::make_unique<DebugPrinter>(gwv.getWorld()))
{
    gwv.addDrawNodeCallback(printer.get());

    DrawPoint curPos(15, 25);
    constexpr Extent ctrlSize(200, 20);
    constexpr auto spaceY = ctrlSize.y + 5;

    ctrlCheck* cbShowCoords =
      addCheckBox(ID_cbShowCoordinates, curPos, ctrlSize, TextureColor::Grey, _("Show coordinates"), NormalFont);
    curPos.y += spaceY;
    cbShowCoords->setChecked(true);

    constexpr Extent editSize(40, ctrlSize.y);
    addText(ID_lblJump, curPos + DrawPoint(0, 2), _("Jump to X:Y"), COLOR_YELLOW, FontStyle{}, NormalFont);
    const DrawPoint edtPos = curPos + DrawPoint(ctrlSize.x - 2 * editSize.x - 2, 0);
    addEdit(ID_edtJumpX, edtPos, editSize, TextureColor::Grey, NormalFont, 4);
    addEdit(ID_edtJumpY, edtPos + DrawPoint(editSize.x + 2, 0), editSize, TextureColor::Grey, NormalFont, 4);
    curPos.y += spaceY;

    ctrlComboBox* cbCheckEvents =
      addComboBox(ID_cbCheckEventForPlayer, curPos, ctrlSize, TextureColor::Grey, NormalFont, 100);
    curPos.y += spaceY;
    cbCheckEvents->addItem(_("BQ check disabled"));
    for(const std::chrono::milliseconds ms : BQ_CHECK_INTERVALS)
    {
        cbCheckEvents->addItem((boost::format(_("BQ check every %1%ms")) % ms.count()).str());
    }
    cbCheckEvents->setSelection(0);
    using namespace std::chrono_literals;
    addTimer(ID_tmrCheckEvents, 500ms)->stop();

    if(allowCheating)
    {
        ctrlComboBox* data = addComboBox(ID_cbShowWhat, curPos, ctrlSize, TextureColor::Grey, NormalFont, 100);
        curPos.y += spaceY;
        data->addItem(_("Nothing"));
        data->addItem(_("Reserved"));
        data->addItem(_("Altitude"));
        data->addItem(_("Resources"));
        data->addItem(_("Sea Id"));
        data->addItem(_("Owner"));
        data->addItem(_("Restricted area"));
        data->setSelection(1);
        ctrlComboBox* players = addComboBox(ID_cbShowForPlayer, curPos, ctrlSize, TextureColor::Grey, NormalFont, 100);
        curPos.y += spaceY;
        for(unsigned pIdx = 0; pIdx < gwv.getWorld().getNumPlayers(); pIdx++)
        {
            const GamePlayer& p = gwv.getWorld().getPlayer(pIdx);
            if(!p.isUsed())
                players->addItem((boost::format(_("Player %1%")) % pIdx).str());
            else
                players->addItem(p.name);
        }
        players->setSelection(0);
        printer->showDataIdx = *data->getSelection();
        printer->playerIdx = *players->getSelection();
    } else
    {
        printer->showDataIdx = 0;
        printer->playerIdx = 0;
        Extent iwSize = getIwSize();
        iwSize.y -= 40 + 10;
        setIwSize(iwSize);
    }

    printer->showCoords = cbShowCoords->isChecked();
}

iwMapDebug::~iwMapDebug()
{
    gwv.removeDrawNodeCallback(printer.get());
}

void iwMapDebug::msgComboSelectItem(const unsigned ctrl_id, const unsigned select)
{
    if(ctrl_id == ID_cbShowWhat)
        printer->showDataIdx = select;
    else if(ctrl_id == ID_cbShowForPlayer)
        printer->playerIdx = select;
    else if(ctrl_id == ID_cbCheckEventForPlayer)
    {
        if(select == 0)
        {
            eventChecker.reset();
            getCtrl<ctrlTimer>(ID_tmrCheckEvents)->stop();
        } else
        {
            eventChecker = std::make_unique<EventChecker>(gwv.getWorld());
            getCtrl<ctrlTimer>(ID_tmrCheckEvents)->start(BQ_CHECK_INTERVALS[select - 1]);
        }
    }
}

void iwMapDebug::msgCheckboxChange(const unsigned ctrl_id, const bool checked)
{
    if(ctrl_id == ID_cbShowCoordinates)
        printer->showCoords = checked;
}

void iwMapDebug::msgTimer(unsigned /*ctrl_id*/)
{
    if(eventChecker)
        eventChecker->check();
}

void iwMapDebug::msgEditEnter(unsigned ctrl_id)
{
    RTTR_Assert(ctrl_id == ID_edtJumpX || ctrl_id == ID_edtJumpY);
    Position pos;
    if(s25util::tryFromStringClassic(getCtrl<ctrlEdit>(ID_edtJumpX)->getText(), pos.x)
       && s25util::tryFromStringClassic(getCtrl<ctrlEdit>(ID_edtJumpY)->getText(), pos.y))
    {
        gwv.moveToMapPt(gwv.getWorld().makeMapPoint(pos));
    }
}

bool iwMapDebug::msgKeyDown(const KeyEvent& ke)
{
    switch(ke.kt)
    {
        case KeyType::Return: msgEditEnter(ID_edtJumpY); break;
        case KeyType::Tab:
        {
            auto* edtNewFocus = getCtrl<ctrlEdit>(ID_edtJumpX);
            if(edtNewFocus->hasFocus())
                edtNewFocus = getCtrl<ctrlEdit>(ID_edtJumpY);
            edtNewFocus->setFocus();
            edtNewFocus->setText("");
        };
        break;
        default: return IngameWindow::msgKeyDown(ke);
    }
    return true;
}

void iwMapDebug::setActive(bool activate)
{
    IngameWindow::setActive(activate);
    if(activate)
        getCtrl<ctrlEdit>(ID_edtJumpX)->setFocus();
}
