// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "desktops/Desktop.h"
#include "network/CreateServerInfo.h"
#include <boost/filesystem.hpp>
#include <boost/signals2/connection.hpp>

struct CampaignDescription;

class dskCampaingMainMenu : public Desktop
{
public:
    dskCampaingMainMenu(CreateServerInfo csi, std::string campaignFolder, int selectedCapital);

    //void UpdateMissionPage(const unsigned page);

private:
    void Msg_ButtonClick(unsigned ctrl_id) override;
    void StartServer(const boost::filesystem::path& mapPath);
    int currentSelectedCapital;
    std::string campaignFolder_;
    CreateServerInfo csi_;
    std::unique_ptr<CampaignDescription> settings;
    boost::signals2::scoped_connection onErrorConnection_;
};