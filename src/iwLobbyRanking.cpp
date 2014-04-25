// $Id: iwLobbyRanking.cpp 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#include "main.h"
#include "iwLobbyRanking.h"

#include "WindowManager.h"
#include "Loader.h"
#include "controls.h"
#include "LobbyClient.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  aktualisiert die Ranking-Tabelle.
 *
 *  @author Devil
 */
void iwLobbyRanking::UpdateRankings(bool first)
{
    if(LOBBYCLIENT.refreshrankinglist == true)
    {
        const LobbyPlayerList* rankinglist = LOBBYCLIENT.GetRankingList();
        ctrlTable* rankingtable = GetCtrl<ctrlTable>(0);

        rankingtable->DeleteAllItems();

        LOBBYCLIENT.refreshrankinglist = false;

        if(rankinglist->getCount() > 0)
        {
            for(unsigned int i = 0; i < rankinglist->getCount() && i < 10; ++i)
            {
                //const LobbyPlayerInfo *player = rankinglist->getElement(i);
                char gewonnen[128], verloren[128], punkte[128];
                snprintf(punkte, 128, "%d", rankinglist->getElement(i)->getPunkte());
                snprintf(verloren, 128, "%d", rankinglist->getElement(i)->getVerloren());
                snprintf(gewonnen, 128, "%d", rankinglist->getElement(i)->getGewonnen());
                rankingtable->AddRow(0, rankinglist->getElement(i)->getName().c_str(), punkte, verloren, gewonnen);
            }
            if(first)
                rankingtable->SetSelection(0);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p iwLobbyRanking.
 *
 *  @author Devil
 */
iwLobbyRanking::iwLobbyRanking(void)
    : IngameWindow(CGI_LOBBYRANKING, 0xFFFF, 0xFFFF, 440, 410, _("Internet Ranking"), LOADER.GetImageN("resource", 41), true)
{
    AddTable(0, 20, 25, 400, 340, TC_GREY, NormalFont, 4, _("Name"), 360, ctrlTable::SRT_STRING, _("Points"), 185, ctrlTable::SRT_NUMBER, _("Lost"), 215, ctrlTable::SRT_NUMBER, _("Won"), 240, ctrlTable::SRT_NUMBER);
    AddTimer(1, 60000);
    AddTimer(2, 1000);

    // "Zurück"
    AddTextButton(3, 20, 370, 400, 20, TC_RED1, _("Back"), NormalFont);
}

void iwLobbyRanking::Msg_Timer(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        case 1: // alle Minute
        {
            LOBBYCLIENT.SendRankingListRequest();
        } break;
        case 2: // alle Sek
        {
            UpdateRankings();
        } break;
    }
}

void iwLobbyRanking::Msg_ButtonClick(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        case 3: // "Zurück"
        {
            Close();
        } break;
    }
}

