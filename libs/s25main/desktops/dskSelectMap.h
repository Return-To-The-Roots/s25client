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
#ifndef dskSELECTMAP_H_INCLUDED
#define dskSELECTMAP_H_INCLUDED

#pragma once

#include "Desktop.h"
#include "mapGenerator/MapSettings.h"
#include "network/ClientInterface.h"
#include "network/CreateServerInfo.h"
#include "liblobby/LobbyInterface.h"
#include <boost/filesystem/path.hpp>
#include <set>
#include <string>
#include <vector>

namespace boost {
class thread;
}

class dskSelectMap final : public Desktop, public ClientInterface, public LobbyInterface
{
public:
    dskSelectMap(CreateServerInfo csi);
    ~dskSelectMap() override;

private:
    void Draw_() override;

    void FillTable(const std::vector<boost::filesystem::path>& files);

    void Msg_OptionGroupChange(unsigned ctrl_id, unsigned selection) override;
    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_MsgBoxResult(unsigned msgbox_id, MsgboxResult mbr) override;
    void Msg_TableSelectItem(unsigned ctrl_id, int selection) override;
    void Msg_TableChooseItem(unsigned ctrl_id, unsigned selection) override;

    void CI_NextConnectState(ConnectState cs) override;
    void CI_Error(ClientError ce) override;

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

    void OnMapCreated(const std::string& mapPath);

    CreateServerInfo csi;
    MapSettings rndMapSettings;
    boost::thread* mapGenThread;
    std::string newRandMapPath;
    IngameWindow* waitWnd;
    /// Mapping of s2 ids to landscape names
    std::map<uint8_t, std::string> landscapeNames;
    /// Maps that we already know are broken
    std::set<boost::filesystem::path> brokenMapPaths;
};

#endif //! dskSELECTMAP_H_INCLUDED
