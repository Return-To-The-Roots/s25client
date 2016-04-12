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
#include "iwShip.h"
#include "Loader.h"
#include "GameClient.h"
#include "nodeObjs/noShip.h"
#include "figures/noFigure.h"
#include "Ware.h"
#include "controls/ctrlButton.h"
#include "ogl/glArchivItem_Bob.h"
#include "ogl/glArchivItem_Font.h"
#include "world/GameWorldView.h"
#include "gameData/JobConsts.h"
#include "gameData/const_gui_ids.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

iwShip::iwShip(GameWorldView& gwv, noShip* const ship)
    : IngameWindow(CGI_SHIP, (unsigned short) - 2, (unsigned short) - 2, 252, 238, _("Ship register"), LOADER.GetImageN("resource", 41)),
      gwv(gwv), ship_id(ship ? GAMECLIENT.GetPlayer(ship->GetPlayer()).GetShipID(ship) : 0), player(ship ? ship->GetPlayer() : GAMECLIENT.GetPlayerID())
{
    AddImage(  0, 126, 101, LOADER.GetImageN("io", 228));
    AddImageButton( 2, 18, 192, 30, 35, TC_GREY, LOADER.GetImageN("io", 225));  // Viewer: 226 - Hilfe
    AddImageButton( 3, 51, 196, 30, 26, TC_GREY, LOADER.GetImageN("io", 102));  // Viewer: 103 - Schnell zurück
    AddImageButton( 4, 81, 196, 30, 26, TC_GREY, LOADER.GetImageN("io", 103));  // Viewer: 104 - Zurück
    AddImageButton( 5, 111, 196, 30, 26, TC_GREY, LOADER.GetImageN("io", 104)); // Viewer: 105 - Vor
    AddImageButton( 6, 141, 196, 30, 26, TC_GREY, LOADER.GetImageN("io", 105)); // Viewer: 106 - Schnell vor
    AddImageButton( 7, 181, 196, 30, 26, TC_GREY, LOADER.GetImageN("io", 107), _("Go to place")); // "Gehe Zu Ort"

    // Die Expeditionsweiterfahrbuttons
    AddImageButton(10, 60, 81, 18, 18, TC_GREY, LOADER.GetImageN("io", 187), _("Found colony"))->SetVisible(false);

    const int BUTTON_POS[6][2] =
    {
        {60, 61}, {80, 70}, {80, 90}, {60, 101}, {40, 90}, {40, 70}
    };

    // Expedition abbrechen
    AddImageButton(11, 200, 143, 18, 18, TC_RED1, LOADER.GetImageN("io", 40), _("Return to harbor"))->SetVisible(false);

    // Die 6 Richtungen
    for(unsigned i = 0; i < 6; ++i)
        AddImageButton(12 + i, BUTTON_POS[i][0], BUTTON_POS[i][1], 18, 18, TC_GREY, LOADER.GetImageN("io", 181 + (i + 4) % 6))->SetVisible(false);

}


void iwShip::Msg_PaintBefore()
{

}

void iwShip::Msg_PaintAfter()
{
    // Schiff holen
    noShip* ship = (player == 0xff) ? NULL : GAMECLIENT.GetPlayer(player).GetShipByID(ship_id);

    // Kein Schiff gefunden? Dann erstes Schiff holen
    if(!ship)
    {
        ship_id = 0;
        // Nochmal probieren
        if(player != 0xff)
            ship = GAMECLIENT.GetPlayer(player).GetShipByID(ship_id);
        // Immer noch nicht? Dann gibt es keine Schiffe mehr und wir zeigen eine entsprechende Meldung an
        if(!ship)
        {
            NormalFont->Draw(GetX() + width_ / 2, GetY() + 60, _("No ships available"), glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_NO_OUTLINE, COLOR_WINDOWBROWN);
            return;
        }
    }


    // Schiffsname
    NormalFont->Draw(GetX() + 42, GetY() + 42, ship->GetName(), glArchivItem_Font::DF_NO_OUTLINE, COLOR_WINDOWBROWN);
    // Schiffs-Nr.
    char str[32];
    sprintf(str, "%u/%u", ship_id + 1, GAMECLIENT.GetPlayer(ship->GetPlayer()).GetShipCount());
    NormalFont->Draw(GetX() + 208, GetY() + 42, str, glArchivItem_Font::DF_RIGHT | glArchivItem_Font::DF_NO_OUTLINE, COLOR_WINDOWBROWN);
    // Das Schiffs-Bild
    LOADER.GetImageN("boot_z", 12)->Draw(GetX() + 138, GetY() + 117);

    // Expeditions-Buttons malen?
    if(ship->IsWaitingForExpeditionInstructions())
    {
        GetCtrl<Window>(10)->SetVisible(ship->IsAbleToFoundColony());
        GetCtrl<Window>(11)->SetVisible(true);

        for(unsigned char i = 0; i < 6; ++i)
            GetCtrl<Window>(12 + i)->SetVisible(gwv.GetViewer().GetNextFreeHarborPoint(ship->GetPos(),
                                                ship->GetCurrentHarbor(), i, ship->GetPlayer()) > 0);

    }
    else
    {
        // Alle Buttons inklusive Anker in der Mitte ausblenden
        for(unsigned i = 0; i < 8; ++i)
            GetCtrl<Window>(10 + i)->SetVisible(false);
    }

    DrawCargo();
}


