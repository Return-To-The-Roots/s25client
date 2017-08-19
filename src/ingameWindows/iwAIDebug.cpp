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

iwAIDebug::iwAIDebug(GameWorldView& gwv, const std::vector<AIPlayer*>& ais)
    : IngameWindow(CGI_AI_DEBUG, IngameWindow::posLastOrCenter, Extent(300, 515), _("AI Debug"), LOADER.GetImageN("resource", 41)), gwv(gwv)
{
    for(std::vector<AIPlayer*>::const_iterator it = ais.begin(); it != ais.end(); ++it)
    {
        AIJH::AIPlayerJH* ai = dynamic_cast<AIJH::AIPlayerJH*>(*it);
        if(ai)
            ais_.push_back(ai);
    }
    // Wenn keine KI-Spieler, schließen
    if(ais_.empty())
    {
        Close();
        return;
    }

    ctrlComboBox* players = AddComboBox(1, DrawPoint(15, 30), Extent(250, 20), TC_GREY, NormalFont, 100);
    for(std::vector<AIJH::AIPlayerJH*>::const_iterator it = ais_.begin(); it != ais_.end(); ++it)
    {
        players->AddString((*it)->GetPlayerName());
    }

    player_ = 0;
    players->SetSelection(player_);

    ctrlComboBox* overlays = AddComboBox(0, DrawPoint(15, 60), Extent(250, 20), TC_GREY, NormalFont, 100);
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
    overlay_ = 0;
    overlays->SetSelection(overlay_);

    // jobs = AddList(1, DrawPoint(15, 60), Extent(120, 220), TC_GREY, NormalFont);

    text = AddText(2, DrawPoint(15, 120), "", COLOR_YELLOW, glArchivItem_Font::DF_LEFT | glArchivItem_Font::DF_TOP,
                   LOADER.GetFontN("resource", 0));

    // for(unsigned char i = 0; i < 31; ++i)
    //  list->AddString(_(BUILDING_NAMES[GAMECLIENT.visual_settings.build_order[i]]));
    // list->SetSelection(0);
}

void iwAIDebug::Msg_ComboSelectItem(const unsigned ctrl_id, const int selection)
{
    switch(ctrl_id)
    {
        default: break;

        case 1:
        {
            player_ = selection;
            gwv.SetAIDebug(overlay_, ais_[player_]->GetPlayerId(), true);
        }
        break;
        case 0:
        {
            overlay_ = selection;
            gwv.SetAIDebug(overlay_, ais_[player_]->GetPlayerId(), true);
        }
        break;
    }
}

void iwAIDebug::Msg_PaintBefore()
{
    IngameWindow::Msg_PaintBefore();
    std::stringstream ss;

    AIJH::Job* currentJob = ais_[player_]->GetCurrentJob();
    if(!currentJob)
    {
        text->SetText(_("No current job"));
        return;
    }

    ss << "Jobs to do: " << ais_[player_]->GetJobNum() << std::endl << std::endl;

    AIJH::BuildJob* bj = dynamic_cast<AIJH::BuildJob*>(currentJob);
    AIJH::EventJob* ej = dynamic_cast<AIJH::EventJob*>(currentJob);

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

    switch(currentJob->GetStatus())
    {
        case AIJH::JOB_WAITING: ss << "JOB_WAITING"; break;
        case AIJH::JOB_EXECUTING_START: ss << "JOB_EXECUTING_START"; break;
        case AIJH::JOB_EXECUTING_ROAD1: ss << "JOB_EXECUTING_ROAD1"; break;
        case AIJH::JOB_EXECUTING_ROAD2: ss << "JOB_EXECUTING_ROAD2"; break;
        case AIJH::JOB_EXECUTING_ROAD2_2: ss << "JOB_EXECUTING_ROAD2_2"; break;
        case AIJH::JOB_FINISHED: ss << "JOB_FINISHED"; break;
        case AIJH::JOB_FAILED: ss << "JOB_FAILED"; break;
        default: ss << "Unknown status"; break;
    }

    text->SetText(ss.str());
}
