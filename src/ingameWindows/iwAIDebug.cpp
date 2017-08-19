// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "defines.h" // IWYU pragma: keep
#include "iwAIDebug.h"

#include "Loader.h"
#include "ai/aijh/AIPlayerJH.h"
#include "controls/ctrlComboBox.h"
#include "controls/ctrlText.h"
#include "ogl/glArchivItem_Font.h"
#include "world/GameWorldView.h"
#include "gameData/const_gui_ids.h"
#include "libutil/colors.h"
#include <boost/foreach.hpp>
#include <sstream>

namespace {
enum
{
    ID_CbPlayer,
    ID_CbOverlay,
    ID_Text
};
}

class iwAIDebug::DebugPrinter : public IDrawNodeCallback
{
public:
    DebugPrinter(const AIJH::AIPlayerJH* ai, unsigned overlay) : ai(ai), overlay(overlay) {}

    const AIJH::AIPlayerJH* ai;
    unsigned overlay;

    void onDraw(const MapPoint& pt, const DrawPoint& curPos) override
    {
        if(overlay == 1)
        {
            if(ai->GetAINode(pt).bq && ai->GetAINode(pt).bq < 7) //-V807
                LOADER.GetMapImageN(49 + ai->GetAINode(pt).bq)->DrawFull(curPos);
        } else if(overlay == 2)
        {
            if(ai->GetAINode(pt).reachable)
                LOADER.GetImageN("io", 32)->DrawFull(curPos);
            else
                LOADER.GetImageN("io", 40)->DrawFull(curPos);
        } else if(overlay == 3)
        {
            if(ai->GetAINode(pt).farmed)
                LOADER.GetImageN("io", 32)->DrawFull(curPos);
            else
                LOADER.GetImageN("io", 40)->DrawFull(curPos);
        } else if(overlay > 3 && overlay < 13)
        {
            std::stringstream ss;
            ss << ai->GetResMapValue(pt, AIJH::Resource(overlay - 4));
            NormalFont->Draw(curPos, ss.str(), 0, 0xFFFFFF00);
        }
    }
};

iwAIDebug::iwAIDebug(GameWorldView& gwv, const std::vector<const AIPlayer*>& ais)
    : IngameWindow(CGI_AI_DEBUG, IngameWindow::posLastOrCenter, Extent(300, 515), _("AI Debug"), LOADER.GetImageN("resource", 41)),
      gwv(gwv), text(NULL), printer(NULL)
{
    BOOST_FOREACH(const AIPlayer* ai, ais)
    {
        const AIJH::AIPlayerJH* aijh = dynamic_cast<const AIJH::AIPlayerJH*>(ai);
        if(aijh)
            ais_.push_back(aijh);
    }
    // Wenn keine KI-Spieler, schließen
    if(ais_.empty())
    {
        Close();
        return;
    }

    ctrlComboBox* players = AddComboBox(ID_CbPlayer, DrawPoint(15, 30), Extent(250, 20), TC_GREY, NormalFont, 100);
    BOOST_FOREACH(const AIJH::AIPlayerJH* ai, ais_)
    {
        players->AddString(ai->GetPlayerName());
    }

    ctrlComboBox* overlays = AddComboBox(ID_CbOverlay, DrawPoint(15, 60), Extent(250, 20), TC_GREY, NormalFont, 100);
    overlays->AddString("None");
    overlays->AddString("BuildingQuality");
    overlays->AddString("Reachability");
    overlays->AddString("Farmed");
    overlays->AddString("Wood");
    overlays->AddString("Stones");
    overlays->AddString("Gold");
    overlays->AddString("Ironore");
    overlays->AddString("Coal");
    overlays->AddString("Granite");
    overlays->AddString("Plantspace");
    overlays->AddString("Borderland");
    overlays->AddString("Fish");

    text = AddText(ID_Text, DrawPoint(15, 120), "", COLOR_YELLOW, glArchivItem_Font::DF_LEFT | glArchivItem_Font::DF_TOP,
                   LOADER.GetFontN("resource", 0));

    players->SetSelection(0);
    overlays->SetSelection(0);
    printer = new DebugPrinter(ais_[0], 0);
    gwv.AddDrawNodeCallback(printer);
}

