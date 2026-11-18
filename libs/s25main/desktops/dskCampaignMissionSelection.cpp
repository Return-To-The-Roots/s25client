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
    ID_btBack,
    ID_btStart,
    ID_MapSelection,
    // IDs used if map selection is not available
    ID_btFirstPage,
    ID_btNextPage,
    ID_lblPage,
    ID_btPrevPage,
    ID_btLastPage,
    ID_lblChooseMission,
    /// ID of the first group control that makes up pages, must be the last
    ID_FirstPageGrp
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
    : Desktop(LOADER.getImageN("setup015", 0)), csi_(std::move(csi)),
      campaign_(std::make_unique<CampaignDescription>(campaign)), missionsPerPage_(10), currentPage_(0),
      lastPage_((campaign_->getNumMaps() - 1) / missionsPerPage_)
{
    if(campaign_->getNumMaps() == 0)
        LOG.write(_("Campaign %1% has no maps.\n")) % campaign_->name;
    if(campaign_->selectionMapData)
    {
        constexpr Extent buttonSize(200, buttonHeight);
        auto* btStart =
          addTextButton(ID_btStart, DrawPoint(300, 530), buttonSize, TextureColor::Red1, _("Start"), NormalFont);
        addTextButton(ID_btBack, DrawPoint(300, 530 + buttonSize.y + spacingBetweenButtons), buttonSize,
                      TextureColor::Red1, _("Back"), NormalFont);

        auto* mapSelection =
          addMapSelection(ID_MapSelection, DrawPoint(0, 0), Extent(800, 508), *campaign_->selectionMapData);
        mapSelection->setMissionsStatus(std::vector<MissionStatus>(campaign_->getNumMaps(), {true, true}));
        btStart->setEnabled(static_cast<bool>(mapSelection->getSelection()));
    } else
    {
        constexpr Extent buttonSize(22, buttonHeight);
        addText(ID_lblChooseMission, DrawPoint(400, startOffsetY), _("Choose mission"), COLOR_YELLOW, FontStyle::CENTER,
                LargeFont);

        // Add navigation controls
        const unsigned btOffset = getStartOffsetMissionButtonsY()
                                  + missionsPerPage_ * (buttonSize.y + distanceBetweenMissionButtonsY)
                                  + distanceBetweenElementsY;

        const DrawPoint pageLabelPos(400, btOffset + buttonSize.y / 2);

        // width of one button as space around the label center, 2 buttons per side
        // |bt|s|bt|s|(bt)|label|(bt)|s|bt|s|bt
        DrawPoint btPos(pageLabelPos.x - buttonSize.x * 3 - 2 * spacingBetweenButtons, btOffset);
        addImageButton(ID_btFirstPage, btPos, buttonSize, TextureColor::Green2, LOADER.getImageN("io", 102));
        btPos.x += buttonSize.x + spacingBetweenButtons;
        addImageButton(ID_btPrevPage, btPos, buttonSize, TextureColor::Green2, LOADER.getImageN("io", 103));

        addText(ID_lblPage, pageLabelPos, "0/0", COLOR_YELLOW, FontStyle::CENTER | FontStyle::VCENTER, LargeFont);

        btPos.x = pageLabelPos.x + buttonSize.x + spacingBetweenButtons;
        addImageButton(ID_btNextPage, btPos, buttonSize, TextureColor::Green2, LOADER.getImageN("io", 104));
        btPos.x += buttonSize.x + spacingBetweenButtons;
        addImageButton(ID_btLastPage, btPos, buttonSize, TextureColor::Green2, LOADER.getImageN("io", 105));

        addTextButton(ID_btBack, DrawPoint(300, 560), Extent(200, 22), TextureColor::Red1, _("Back"), NormalFont);

        updateStateOfNavigationButtons();
        updateMissionPage();
    }
}

