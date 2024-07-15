// Copyright (C) 2005 - 2023 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "CreatedFrom.h"
#include "desktops/Desktop.h"
#include "network/CreateServerInfo.h"
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <boost/signals2/connection.hpp>

struct CampaignDescription;

class dskCampaignMissionSelection : public Desktop
{
public:
    dskCampaignMissionSelection(CreateServerInfo csi, boost::filesystem::path campaignFolder, CreatedFrom createdFrom);
    ~dskCampaignMissionSelection();

private:
    void UpdateMissionPage();
    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_Group_ButtonClick(unsigned group_id, unsigned ctrl_id) override;
    void StartServer(const boost::filesystem::path& mapPath, const boost::optional<boost::filesystem::path>& luaPath);
    void UpdateEnabledStateOfNextPreviousButton();
    boost::filesystem::path campaignFolder_;
    CreateServerInfo csi_;
    unsigned currentPage_;
    unsigned lastPage_;
    std::unique_ptr<CampaignDescription> settings_;
    unsigned missionsPerPage_;
    boost::signals2::scoped_connection onErrorConnection_;
    CreatedFrom createdFrom_;
};
