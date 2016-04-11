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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h" // IWYU pragma: keep
#include "iwAIDebug.h"

#include "GameClient.h"
#include "GameServer.h"
#include "ai/AIPlayerJH.h"
#include "controls/ctrlComboBox.h"
#include "controls/ctrlText.h"
#include "world/GameWorldView.h"
#include "Loader.h"
#include "ogl/glArchivItem_Font.h"
#include "gameData/const_gui_ids.h"
#include "libutil/src/colors.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

iwAIDebug::iwAIDebug(GameWorldView& gwv)
    : IngameWindow(CGI_OPTIONSWINDOW, 0xFFFF, 0xFFFF, 300, 515, _("AI Debug"), LOADER.GetImageN("resource", 41)),
      gwv(gwv)
{
    // Nur Host hat Zugriff auf die Daten über die KI-Spieler
    if (!GAMECLIENT.IsHost())
    {
        Close();
        return;
    }

    // Erstmal die AIs einsammeln
    for (unsigned i = 0; i < GAMECLIENT.GetPlayerCount(); ++i)
    {
        AIPlayerJH* ai = dynamic_cast<AIPlayerJH*>(GAMESERVER.GetAIPlayer(i));
        if (ai)
            ais.push_back(ai);
    }

    // Wenn keine KI-Spieler, schließen
    if (ais.empty())
    {
        Close();
        return;
    }

	ctrlComboBox* players = AddComboBox(1, 15, 30, 250, 20, TC_GREY, NormalFont, 100);
    for (std::vector<AIPlayerJH*>::iterator it = ais.begin(); it != ais.end(); ++it)
    {
        players->AddString((*it)->GetPlayerName());
    }

    selection = 0;
    players->SetSelection(selection);

    ctrlComboBox* overlays = AddComboBox(0, 15, 60, 250, 20, TC_GREY, NormalFont, 100);
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
    overlay = 0;
    overlays->SetSelection(overlay);

	

    //jobs = AddList(1, 15, 60, 120, 220, TC_GREY, NormalFont);

    text = AddText(2, 15, 120, "", COLOR_YELLOW,
                   glArchivItem_Font::DF_LEFT | glArchivItem_Font::DF_TOP, LOADER.GetFontN("resource", 0));

    //for(unsigned char i = 0; i < 31; ++i)
    //  list->AddString(_(BUILDING_NAMES[GAMECLIENT.visual_settings.build_order[i]]));
    //list->SetSelection(0);
}

void iwAIDebug::Msg_ComboSelectItem(const unsigned int ctrl_id, const int selection)
{
    switch(ctrl_id)
    {
        default:
            break;

        case 1:
        {
            this->selection = selection;
            gwv.SetAIDebug(overlay, ais[selection]->GetPlayerID(), false);
        } break;
        case 0:
        {
            overlay = selection;
            gwv.SetAIDebug(overlay, ais[selection]->GetPlayerID(), true);
        } break;
    }
}

void iwAIDebug::Msg_PaintBefore()
{
    std::stringstream ss;

    AIJH::Job* currentJob = ais[selection]->GetCurrentJob();
    if (!currentJob)
    {
        text->SetText(_("No current job"));
        return;
    }

    ss << "Jobs to do: " << ais[selection]->GetJobNum() << std::endl << std::endl;

    AIJH::BuildJob* bj = dynamic_cast<AIJH::BuildJob*>(currentJob);
    AIJH::EventJob* ej = dynamic_cast<AIJH::EventJob*>(currentJob);

    if (bj)
    {
        ss << "BuildJob:" << std::endl;
        ss << BUILDING_NAMES[bj->GetType()] << std::endl;
        ss << bj->GetTarget().x << " / " << bj->GetTarget().y << std::endl;
    }
    else if (ej)
    {
        ss << "EventJob:" << std::endl;
        switch(ej->GetEvent()->GetType())
        {
            case AIEvent::BuildingDestroyed: ss << "BuildingDestroyed" << std::endl; break;
            case AIEvent::BuildingConquered: ss << "BuildingConquered" << std::endl; break;
            case AIEvent::BuildingLost: ss << "BuildingLost" << std::endl; break;
            case AIEvent::BuildingOccupied: ss << "BuildingOccupied" << std::endl; break;
            case AIEvent::BorderChanged: ss << "BorderChanged" << std::endl; break;
            case AIEvent::TerritoryLost: ss << "world/TerritoryLost" << std::endl; break;
            case AIEvent::NoMoreResourcesReachable: ss << "NoMoreResourcesReachable" << std::endl; break;
            case AIEvent::BuildingFinished: ss << "BuildingFinished" << std::endl; break;
            case AIEvent::ExpeditionWaiting: ss << "ExpeditionWaiting" << std::endl; break;
            default: ss << "Unknown Event" << std::endl; break;
        }

        AIEvent::Building* evb = dynamic_cast<AIEvent::Building*>(ej->GetEvent());
        if (evb)
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