void dskCampaignMissionSelection::updateMissionPage()
{
    // Hide all groups
    for(const auto i : helpers::range(lastPage_ + 1))
    {
        auto* group = getCtrl<ctrlGroup>(ID_FirstPageGrp + i);
        if(group)
            group->setVisible(false);
    }
    const auto grpIdCurrentPage = ID_FirstPageGrp + currentPage_;
    auto* group = getCtrl<ctrlGroup>(grpIdCurrentPage);
    if(group)
        group->setVisible(true);
    else
    {
        // Create group (once)
        group = addGroup(grpIdCurrentPage);

        constexpr Extent missionBtSize(300, buttonHeight);
        DrawPoint curBtPos(250, getStartOffsetMissionButtonsY());
        for(const auto i : helpers::range(missionsPerPage_))
        {
            const unsigned missionIndex = currentPage_ * missionsPerPage_ + i;
            if(missionIndex >= campaign_->getNumMaps())
                break;

            const bfs::path& mapFilePath = campaign_->getMapFilePath(missionIndex);

            libsiedler2::Archiv map;
            if(int ec = libsiedler2::loader::loadMAP(mapFilePath, map, true))
            {
                LOG.write(_("Failed to load map %1%: %2%\n")) % mapFilePath % libsiedler2::getErrorString(ec);
                continue;
            }

            const auto& header = checkedCast<const libsiedler2::ArchivItem_Map*>(map[0])->getHeader();

            group->addTextButton(i, curBtPos, missionBtSize, TextureColor::Grey, s25util::ansiToUTF8(header.getName()),
                                 NormalFont);
            curBtPos.y += missionBtSize.y + distanceBetweenMissionButtonsY;
        }
    }
    getCtrl<ctrlText>(ID_lblPage)->setText(std::to_string(currentPage_ + 1) + "/" + std::to_string(lastPage_ + 1));
}

void dskCampaignMissionSelection::startServer(unsigned missionIdx)
{
    MapDescription map{campaign_->getMapFilePath(missionIdx), MapType::OldMap, campaign_->getLuaFilePath(missionIdx)};
    if(!GAMECLIENT.hostGame(csi_, map))
    {
        WINDOWMANAGER.show(std::make_unique<iwMsgbox>(_("Error"), _("Hosting of game not possible"), this,
                                                      MsgboxButton::Ok, MsgboxIcon::ExclamationRed, ID_msgBoxError));
    } else
    {
        iwConnecting& wnd = WINDOWMANAGER.show(std::make_unique<iwConnecting>(csi_.type, nullptr));
        onErrorConnection_ = wnd.onError.connect([this](ClientError error) {
            WINDOWMANAGER.show(std::make_unique<iwMsgbox>(_("Error"), clientErrorToStr(error), this, MsgboxButton::Ok,
                                                          MsgboxIcon::ExclamationRed, ID_msgBoxError));
        });
    }
}

void dskCampaignMissionSelection::updateStateOfNavigationButtons()
{
    getCtrl<ctrlImageButton>(ID_btFirstPage)->setEnabled(currentPage_ > 0);
    getCtrl<ctrlImageButton>(ID_btPrevPage)->setEnabled(currentPage_ > 0);
    getCtrl<ctrlImageButton>(ID_btNextPage)->setEnabled(currentPage_ < lastPage_);
    getCtrl<ctrlImageButton>(ID_btLastPage)->setEnabled(currentPage_ < lastPage_);
}

void dskCampaignMissionSelection::msgGroupButtonClick(unsigned group_id, unsigned ctrl_id)
{
    const auto page = group_id - ID_FirstPageGrp;
    RTTR_Assert(page == currentPage_);
    startServer(page * missionsPerPage_ + ctrl_id);
}

void dskCampaignMissionSelection::msgButtonClick(unsigned ctrl_id)
{
    if(ctrl_id == ID_btBack)
        WINDOWMANAGER.switchTo(std::make_unique<dskCampaignSelection>(csi_));
    else if(ctrl_id == ID_btStart)
    {
        const auto selectedMission = getCtrl<ctrlMapSelection>(ID_MapSelection)->getSelection();
        if(selectedMission)
            startServer(*selectedMission);
    } else if(ctrl_id == ID_MapSelection)
    {
        auto* btStart = getCtrl<ctrlButton>(ID_btStart);
        const auto* mapSelection = getCtrl<ctrlMapSelection>(ID_MapSelection);
        btStart->setEnabled(static_cast<bool>(mapSelection->getSelection()));
    } else if(ctrl_id >= ID_btFirstPage && ctrl_id <= ID_btLastPage)
    {
        switch(ctrl_id)
        {
            case ID_btFirstPage: currentPage_ = 0; break;
            case ID_btNextPage:
                if(currentPage_ < lastPage_)
                    currentPage_++;
                break;
            case ID_btPrevPage:
                if(currentPage_ > 0)
                    currentPage_--;
                break;
            case ID_btLastPage: currentPage_ = lastPage_; break;
        }
        updateMissionPage();
        updateStateOfNavigationButtons();
    }
}
