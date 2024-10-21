// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "dskCampaignMissionSelection.h"
#include "Loader.h"
#include "WindowManager.h"
#include "commonDefines.h"
#include "controls/ctrlGroup.h"
#include "controls/ctrlImageButton.h"
#include "controls/ctrlMapSelection.h"
#include "controls/ctrlText.h"
#include "dskCampaignSelection.h"
#include "helpers/Range.h"
#include "ingameWindows/iwConnecting.h"
#include "ingameWindows/iwMsgbox.h"
#include "lua/CampaignDataLoader.h"
#include "network/GameClient.h"
#include "ogl/glFont.h"
#include "gameData/CampaignDescription.h"
#include "libsiedler2/ArchivItem_Map.h"
#include "libsiedler2/ArchivItem_Map_Header.h"
#include "libsiedler2/ErrorCodes.h"
#include "libsiedler2/prototypen.h"
#include "s25util/Log.h"
#include "s25util/utf8.h"

namespace bfs = boost::filesystem;
constexpr unsigned ID_msgBoxError = 0;

namespace {
enum
{
    ID_Back,
    ID_Start,
    ID_MapSelection,
    // IDs used if map selection is not available
    ID_FirstPage,
    ID_NextPage,
    ID_PageLabel,
    ID_PreviousPage,
    ID_LastPage,
    ID_ChooseMissionLabel,
    ID_GroupStart
};

constexpr auto startOffsetY = 70;
constexpr auto distanceBetweenElementsY = 10;
constexpr auto distanceBetweenMissionButtonsY = 10;
constexpr auto buttonHeight = 20;
constexpr auto spacingBetweenButtons = 2;

int getStartOffsetMissionButtonsY()
{
    return startOffsetY + LargeFont->getHeight() + distanceBetweenElementsY + 8;
}
} // namespace

dskCampaignMissionSelection::dskCampaignMissionSelection(CreateServerInfo csi, const CampaignDescription& campaign)
    : Desktop(LOADER.GetImageN("setup015", 0)), csi_(std::move(csi)),
      campaign_(std::make_unique<CampaignDescription>(campaign)), currentPage_(0), missionsPerPage_(10 - 8)
{
    if(campaign_->getNumMaps() == 0)
    {
        LOG.write(_("Campaign %1% has no maps.\n")) % campaign_->name;
        lastPage_ = 0;
    } else
        lastPage_ = (campaign_->getNumMaps() - 1) / missionsPerPage_;
    if(campaign_->selectionMapData)
    {
        constexpr Extent buttonSize(200, buttonHeight);
        AddTextButton(ID_Start, DrawPoint(300, 530), buttonSize, TextureColor::Red1, _("Start"), NormalFont);
        AddTextButton(ID_Back, DrawPoint(300, 530 + buttonSize.y + spacingBetweenButtons), buttonSize,
                      TextureColor::Red1, _("Back"), NormalFont);

        auto* mapSelection =
          AddMapSelection(ID_MapSelection, DrawPoint(0, 0), Extent(800, 508), *campaign_->selectionMapData);
        mapSelection->setMissionsStatus(std::vector<MissionStatus>(campaign_->getNumMaps(), {true, true}));
    } else
    {
        constexpr Extent buttonSize(22, buttonHeight);
        AddText(ID_ChooseMissionLabel, DrawPoint(400, startOffsetY), _("Choose mission"), COLOR_YELLOW,
                FontStyle::CENTER, LargeFont);

        // Add navigation controls
        const unsigned btOffset = getStartOffsetMissionButtonsY()
                                  + missionsPerPage_ * (buttonSize.y + distanceBetweenMissionButtonsY)
                                  + distanceBetweenElementsY;

        const DrawPoint pageLabelPos(400, btOffset + buttonSize.y / 2);

        // width of one button as space around the label center, 2 buttons per side
        // |bt|s|bt|s|(bt)|label|(bt)|s|bt|s|bt
        DrawPoint btPos(pageLabelPos.x - buttonSize.x * 3 - 2 * spacingBetweenButtons, btOffset);
        AddImageButton(ID_FirstPage, btPos, buttonSize, TextureColor::Green2, LOADER.GetImageN("io", 102));
        btPos.x += buttonSize.x + spacingBetweenButtons;
        AddImageButton(ID_PreviousPage, btPos, buttonSize, TextureColor::Green2, LOADER.GetImageN("io", 103));

        AddText(ID_PageLabel, pageLabelPos, "0/0", COLOR_YELLOW, FontStyle::CENTER | FontStyle::VCENTER, LargeFont);

        btPos.x = pageLabelPos.x + buttonSize.x + spacingBetweenButtons;
        AddImageButton(ID_NextPage, btPos, buttonSize, TextureColor::Green2, LOADER.GetImageN("io", 104));
        btPos.x += buttonSize.x + spacingBetweenButtons;
        AddImageButton(ID_LastPage, btPos, buttonSize, TextureColor::Green2, LOADER.GetImageN("io", 105));

        AddTextButton(ID_Back, DrawPoint(300, 560), Extent(200, 22), TextureColor::Red1, _("Back"), NormalFont);

        UpdateStateOfNavigationButtons();
        UpdateMissionPage();
    }
}

