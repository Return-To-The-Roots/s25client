// Copyright (C) 2005 - 2023 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "dskCampaignMissionSelection.h"
#include "Loader.h"
#include "WindowManager.h"
#include "commonDefines.h"
#include "controls/ctrlGroup.h"
#include "controls/ctrlImageButton.h"
#include "controls/ctrlText.h"
#include "dskCampaignSelection.h"
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
    ID_FirstPage,
    ID_NextPage,
    ID_PageLabel,
    ID_PreviousPage,
    ID_LastPage,
    ID_ChooseMissionLabel,
    ID_GroupStart
};

constexpr int startOffsetY = 70;
constexpr int distanceBetweenElementsY = 10;
constexpr int distanceBetweenMissionButtonsY = 10;
constexpr Extent buttonSize(22, 20);
constexpr int spacingBetweenButtons = 2;

int getStartOffsetMissionButtonsY()
{
    return startOffsetY + LargeFont->getHeight() + distanceBetweenElementsY + 8;
}
} // namespace

dskCampaignMissionSelection::dskCampaignMissionSelection(CreateServerInfo csi, const CampaignDescription& campaign)
    : Desktop(LOADER.GetImageN("setup015", 0)), csi_(std::move(csi)),
      campaign_(std::make_unique<CampaignDescription>(campaign)), currentPage_(0), lastPage_(0), missionsPerPage_(10)
{
    const unsigned btOffset = getStartOffsetMissionButtonsY()
                              + missionsPerPage_ * (buttonSize.y + distanceBetweenMissionButtonsY)
                              + distanceBetweenElementsY;
    AddTextButton(ID_Back, DrawPoint(300, 560), Extent(200, 22), TextureColor::Red1, _("Back"), NormalFont);

    AddImageButton(ID_FirstPage, DrawPoint(400 - buttonSize.x * 3 - 2 * spacingBetweenButtons, btOffset), buttonSize,
                   TextureColor::Green2, LOADER.GetImageN("io", 102));

    AddImageButton(ID_PreviousPage, DrawPoint(400 - buttonSize.x * 2, btOffset), buttonSize, TextureColor::Green2,
                   LOADER.GetImageN("io", 103));

    AddImageButton(ID_NextPage, DrawPoint(400 + buttonSize.x, btOffset), buttonSize, TextureColor::Green2,
                   LOADER.GetImageN("io", 104));

    AddImageButton(ID_LastPage, DrawPoint(400 + buttonSize.x * 2 + 2 * spacingBetweenButtons, btOffset), buttonSize,
                   TextureColor::Green2, LOADER.GetImageN("io", 105));

    if(campaign_->getNumMaps() == 0)
        LOG.write(_("Campaign %1% has no maps.\n")) % campaign_->name;
    else
        lastPage_ = (campaign_->getNumMaps() - 1) / missionsPerPage_;

    UpdateEnabledStateOfNextPreviousButton();

    AddText(ID_PageLabel, DrawPoint(400, btOffset + buttonSize.y / 2),
            std::to_string(currentPage_ + 1) + "/" + std::to_string(lastPage_ + 1), COLOR_YELLOW,
            FontStyle::CENTER | FontStyle::VCENTER, LargeFont);

    AddText(ID_ChooseMissionLabel, DrawPoint(400, startOffsetY), _("Choose mission"), COLOR_YELLOW, FontStyle::CENTER,
            LargeFont);

    UpdateMissionPage();
}

void dskCampaignMissionSelection::UpdateMissionPage()
{
    ctrlGroup* group = AddGroup(ID_GroupStart + currentPage_);
    Extent catBtSize = Extent(300, buttonSize.y);
    DrawPoint curBtPos(250, getStartOffsetMissionButtonsY());
    for(unsigned i = 0; i < missionsPerPage_; i++)
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

        const libsiedler2::ArchivItem_Map_Header& header =
          checkedCast<const libsiedler2::ArchivItem_Map*>(map[0])->getHeader();

        group->AddTextButton(i, curBtPos, catBtSize, TextureColor::Grey, s25util::ansiToUTF8(header.getName()),
                             NormalFont);

        curBtPos.y += catBtSize.y + distanceBetweenMissionButtonsY;
    }
}

void dskCampaignMissionSelection::StartServer(const boost::filesystem::path& mapPath,
                                              const boost::filesystem::path& luaPath)
{
    // Start server
    if(!GAMECLIENT.HostGame(csi_, {mapPath, MapType::OldMap, luaPath}))
    {
        WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(_("Error"), _("Hosting of game not possible"), this,
                                                      MsgboxButton::Ok, MsgboxIcon::ExclamationRed, ID_msgBoxError));
    } else
    {
        iwConnecting& wnd =
          WINDOWMANAGER.Show(std::make_unique<iwConnecting>(csi_.type, std::unique_ptr<ILobbyClient>()));
        onErrorConnection_ = wnd.onError.connect([this](ClientError error) {
            WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(_("Error"), ClientErrorToStr(error), this, MsgboxButton::Ok,
                                                          MsgboxIcon::ExclamationRed, ID_msgBoxError));
        });
    }
}

void dskCampaignMissionSelection::UpdateEnabledStateOfNextPreviousButton()
{
    GetCtrl<ctrlImageButton>(ID_PreviousPage)->SetEnabled(currentPage_ > 0);
    GetCtrl<ctrlImageButton>(ID_NextPage)->SetEnabled(currentPage_ < lastPage_);
    GetCtrl<ctrlImageButton>(ID_FirstPage)->SetEnabled(currentPage_ > 0);
    GetCtrl<ctrlImageButton>(ID_LastPage)->SetEnabled(currentPage_ < lastPage_);
}

void dskCampaignMissionSelection::Msg_Group_ButtonClick(unsigned group_id, unsigned ctrl_id)
{
    const unsigned missionIndex = (group_id - ID_GroupStart) * missionsPerPage_ + ctrl_id;
    StartServer(campaign_->getMapFilePath(missionIndex), campaign_->getLuaFilePath(missionIndex));
}

void dskCampaignMissionSelection::Msg_ButtonClick(unsigned ctrl_id)
{
    if(ctrl_id == ID_Back)
        WINDOWMANAGER.Switch(std::make_unique<dskCampaignSelection>(csi_));
    else if(ctrl_id >= ID_FirstPage && ctrl_id <= ID_LastPage)
    {
        // Destroy all controls first (the whole group)
        DeleteCtrl(ID_GroupStart + currentPage_);

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
        GetCtrl<ctrlText>(ID_PageLabel)
          ->SetText(std::to_string(currentPage_ + 1) + "/" + std::to_string(lastPage_ + 1));
        UpdateEnabledStateOfNextPreviousButton();
    }
}
