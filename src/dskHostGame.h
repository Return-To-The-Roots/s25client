// $Id: dskHostGame.h 9357 2014-04-25 15:35:25Z FloSoft $
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

#ifndef WP_HOSTGAME_H_
#define WP_HOSTGAME_H_

#include "Desktop.h"

#include "GameProtocol.h"
#include "GlobalGameSettings.h"

#include "LobbyInterface.h"
#include "ClientInterface.h"

/// Desktop für das Hosten-eines-Spiels-Fenster
class dskHostGame :
    public Desktop,
    public ClientInterface,
    public LobbyInterface
{
    public:

        /// Map übergeben, damit die Kartenvorschau erstellt werden kann
        dskHostGame(bool single_player = false);

        /// Größe ändern-Reaktionen die nicht vom Skaling-Mechanismus erfasst werden.
        void Resize_(unsigned short width, unsigned short height);
    private:

        void TogglePlayerReady(unsigned char player, bool ready);
        // GGS von den Controls auslesen
        void UpdateGGS();
        /// Aktualisiert eine Spielerreihe (löscht Controls und legt neue an)
        void UpdatePlayerRow(const unsigned row);

        /// Füllt die Felder einer Reihe aus
        void ChangeTeam(const unsigned i, const unsigned char nr);
        void ChangeReady(const unsigned i, const bool ready);
        void ChangeNation(const unsigned i, const Nation nation);
        void ChangePing(const unsigned i);
        void ChangeColor(const unsigned i, const unsigned char color);

        void Msg_PaintBefore();
        void Msg_Group_ButtonClick(const unsigned int group_id, const unsigned int ctrl_id);
        void Msg_Group_CheckboxChange(const unsigned int group_id, const unsigned int ctrl_id, const bool checked);
        void Msg_Group_ComboSelectItem(const unsigned int group_id, const unsigned int ctrl_id, const unsigned short selection);
        void Msg_ButtonClick(const unsigned int ctrl_id);
        void Msg_EditEnter(const unsigned int ctrl_id);
        void Msg_MsgBoxResult(const unsigned msgbox_id, const MsgboxResult mbr);
        void Msg_ComboSelectItem(const unsigned int ctrl_id, const unsigned short selection);
        void Msg_CheckboxChange(const unsigned int ctrl_id, const bool checked);

        void LC_RankingInfo(const LobbyPlayerInfo& player);

        void CI_Error(const ClientError ce);

        void CI_NewPlayer(const unsigned player_id);
        void CI_PlayerLeft(const unsigned player_id);

        void CI_GameStarted(GameWorldViewer* gwv);

        void CI_PSChanged(const unsigned player_id, const PlayerState ps);
        void CI_NationChanged(const unsigned player_id, const Nation nation);
        void CI_TeamChanged(const unsigned player_id, const unsigned char team);
        void CI_PingChanged(const unsigned player_id, const unsigned short ping);
        void CI_ColorChanged(const unsigned player_id, const unsigned char color);
        void CI_ReadyChanged(const unsigned player_id, const bool ready);
        void CI_PlayersSwapped(const unsigned player1, const unsigned player2);
        void CI_GGSChanged(const GlobalGameSettings& ggs);

        void CI_Chat(const unsigned player_id, const ChatDestination cd, const std::string& msg);
        void CI_Countdown(int countdown);
        void CI_CancelCountdown();

        void LC_Status_Error(const std::string& error);

    private:
        int temppunkte; // TODO - wegmachen und durch korrekte punkte ersetzen!
        glArchivItem_Bitmap_Raw preview;
        GlobalGameSettings ggs;
        bool has_countdown;
        bool single_player;
};


#endif