void dskCampaignMissionSelection::UpdateMissionPage()
{
    // Hide all groups
    for(const auto i : helpers::range(lastPage_ + 1))
    {
        auto* group = GetCtrl<ctrlGroup>(ID_GroupStart + i);
        if(group)
            group->SetVisible(false);
    }
    ctrlGroup* group = GetCtrl<ctrlGroup>(ID_GroupStart + currentPage_);
    if(group)
        group->SetVisible(true);
    else
    {
        // Create group (once)
        group = AddGroup(ID_GroupStart + currentPage_);

        constexpr Extent missionBtSize(300, buttonHeight);
        DrawPoint curBtPos(250, getStartOffsetMissionButtonsY());
        for(const auto i : helpers::range(missionsPerPage_))
        {
            const unsigned missionIndex = currentPage_ * missionsPerPage_ + i;
            if(missionIndex >= campaign_->getNumMaps())
                break;

            const bfs::path& mapFilePath = campaign_->getMapFilePath(missionIndex);

            libsiedler2::Archiv map;
            if(int ec = libsiedler2::loader::LoadMAP(mapFilePath, map, true))
            {
                LOG.write(_("Failed to load map %1%: %2%\n")) % mapFilePath % libsiedler2::getErrorString(ec);
                continue;
            }

            const auto& header = checkedCast<const libsiedler2::ArchivItem_Map*>(map[0])->getHeader();

            group->AddTextButton(i, curBtPos, missionBtSize, TextureColor::Grey, s25util::ansiToUTF8(header.getName()),
                                 NormalFont);
            curBtPos.y += missionBtSize.y + distanceBetweenMissionButtonsY;
        }
    }
    GetCtrl<ctrlText>(ID_PageLabel)->SetText(std::to_string(currentPage_ + 1) + "/" + std::to_string(lastPage_ + 1));
}

void dskCampaignMissionSelection::StartServer(unsigned missionIdx)
{
    MapDescription map{campaign_->getMapFilePath(missionIdx), MapType::OldMap, campaign_->getLuaFilePath(missionIdx)};
    if(!GAMECLIENT.HostGame(csi_, map))
    {
        WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(_("Error"), _("Hosting of game not possible"), this,
                                                      MsgboxButton::Ok, MsgboxIcon::ExclamationRed, ID_msgBoxError));
    } else
    {
        iwConnecting& wnd = WINDOWMANAGER.Show(std::make_unique<iwConnecting>(csi_.type, nullptr));
        onErrorConnection_ = wnd.onError.connect([this](ClientError error) {
            WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(_("Error"), ClientErrorToStr(error), this, MsgboxButton::Ok,
                                                          MsgboxIcon::ExclamationRed, ID_msgBoxError));
        });
    }
}

void dskCampaignMissionSelection::UpdateStateOfNavigationButtons()
{
    GetCtrl<ctrlImageButton>(ID_FirstPage)->SetEnabled(currentPage_ > 0);
    GetCtrl<ctrlImageButton>(ID_PreviousPage)->SetEnabled(currentPage_ > 0);
    GetCtrl<ctrlImageButton>(ID_NextPage)->SetEnabled(currentPage_ < lastPage_);
    GetCtrl<ctrlImageButton>(ID_LastPage)->SetEnabled(currentPage_ < lastPage_);
}

void dskCampaignMissionSelection::Msg_Group_ButtonClick(unsigned group_id, unsigned ctrl_id)
{
    const auto page = group_id - ID_GroupStart;
    RTTR_Assert(page == currentPage_);
    StartServer(page * missionsPerPage_ + ctrl_id);
}

void dskCampaignMissionSelection::Msg_ButtonClick(unsigned ctrl_id)
{
    if(ctrl_id == ID_Back)
        WINDOWMANAGER.Switch(std::make_unique<dskCampaignSelection>(csi_));
    else if(ctrl_id == ID_Start)
    {
        const auto selectedMission = GetCtrl<ctrlMapSelection>(ID_MapSelection)->getSelection();
        if(selectedMission)
            StartServer(*selectedMission);
    } else if(ctrl_id >= ID_FirstPage && ctrl_id <= ID_LastPage)
    {
        switch(ctrl_id)
        {
            case ID_FirstPage: currentPage_ = 0; break;
            case ID_NextPage:
                if(currentPage_ < lastPage_)
                    currentPage_++;
                break;
            case ID_PreviousPage:
                if(currentPage_ > 0)
                    currentPage_--;
                break;
            case ID_LastPage: currentPage_ = lastPage_; break;
        }
        UpdateMissionPage();
        UpdateStateOfNavigationButtons();
    }
}
