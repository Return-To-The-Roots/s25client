// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Desktop.h"
#include "mapGenerator/MapSettings.h"
#include "network/CreateServerInfo.h"
#include "liblobby/LobbyInterface.h"
#include <boost/filesystem/path.hpp>
#include <boost/signals2/connection.hpp>
#include <set>
#include <string>
#include <vector>

namespace boost {
class thread;
}

class dskSelectMap final : public Desktop, public LobbyInterface
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
    void Msg_TableSelectItem(unsigned ctrl_id, const boost::optional<unsigned>& selection) override;
    void Msg_TableChooseItem(unsigned ctrl_id, unsigned selection) override;

    void LC_Status_Error(const std::string& error) override;

    /**
     * Starts a new game with the currently selected map.
     */
    void StartServer();

    /**
     * Go back to the previous menu.
     */
    void GoBack() const;

    /**
     * Generates a new random map and selects the new map in the table (UI).
     */
    void CreateRandomMap();

    void OnMapCreated(const boost::filesystem::path& mapPath);

    CreateServerInfo csi;
    rttr::mapGenerator::MapSettings rndMapSettings;
    boost::thread* mapGenThread;
    boost::filesystem::path newRandMapPath;
    std::string randMapGenError;
    IngameWindow* waitWnd;
    /// Mapping of s2 ids to landscape names
    std::map<uint8_t, std::string> landscapeNames;
    /// Maps that we already know are broken
    std::set<boost::filesystem::path> brokenMapPaths;
    boost::signals2::scoped_connection onErrorConnection_;
};
