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
#ifndef dskSELECTMAP_H_INCLUDED
#define dskSELECTMAP_H_INCLUDED

#pragma once

#include "Desktop.h"
#include "mapGenerator/MapSettings.h"

#include "ClientInterface.h"
#include "ingameWindows/iwDirectIPCreate.h"
#include "liblobby/LobbyInterface.h"
#include <string>
#include <vector>

namespace boost
{
    class thread;
}

class dskSelectMap : public Desktop, public ClientInterface, public LobbyInterface
{
public:
    dskSelectMap(const CreateServerInfo& csi);
    ~dskSelectMap() override;

private:
    void Draw_() override;

    void FillTable(const std::vector<std::string>& files);

    void Msg_OptionGroupChange(const unsigned ctrl_id, const int selection) override;
    void Msg_ButtonClick(const unsigned ctrl_id) override;
    void Msg_MsgBoxResult(const unsigned msgbox_id, const MsgboxResult mbr) override;
    void Msg_TableSelectItem(const unsigned ctrl_id, const int selection) override;
    void Msg_TableChooseItem(const unsigned ctrl_id, const unsigned selection) override;

    void CI_NextConnectState(const ConnectState cs) override;
    void CI_Error(const ClientError ce) override;

    void LC_Created() override;
    void LC_Status_Error(const std::string& error) override;

    /**
     * Starts a new game with the currently selected map.
     */
    void StartServer();

    /**
     * Go back to the previous menu.
     */
    void GoBack();

    /**
     * Generates a new random map and selects the new map in the table (UI).
     */
    void CreateRandomMap();

    void OnMapCreated(std::string mapPath);

    CreateServerInfo csi;
    MapSettings rndMapSettings;
    boost::thread* mapGenThread;
    std::string newRandMapPath;
    IngameWindow* waitWnd;
};

#endif //! dskSELECTMAP_H_INCLUDED
