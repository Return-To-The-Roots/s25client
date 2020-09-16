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

#include "iwLobbyRanking.h"
#include "Loader.h"
#include "controls/ctrlTable.h"
#include "helpers/toString.h"
#include "gameData/const_gui_ids.h"
#include "liblobby/LobbyClient.h"

/**
 *  aktualisiert die Ranking-Tabelle.
 */
void iwLobbyRanking::UpdateRankings(const LobbyPlayerList& rankinglist)
{
    auto* rankingtable = GetCtrl<ctrlTable>(0);
    bool first = rankingtable->GetNumRows() == 0;

    rankingtable->DeleteAllItems();

    if(!rankinglist.empty())
    {
        for(unsigned i = 0; i < rankinglist.size() && i < 10; ++i)
        {
            const LobbyPlayerInfo& rankInfo = *rankinglist.getElement(i);
            std::string points = helpers::toString(rankInfo.getPunkte());
            std::string numLost = helpers::toString(rankInfo.getVerloren());
            std::string numWon = helpers::toString(rankInfo.getGewonnen());
            rankingtable->AddRow({rankInfo.getName(), points, numLost, numWon});
        }
        if(first)
            rankingtable->SetSelection(0);
    }
}

iwLobbyRanking::iwLobbyRanking()
    : IngameWindow(CGI_LOBBYRANKING, IngameWindow::posLastOrCenter, Extent(440, 410), _("Internet Ranking"),
                   LOADER.GetImageN("resource", 41), true)
{
    using SRT = ctrlTable::SortType;
    AddTable(0, DrawPoint(20, 25), Extent(400, 340), TC_GREY, NormalFont,
             ctrlTable::Columns{{_("Name"), 360, SRT::String},
                                {_("Points"), 185, SRT::Number},
                                {_("Lost"), 215, SRT::Number},
                                {_("Won"), 240, SRT::Number}});
    AddTimer(1, 60000);

    // "Zurück"
    AddTextButton(3, DrawPoint(20, 370), Extent(400, 20), TC_RED1, _("Back"), NormalFont);
}

void iwLobbyRanking::Msg_Timer(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 1: // alle Minute
            LOBBYCLIENT.SendRankingListRequest();
            break;
    }
}

void iwLobbyRanking::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 3: // "Zurück"
        {
            Close();
        }
        break;
    }
}
