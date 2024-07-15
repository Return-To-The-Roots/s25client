// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
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

class dskCampaignMissionMapSelection : public Desktop
{
public:
    dskCampaignMissionMapSelection(CreateServerInfo csi, boost::filesystem::path campaignFolder,
                                   CreatedFrom createdFrom);
    ~dskCampaignMissionMapSelection();

private:
    void Msg_ButtonClick(unsigned ctrl_id) override;
    void StartServer(const boost::filesystem::path& mapPath, const boost::optional<boost::filesystem::path>& luaPath);

    boost::filesystem::path campaignFolder_;
    CreateServerInfo csi_;
    std::unique_ptr<CampaignDescription> settings_;
    boost::signals2::scoped_connection onErrorConnection_;
    CreatedFrom createdFrom_;
};