void iwShip::Msg_ButtonClick(const unsigned int ctrl_id)
{
    noShip* ship = GAMECLIENT.GetPlayer(player).GetShipByID(ship_id);

    if(!ship)
        return;

    // Expeditionskommando? (Schiff weiterfahren lassen, Kolonie gründen)
    if(ctrl_id >= 10 && ctrl_id <= 17)
    {
        if(ctrl_id == 10)
            GAMECLIENT.FoundColony(ship_id);
        else if(ctrl_id == 11)
            GAMECLIENT.CancelExpedition(ship_id);
        else
            GAMECLIENT.TravelToNextSpot(ShipDirection(ctrl_id - 12), ship_id);
        Close();
    }

    switch(ctrl_id)
    {
        default: break;
            // Erstes Schiff
        case 3:
        {
            ship_id = 0;
        } break;
        // Eins zurück
        case 4:
        {
            if(ship_id == 0)
                ship_id = GAMECLIENT.GetPlayer(ship->GetPlayer()).GetShipCount() - 1;
            else
                --ship_id;
        } break;
        // Eins vor
        case 5:
        {
            ++ship_id;
            if(ship_id == GAMECLIENT.GetPlayer(ship->GetPlayer()).GetShipCount())
                ship_id = 0;

        } break;
        // Letztes Schiff
        case 6:
        {
            ship_id = GAMECLIENT.GetPlayer(ship->GetPlayer()).GetShipCount() - 1;
        } break;
        case 7: // "Gehe Zu Ort"
        {
            gwv.MoveToMapPt(ship->GetPos());
        } break;
        /*  case 2: // Hilfe
                {
                //  WINDOWMANAGER.Show(new iwHelp(GUI_ID(CGI_HELPBUILDING+ship->GetShipType()),_(BUILDING_NAMES[ship->GetShipType()]),
                //      _(BUILDING_HELP_STRINGS[ship->GetShipType()])));
                } break;*/
    }
}

void iwShip::DrawCargo()
{
    noShip* ship = GAMECLIENT.GetPlayer(player).GetShipByID(ship_id);

    std::vector<unsigned short> orderedWares = std::vector<unsigned short>(WARE_TYPES_COUNT);
    std::vector<unsigned short> orderedFigures = std::vector<unsigned short>(JOB_TYPES_COUNT);

    // Alle Figuren in Gruppen zählen
    const std::list<noFigure*> figures = ship->GetFigures();
    for(std::list<noFigure*>::const_iterator it = figures.begin(); it != figures.end(); ++it)
    {
        orderedFigures[(*it)->GetJobType()]++;
    }

    // Alle Waren in Gruppen zählen
    const std::list<Ware*> wares = ship->GetWares();
    for(std::list<Ware*>::const_iterator it = wares.begin(); it != wares.end(); ++it)
    {
        orderedWares[(*it)->type]++;
    }

    // Spezialfall Expedition:
    if (ship->IsOnExpedition())
    {
        orderedFigures[JOB_BUILDER] = 1;
        orderedWares[GD_BOARDS] = 4;
        orderedWares[GD_STONES] = 6;
    }
    else if(ship->IsOnExplorationExpedition())
    {
        orderedFigures[JOB_SCOUT] = GAMECLIENT.GetGGS().GetNumScoutsExedition();
    }

    // Start Offset zum malen
    const int xStart = 40 + this->x_;
    const int yStart = 130 + this->y_;

    // Step pro Ware/Figur
    const int xStep = 10;

    // Step pro Zeile
    const int yStep = 15;

    // Elemente pro Zeile
    const unsigned elementsPerLine = 17;

    int x = xStart;
    int y = yStart;

    unsigned lineCounter = 0;

    // Leute zeichnen
    for (unsigned i = 0; i < orderedFigures.size(); ++i)
    {
        while (orderedFigures[i] > 0)
        {
            if (lineCounter > elementsPerLine)
            {
                x = xStart;
                y += yStep;
                lineCounter = 0;
            }
            orderedFigures[i]--;

            unsigned job_bobs_id = JOB_CONSTS[i].jobs_bob_id;
            if((i >= JOB_PRIVATE && i <= JOB_GENERAL) || (i == JOB_SCOUT))
                job_bobs_id += NATION_RTTR_TO_S2[GAMECLIENT.GetPlayer(player).nation] * 6;

            if (i == JOB_PACKDONKEY)
                LOADER.GetMapImageN(2016)->Draw(x, y);
            else if(i == JOB_BOATCARRIER)
                LOADER.GetBobN("carrier")->Draw(GD_BOAT, 5, false, 0, x, y, gwv.GetViewer().GetPlayer(ship->GetPlayer()).color);
            else
                LOADER.GetBobN("jobs")->Draw(job_bobs_id, 5, JOB_CONSTS[i].fat, 0, x, y, gwv.GetViewer().GetPlayer(ship->GetPlayer()).color);

            x += xStep;
            lineCounter++;
        }
    }

    // Waren zeichnen
    for (unsigned i = 0; i < orderedWares.size(); ++i)
    {
        while (orderedWares[i] > 0)
        {
            if (lineCounter > elementsPerLine)
            {
                x = xStart;
                y += yStep;
                lineCounter = 0;
            }
            orderedWares[i]--;

            unsigned draw_id = i;

            // Schilder? Dann das  Schild der jeweiligen Nationalität nehmen
            if(draw_id == GD_SHIELDROMANS)
            {
                switch(GAMECLIENT.GetLocalPlayer().nation)
                {
                    case NAT_AFRICANS: draw_id = GD_SHIELDAFRICANS; break;
                    case NAT_JAPANESE: draw_id = GD_SHIELDJAPANESE; break;
                    case NAT_VIKINGS: draw_id = GD_SHIELDVIKINGS; break;
                    default: break;
                }
            }


            LOADER.GetMapImageN(2200 + draw_id)->Draw(x, y, 0, 0, 0, 0, 0, 0);
            x += xStep;
            lineCounter++;
        }
    }




}
