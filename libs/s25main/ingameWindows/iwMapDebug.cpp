// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

#include "iwMapDebug.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "PointOutput.h"
#include "RttrForeachPt.h"
#include "controls/ctrlCheck.h"
#include "controls/ctrlComboBox.h"
#include "controls/ctrlTimer.h"
#include "helpers/toString.h"
#include "notifications/NodeNote.h"
#include "ogl/glFont.h"
#include "world/GameWorldBase.h"
#include "world/GameWorldView.h"
#include "world/NodeMapBase.h"
#include "world/TerritoryRegion.h"
#include "gameTypes/TextureColor.h"
#include "gameData/const_gui_ids.h"
#include <boost/nowide/iostream.hpp>

namespace {
enum
{
    ID_cbShowCoordinates,
    ID_cbShowWhat,
    ID_cbShowForPlayer,
    ID_cbCheckEventForPlayer,
    ID_lblCheckEvents,
    ID_tmrCheckEvents,
};
}

class iwMapDebug::DebugPrinter : public IDrawNodeCallback
{
public:
    DebugPrinter(const GameWorldBase& gwb) : showCoords(true), showDataIdx(0), playerIdx(0), gw(gwb), font(NormalFont) {}

    void onDraw(const MapPoint& pt, const DrawPoint& displayPt) override
    {
        std::string data;
        unsigned coordsColor = 0xFFFFFF00;
        unsigned dataColor = 0xFFFF0000;
        const MapNode& node = gw.GetNode(pt);
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
                else if(gw.GetSeaFromCoastalPoint(pt))
                    data = "C";
                break;
            case 5: data = helpers::toString(node.owner); break;
            case 6:
            {
                bool isAllowed = TerritoryRegion::IsPointValid(gw.GetSize(), gw.GetPlayer(playerIdx).GetRestrictedArea(), pt);
                coordsColor = dataColor = isAllowed ? 0xFF00FF00 : 0xFFFF0000;
                if(!showCoords)
                    data = isAllowed ? "y" : "n";
                break;
            }
        }

