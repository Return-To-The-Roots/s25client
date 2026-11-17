// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "desktops/Desktop.h"
#include "network/CreateServerInfo.h"
#include <boost/filesystem/path.hpp>
#include <vector>

struct CampaignDescription;

class dskCampaignSelection : public Desktop
{
public:
    dskCampaignSelection(CreateServerInfo csi);
    ~dskCampaignSelection() noexcept;

protected:
    void drawImpl() override;

private:
    class CampaignDataHolder;

    void msgTableChooseItem(unsigned, unsigned) override;
    void msgTableSelectItem(unsigned ctrl_id, const std::optional<unsigned>& selection) override;
    void msgButtonClick(unsigned ctrl_id) override;
    void msgTimer(unsigned ctrl_id) override;
    void fillCampaignsTable();
    void showCampaignInfo(bool show);
    void showCampaignMissionSelectionScreen();
    void loadCampaigns();

    CreateServerInfo csi_;
    glArchivItem_Bitmap* campaignImage_ = nullptr;
    std::vector<CampaignDescription> campaigns_;
};
