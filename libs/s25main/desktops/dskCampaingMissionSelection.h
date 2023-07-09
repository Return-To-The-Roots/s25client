// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "desktops/Desktop.h"
#include "network/CreateServerInfo.h"
#include <boost/filesystem.hpp>
#include <boost/signals2/connection.hpp>

struct CampaignDescription;

class dskCampaignMissionSelection : public Desktop
{
public:
    dskCampaignMissionSelection(CreateServerInfo csi, std::string campaignFolder);

    void UpdateMissionPage(unsigned page);

private:
    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_Group_ButtonClick(unsigned group_id, unsigned ctrl_id) override;
    void StartServer(const boost::filesystem::path& mapPath);
    void UpdateEnabledStateOfNextPreviousButton();
    std::string campaignFolder_;
    CreateServerInfo csi_;
    unsigned int currentPage;
    unsigned int lastPage;
    std::unique_ptr<CampaignDescription> settings;
    unsigned int missionsPerPage;
    boost::signals2::scoped_connection onErrorConnection_;
};