        if(showCoords)
        {
            std::string coord = helpers::toString(pt.x) + ":" + helpers::toString(pt.y);
            font->Draw(displayPt, coord, FontStyle{}, coordsColor);
        }
        if(!data.empty())
            font->Draw(displayPt, data, FontStyle{}, dataColor);
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
        bqMap.Resize(gw.GetSize());
        for(unsigned i = 0; i < gw.GetNumPlayers(); i++)
        {
            if(gw.GetPlayer(i).isUsed())
                usedPlayerIdxs.push_back(i);
        }
        RTTR_FOREACH_PT(MapPoint, bqMap.GetSize())
        {
            for(const unsigned playerIdx : usedPlayerIdxs)
                bqMap[pt][playerIdx] = gw.GetBQ(pt, playerIdx);
        }
        nodeSub = gw.GetNotifications().subscribe<NodeNote>([this](const NodeNote& note) {
            if(note.type == NodeNote::BQ)
                pointsToUpdate.push_back(note.pos);
            else if(note.type == NodeNote::Owner)
            {
                // Owner changes border, which changes where buildings can be placed next to it
                // And as flags are need for buildings we need range 2 (e.g. range 1 is flag, range 2 building)
                this->gw.CheckPointsInRadius(
                  note.pos, 2,
                  [this](const MapPoint pt, unsigned) {
                      pointsToUpdate.push_back(pt);
                      return false;
                  },
                  true);
            }
        });
    }
    void check()
    {
        for(const MapPoint pt : pointsToUpdate)
            updateBq(pt);
        pointsToUpdate.clear();
        RTTR_FOREACH_PT(MapPoint, bqMap.GetSize())
        {
            for(const unsigned playerIdx : usedPlayerIdxs)
            {
                if(bqMap[pt][playerIdx] != gw.GetBQ(pt, playerIdx))
                {
                    boost::nowide::cerr << "BQs mismatch at " << pt << " for player " << playerIdx << ": " << bqMap[pt][playerIdx]
                                        << "!=" << gw.GetBQ(pt, playerIdx) << "\n";
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

static const std::array<unsigned, 6> BQ_CHECK_INTERVALS = {10000, 1000, 500, 250, 100, 50};

iwMapDebug::iwMapDebug(GameWorldView& gwv, bool allowCheating)
    : IngameWindow(CGI_MAP_DEBUG, IngameWindow::posLastOrCenter, Extent(230, 135), _("Map Debug"), LOADER.GetImageN("resource", 41)),
      gwv(gwv), printer(std::make_unique<DebugPrinter>(gwv.GetWorld()))
{
    gwv.AddDrawNodeCallback(printer.get());

    ctrlCheck* cbShowCoords =
      AddCheckBox(ID_cbShowCoordinates, DrawPoint(15, 25), Extent(200, 20), TC_GREY, _("Show coordinates"), NormalFont);
    cbShowCoords->SetCheck(true);
    ctrlComboBox* cbCheckEvents = AddComboBox(ID_cbCheckEventForPlayer, DrawPoint(15, 50), Extent(200, 20), TC_GREY, NormalFont, 100);
    cbCheckEvents->AddString(_("BQ check disabled"));
    for(unsigned ms : BQ_CHECK_INTERVALS)
    {
        cbCheckEvents->AddString((boost::format(_("BQ check every %1%ms")) % ms).str());
    }
    cbCheckEvents->SetSelection(0);
    AddTimer(ID_tmrCheckEvents, 500)->Stop();

    if(allowCheating)
    {
        ctrlComboBox* data = AddComboBox(ID_cbShowWhat, DrawPoint(15, 75), Extent(200, 20), TC_GREY, NormalFont, 100);
        data->AddString(_("Nothing"));
        data->AddString(_("Reserved"));
        data->AddString(_("Altitude"));
        data->AddString(_("Resources"));
        data->AddString(_("Sea Id"));
        data->AddString(_("Owner"));
        data->AddString(_("Restricted area"));
        data->SetSelection(1);
        ctrlComboBox* players = AddComboBox(ID_cbShowForPlayer, DrawPoint(15, 100), Extent(200, 20), TC_GREY, NormalFont, 100);
        for(unsigned pIdx = 0; pIdx < gwv.GetWorld().GetNumPlayers(); pIdx++)
        {
            const GamePlayer& p = gwv.GetWorld().GetPlayer(pIdx);
            if(!p.isUsed())
                players->AddString((boost::format(_("Player %1%")) % pIdx).str());
            else
                players->AddString(p.name);
        }
        players->SetSelection(0);
        printer->showDataIdx = data->GetSelection();
        printer->playerIdx = players->GetSelection();
    } else
    {
        printer->showDataIdx = 0;
        printer->playerIdx = 0;
        Extent iwSize = GetIwSize();
        iwSize.y -= 40 + 10;
        SetIwSize(iwSize);
    }

    printer->showCoords = cbShowCoords->GetCheck();
}

iwMapDebug::~iwMapDebug()
{
    gwv.RemoveDrawNodeCallback(printer.get());
}

void iwMapDebug::Msg_ComboSelectItem(const unsigned ctrl_id, const int select)
{
    if(ctrl_id == ID_cbShowWhat)
    {
        if(select >= 0)
            printer->showDataIdx = static_cast<unsigned>(select);
    } else if(ctrl_id == ID_cbShowForPlayer)
    {
        if(select >= 0)
            printer->playerIdx = static_cast<unsigned>(select);
    } else if(ctrl_id == ID_cbCheckEventForPlayer)
    {
        if(select == 0)
        {
            eventChecker.reset();
            GetCtrl<ctrlTimer>(ID_tmrCheckEvents)->Stop();
        } else
        {
            eventChecker = std::make_unique<EventChecker>(gwv.GetWorld());
            GetCtrl<ctrlTimer>(ID_tmrCheckEvents)->Start(BQ_CHECK_INTERVALS[select - 1]);
        }
    }
}

void iwMapDebug::Msg_CheckboxChange(const unsigned ctrl_id, const bool checked)
{
    if(ctrl_id == ID_cbShowCoordinates)
        printer->showCoords = checked;
}

void iwMapDebug::Msg_Timer(unsigned /*ctrl_id*/)
{
    if(eventChecker)
        eventChecker->check();
}
