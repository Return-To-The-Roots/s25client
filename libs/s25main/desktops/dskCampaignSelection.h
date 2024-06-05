// Copyright (C) 2005 - 2023 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "desktops/Desktop.h"
#include "network/CreateServerInfo.h"
#include <boost/filesystem/path.hpp>
#include <set>

class dskCampaignSelection : public Desktop
{
public:
    dskCampaignSelection(CreateServerInfo csi);

protected:
    void Draw_() override;

private:
    void Msg_TableChooseItem(unsigned, unsigned) override;
    void Msg_TableSelectItem(unsigned ctrl_id, const boost::optional<unsigned>& selection) override;
    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_Timer(unsigned ctrl_id) override;
    void FillCampaignsTable();
    void showCampaignInfo(bool show);
    bool hasMapSelectionScreen();
    void showCampaignMissionSelectionScreen();
    boost::filesystem::path getSelectedCampaignPath();
    bool showCampaignInfo_;
    CreateServerInfo csi_;
    glArchivItem_Bitmap* campaignImage_;
    /// Campaigns that we already know are broken
    std::set<boost::filesystem::path> brokenCampaignPaths_;
};
