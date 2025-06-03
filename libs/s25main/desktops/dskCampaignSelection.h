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
    void Draw_() override;

private:
    class CampaignDataHolder;

    void Msg_TableChooseItem(unsigned, unsigned) override;
    void Msg_TableSelectItem(unsigned ctrl_id, const boost::optional<unsigned>& selection) override;
    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_Timer(unsigned ctrl_id) override;
    void FillCampaignsTable();
    void showCampaignInfo(bool show);
    void showCampaignMissionSelectionScreen();
    void loadCampaigns();

    CreateServerInfo csi_;
    glArchivItem_Bitmap* campaignImage_ = nullptr;
    std::vector<CampaignDescription> campaigns_;
};
