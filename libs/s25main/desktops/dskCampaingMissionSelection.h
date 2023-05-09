// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "desktops/Desktop.h"
#include "network/CreateServerInfo.h"
#include <boost/signals2/connection.hpp>
#include <boost/filesystem.hpp>

class CampaignSettings;

class dskCampaignMissionSelection : public Desktop
{
public:
    dskCampaignMissionSelection(CreateServerInfo csi, std::string campaignFolder);

    void UpdateMissionPage(const unsigned page);

private:
    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_Group_ButtonClick(const unsigned group_id, const unsigned ctrl_id) override;
    void StartServer(const boost::filesystem::path& mapPath);
    void UpdateEnabledStateOfNextPreviousButton();
    std::string campaignFolder_;
    CreateServerInfo csi_;
    unsigned int currentPage;
    unsigned int lastPage;
    std::unique_ptr<CampaignSettings> settings;
    unsigned int missionsPerPage;
    boost::signals2::scoped_connection onErrorConnection_;
};
