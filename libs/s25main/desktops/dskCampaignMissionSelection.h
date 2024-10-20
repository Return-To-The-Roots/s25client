// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "desktops/Desktop.h"
#include "network/CreateServerInfo.h"
#include <boost/filesystem.hpp>
#include <boost/signals2/connection.hpp>

struct CampaignDescription;

/// Select campaign mission from list of maps
class dskCampaignMissionSelection : public Desktop
{
public:
    dskCampaignMissionSelection(CreateServerInfo csi, const CampaignDescription& campaign);

private:
    void UpdateMissionPage();
    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_Group_ButtonClick(unsigned group_id, unsigned ctrl_id) override;
    void StartServer(const boost::filesystem::path& mapPath, const boost::filesystem::path& luaPath);
    void UpdateEnabledStateOfNextPreviousButton();
    CreateServerInfo csi_;
    std::unique_ptr<CampaignDescription> campaign_;
    unsigned currentPage_;
    unsigned lastPage_;
    unsigned missionsPerPage_;
    boost::signals2::scoped_connection onErrorConnection_;
};
