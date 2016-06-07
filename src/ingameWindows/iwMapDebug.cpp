// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#include "defines.h" // IWYU-pragma: keep
#include "iwMapDebug.h"
#include "controls/ctrlComboBox.h"
#include "controls/ctrlCheck.h"
#include "Loader.h"
#include "ogl/glArchivItem_Font.h"
#include "world/GameWorldView.h"
#include "world/GameWorldBase.h"
#include "gameTypes/TextureColor.h"
#include "gameData/const_gui_ids.h"
#include "helpers/converters.h"

class iwMapDebug::DebugPrinter: public IDebugNodePrinter
{
public:
    DebugPrinter(const World& gwb): showCoords(true), showDataIdx(0), gw(gwb), font(NormalFont){}

    void print(const MapPoint& pt, const DrawPoint& displayPt) override
    {
        if(showCoords){
            std::string coord = helpers::toString(pt.x) + ":" + helpers::toString(pt.y);
            font->Draw(displayPt, coord, 0, 0xFFFFFF00);
        }
        std::string data;
        unsigned color = 0xFFFF0000;
        const MapNode& node = gw.GetNode(pt);
        switch(showDataIdx)
        {
        case 1:
            if(node.reserved)
                data = "R";
            break;
        case 2:
            data = helpers::toString(static_cast<unsigned>(node.altitude));
            break;
        case 3:
            data = helpers::toString(static_cast<unsigned>(node.resources));
            break;
        case 4:
            if(node.seaId)
                data = helpers::toString(node.seaId);
            else if(gw.GetSeaFromCoastalPoint(pt))
                data = "C";
            break;
        case 5:
            data = helpers::toString(static_cast<unsigned>(node.owner));
            break;
        default:
            return;
        }        

        if(!data.empty())
            font->Draw(displayPt, data, 0, color);
    }

    bool showCoords;
    unsigned showDataIdx;
    const World& gw;
    glArchivItem_Font* font;
};

iwMapDebug::iwMapDebug(GameWorldView& gwv):
    IngameWindow(CGI_MAP_DEBUG, 0xFFFF, 0xFFFF, 300, 200, _("Map Debug"), LOADER.GetImageN("resource", 41)),
    gwv(gwv), printer(new DebugPrinter(gwv.GetWorld()))
{
    gwv.SetDebugNodePrinter(printer);

    ctrlCheck* cbShowCoords = AddCheckBox(0, 15, 30, 250, 20, TC_GREY, _("Show coordinates"), NormalFont);
    cbShowCoords->SetCheck(true);
    ctrlComboBox* data = AddComboBox(1, 15, 60, 250, 20, TC_GREY, NormalFont, 100);
    data->AddString(_("Nothing"));
    data->AddString(_("Reserved"));
    data->AddString(_("Altitude"));
    data->AddString(_("Resources"));
    data->AddString(_("Sea Id"));
    data->AddString(_("Owner"));
    data->SetSelection(1);

    printer->showCoords = cbShowCoords->GetCheck();
    printer->showDataIdx = data->GetSelection();
}

iwMapDebug::~iwMapDebug()
{
    gwv.SetDebugNodePrinter(NULL);
    delete printer;
}

void iwMapDebug::Msg_ComboSelectItem(const unsigned int ctrl_id, const int select)
{
    if(ctrl_id != 1)
        return;
    if(select >= 0)
        printer->showDataIdx = static_cast<unsigned>(select);
}

void iwMapDebug::Msg_CheckboxChange(const unsigned int ctrl_id, const bool checked)
{
    if(ctrl_id != 0)
        return;
    printer->showCoords = checked;
}
