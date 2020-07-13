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
#include "controls/ctrlCheck.h"
#include "controls/ctrlComboBox.h"
#include "helpers/toString.h"
#include "ogl/glFont.h"
#include "world/GameWorldBase.h"
#include "world/GameWorldView.h"
#include "world/TerritoryRegion.h"
#include "gameTypes/TextureColor.h"
#include "gameData/const_gui_ids.h"

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
            default: return;
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

iwMapDebug::iwMapDebug(GameWorldView& gwv, bool allowCheating)
    : IngameWindow(CGI_MAP_DEBUG, IngameWindow::posLastOrCenter, Extent(230, 110), _("Map Debug"), LOADER.GetImageN("resource", 41)),
      gwv(gwv), printer(new DebugPrinter(gwv.GetWorld()))
{
    gwv.AddDrawNodeCallback(printer);

    ctrlCheck* cbShowCoords = AddCheckBox(0, DrawPoint(15, 25), Extent(200, 20), TC_GREY, _("Show coordinates"), NormalFont);
    cbShowCoords->SetCheck(true);
    if(allowCheating)
    {
        ctrlComboBox* data = AddComboBox(1, DrawPoint(15, 50), Extent(200, 20), TC_GREY, NormalFont, 100);
        data->AddString(_("Nothing"));
        data->AddString(_("Reserved"));
        data->AddString(_("Altitude"));
        data->AddString(_("Resources"));
        data->AddString(_("Sea Id"));
        data->AddString(_("Owner"));
        data->AddString(_("Restricted area"));
        data->SetSelection(1);
        ctrlComboBox* players = AddComboBox(2, DrawPoint(15, 75), Extent(200, 20), TC_GREY, NormalFont, 100);
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
    gwv.RemoveDrawNodeCallback(printer);
    delete printer;
}

void iwMapDebug::Msg_ComboSelectItem(const unsigned ctrl_id, const int select)
{
    if(ctrl_id == 1)
    {
        if(select >= 0)
            printer->showDataIdx = static_cast<unsigned>(select);
    }
    if(ctrl_id == 2)
    {
        if(select >= 0)
            printer->playerIdx = static_cast<unsigned>(select);
    }
}

void iwMapDebug::Msg_CheckboxChange(const unsigned ctrl_id, const bool checked)
{
    if(ctrl_id != 0)
        return;
    printer->showCoords = checked;
}
