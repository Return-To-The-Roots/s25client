// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "desktops/Desktop.h"
#include "network/CreateServerInfo.h"
#include <boost/signals2/connection.hpp>

struct CampaignDescription;

class dskCampaignMissionSelection : public Desktop
{
public:
    dskCampaignMissionSelection(CreateServerInfo csi, const CampaignDescription& campaign);

private:
    void UpdateMissionPage();
    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_Group_ButtonClick(unsigned group_id, unsigned ctrl_id) override;
    void StartServer(unsigned missionIdx);
    void UpdateStateOfNavigationButtons();
    CreateServerInfo csi_;
    std::unique_ptr<CampaignDescription> campaign_;
    const unsigned missionsPerPage_;
    /// current page (zero based) in the paged mission list
    unsigned currentPage_;
    /// last page in the paged mission list
    const unsigned lastPage_;
    boost::signals2::scoped_connection onErrorConnection_;
};