iwAIDebug::~iwAIDebug()
{
    if(printer)
    {
        gwv.RemoveDrawNodeCallback(printer);
        delete printer;
    }
}

void iwAIDebug::Msg_ComboSelectItem(const unsigned ctrl_id, const int selection)
{
    switch(ctrl_id)
    {
        case ID_CbPlayer: printer->ai = ais_[selection]; break;
        case ID_CbOverlay: printer->overlay = selection; break;
    }
}

void iwAIDebug::Msg_PaintBefore()
{
    IngameWindow::Msg_PaintBefore();
    std::stringstream ss;

    const AIJH::Job* currentJob = printer->ai->GetCurrentJob();
    if(!currentJob)
    {
        text->SetText(_("No current job"));
        return;
    }

    ss << "Jobs to do: " << printer->ai->GetNumJobs() << std::endl << std::endl;

    const AIJH::BuildJob* bj = dynamic_cast<const AIJH::BuildJob*>(currentJob);
    const AIJH::EventJob* ej = dynamic_cast<const AIJH::EventJob*>(currentJob);

    if(bj)
    {
        ss << "BuildJob:" << std::endl;
        ss << BUILDING_NAMES[bj->GetType()] << std::endl;
        ss << bj->GetTarget().x << " / " << bj->GetTarget().y << std::endl;
    } else if(ej)
    {
#define RTTR_PRINT_EV(ev) \
    case AIEvent::ev: ss << #ev << std::endl; break
        switch(ej->GetEvent()->GetType())
        {
            RTTR_PRINT_EV(BuildingDestroyed);
            RTTR_PRINT_EV(BuildingConquered);
            RTTR_PRINT_EV(BuildingLost);
            RTTR_PRINT_EV(BorderChanged);
            RTTR_PRINT_EV(NoMoreResourcesReachable);
            RTTR_PRINT_EV(BuildingFinished);
            RTTR_PRINT_EV(ExpeditionWaiting);
            RTTR_PRINT_EV(TreeChopped);
            RTTR_PRINT_EV(ShipBuilt);
            RTTR_PRINT_EV(ResourceUsed);
            RTTR_PRINT_EV(RoadConstructionComplete);
            RTTR_PRINT_EV(RoadConstructionFailed);
            RTTR_PRINT_EV(NewColonyFounded);
            RTTR_PRINT_EV(LuaConstructionOrder);
            RTTR_PRINT_EV(ResourceFound);
            RTTR_PRINT_EV(LostLand);
            default: ss << "Unknown Event" << std::endl; break;
        }
#undef RTTR_PRINT_EV

        AIEvent::Building* evb = dynamic_cast<AIEvent::Building*>(ej->GetEvent());
        if(evb)
        {
            ss << evb->GetX() << " / " << evb->GetY() << std::endl;
            ss << BUILDING_NAMES[evb->GetBuildingType()] << std::endl;
        }
    }

#define RTTR_PRINT_STATUS(status) \
    case AIJH::status: ss << #status << std::endl; break
    switch(currentJob->GetStatus())
    {
        RTTR_PRINT_STATUS(JOB_WAITING);
        RTTR_PRINT_STATUS(JOB_EXECUTING_START);
        RTTR_PRINT_STATUS(JOB_EXECUTING_ROAD1);
        RTTR_PRINT_STATUS(JOB_EXECUTING_ROAD2);
        RTTR_PRINT_STATUS(JOB_EXECUTING_ROAD2_2);
        RTTR_PRINT_STATUS(JOB_FINISHED);
        RTTR_PRINT_STATUS(JOB_FAILED);
        default: ss << "Unknown status"; break;
    }

    text->SetText(ss.str());
}
