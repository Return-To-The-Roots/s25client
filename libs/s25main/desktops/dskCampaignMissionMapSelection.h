// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "desktops/Desktop.h"
#include "network/CreateServerInfo.h"
#include <boost/filesystem.hpp>
#include <boost/signals2/connection.hpp>

struct CampaignDescription;

/// Select campaign mission from the mission map.
/// Requires that the campaign provides information for the mission map control
class dskCampaignMissionMapSelection : public Desktop
{
public:
    dskCampaignMissionMapSelection(CreateServerInfo csi, const CampaignDescription& campaign);

private:
    void Msg_ButtonClick(unsigned ctrl_id) override;
    void StartServer(const boost::filesystem::path& mapPath, const boost::filesystem::path& luaPath);

    CreateServerInfo csi_;
    std::unique_ptr<CampaignDescription> campaign_;
    boost::signals2::scoped_connection onErrorConnection_;
};
