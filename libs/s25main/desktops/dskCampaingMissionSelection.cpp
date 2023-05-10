// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "dskCampaingMissionSelection.h"
#include "dskSelectMap.h"
#include "Loader.h"
#include "WindowManager.h"
#include "Desktop.h"
#include "Window.h"
#include "ListDir.h"
#include "ogl/glFont.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlTextButton.h"
#include "controls/ctrlMultiline.h"
#include "controls/ctrlGroup.h"
#include "controls/ctrlText.h"
#include "controls/ctrlImageButton.h"
#include <boost/filesystem/operations.hpp>
#include <boost/pointer_cast.hpp>
#include "libsiedler2/ArchivItem_Map.h"
#include "libsiedler2/ArchivItem_Map_Header.h"
#include "libsiedler2/prototypen.h"
#include "libsiedler2/ErrorCodes.h"
#include "helpers/format.hpp"
#include "gameData/MapConsts.h"
#include "gameData/MaxPlayers.h"
#include "s25util/Log.h"
#include "s25util/utf8.h"
#include "commonDefines.h"
#include "CampaingSettings.h"
#include "network/GameClient.h"
#include "RttrLobbyClient.hpp"
#include "ingameWindows/iwConnecting.h"
#include "ingameWindows/iwMsgbox.h"
#include "liblobby/LobbyClient.h"

namespace bfs = boost::filesystem;
constexpr unsigned ID_msgBoxError = 0;

namespace {
enum CtrlIds
{
    ID_BACK,
    ID_FIRST_MISSION_PAGE,
    ID_NEXT_MISSION_PAGE,
    ID_MISSION_PAGE_LABEL,
    ID_PREVIOUS_MISSION_PAGE,
    ID_LAST_MISSION_PAGE,
    ID_CHOOSE_CAPITAL_LABEL,
    ID_CAMPAIGN_LONG_DESCRIPTION,
    ID_MISSION_GROUP_START
};
}

dskCampaignMissionSelection::dskCampaignMissionSelection(CreateServerInfo csi, std::string campaignFolder)
    : Desktop(LOADER.GetImageN("setup015", 0)), campaignFolder_(campaignFolder), csi_(std::move(csi)), currentPage(0),
      lastPage(0), missionsPerPage(10)
{
    const unsigned int btOffset = 50 + LargeFont->getHeight() + NormalFont->getHeight() + 70 + 10 + 2 + missionsPerPage * (20 + 8) + 10;
    AddTextButton(ID_BACK, DrawPoint(300, 560), Extent(200, 22), TextureColor::Red1, _("Back"), NormalFont);

    AddImageButton(ID_FIRST_MISSION_PAGE, DrawPoint(400 - LargeFont->getHeight() * 3 - 2, btOffset),
                   Extent(LargeFont->getHeight(), LargeFont->getHeight()), TextureColor::Green2,
                   LOADER.GetImageN("io", 102));

    AddImageButton(ID_PREVIOUS_MISSION_PAGE, DrawPoint(400 - LargeFont->getHeight() * 2, btOffset),
                   Extent(LargeFont->getHeight(), LargeFont->getHeight()),
                   TextureColor::Green2,
                   LOADER.GetImageN("io", 103));

    AddImageButton(ID_NEXT_MISSION_PAGE, DrawPoint(400 + LargeFont->getHeight(), btOffset),
                   Extent(LargeFont->getHeight(), LargeFont->getHeight()),
                   TextureColor::Green2,
                   LOADER.GetImageN("io", 104));

    AddImageButton(ID_LAST_MISSION_PAGE, DrawPoint(400 + LargeFont->getHeight() * 2 + 2, btOffset),
                   Extent(LargeFont->getHeight(), LargeFont->getHeight()), TextureColor::Green2,
                   LOADER.GetImageN("io", 105));

    auto const campaignIniFile = bfs::path(campaignFolder) / "campaign.ini";
    settings = std::make_unique<CampaignSettings>(campaignIniFile.string());
    settings->Load();
    lastPage = (settings->missions.mapNames.size() - 1) / missionsPerPage;

    UpdateEnabledStateOfNextPreviousButton();

    AddText(ID_MISSION_PAGE_LABEL, DrawPoint(400, btOffset + LargeFont->getHeight() / 2),
            std::to_string(currentPage + 1) + "/" + std::to_string(lastPage + 1), COLOR_YELLOW,
            FontStyle::CENTER | FontStyle::VCENTER,
            LargeFont);

    DrawPoint curBtPos(200, 50);
    AddText(ID_CHOOSE_CAPITAL_LABEL, DrawPoint(400, 50),
            _("Choose capital"), COLOR_YELLOW, FontStyle::CENTER, LargeFont);

    ctrlMultiline* multiline = AddMultiline(ID_CAMPAIGN_LONG_DESCRIPTION, DrawPoint(200, 50 + LargeFont->getHeight() + 10),
                                            Extent(400, 70), TextureColor::Green1, NormalFont);
    multiline->ShowBackground(true);
    multiline->AddString(settings->campaignDescription.longDescription, COLOR_YELLOW);

    UpdateMissionPage(currentPage);
}

void dskCampaignMissionSelection::UpdateMissionPage(const unsigned page)
{
    ctrlGroup* group = AddGroup(ID_MISSION_GROUP_START + page);
    Extent catBtSize = Extent(400, 20);
    DrawPoint curBtPos(200, 50 + LargeFont->getHeight() + NormalFont->getHeight() + 70 + 10 + 2);
    for(unsigned int i = 0; i < missionsPerPage; i++)
    {
        unsigned int missionIndex = page * missionsPerPage + i;
        if(missionIndex >= settings->missions.mapNames.size())
            break;

        const bfs::path& mapFilePath = bfs::path(campaignFolder_) / settings->missions.mapNames[missionIndex];

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

        curBtPos.y += catBtSize.y + 8;
    }
}

void dskCampaignMissionSelection::StartServer(const boost::filesystem::path& mapPath)
{
    // Server starten
    if(!GAMECLIENT.HostGame(csi_, mapPath, MapType::OldMap))
        WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(_("Error"), _("Hosting of game not possible"), this,
                                                      MsgboxButton::Ok, MsgboxIcon::ExclamationRed, ID_msgBoxError));
    else
    {
        std::unique_ptr<ILobbyClient> lobbyClient;
        if(csi_.type == ServerType::Lobby)
            lobbyClient = std::make_unique<RttrLobbyClient>(LOBBYCLIENT);
        iwConnecting& wnd = WINDOWMANAGER.Show(std::make_unique<iwConnecting>(csi_.type, std::move(lobbyClient)));
        onErrorConnection_ = wnd.onError.connect([this](ClientError error) {
            WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(_("Error"), ClientErrorToStr(error), this, MsgboxButton::Ok,
                                                          MsgboxIcon::ExclamationRed, ID_msgBoxError));
        });
    }
}

void dskCampaignMissionSelection::UpdateEnabledStateOfNextPreviousButton()
{
    GetCtrl<ctrlImageButton>(ID_PREVIOUS_MISSION_PAGE)->SetEnabled(currentPage > 0);
    GetCtrl<ctrlImageButton>(ID_NEXT_MISSION_PAGE)->SetEnabled(currentPage < lastPage);
    GetCtrl<ctrlImageButton>(ID_FIRST_MISSION_PAGE)->SetEnabled(currentPage > 0);
    GetCtrl<ctrlImageButton>(ID_LAST_MISSION_PAGE)->SetEnabled(currentPage < lastPage);
}

void dskCampaignMissionSelection::Msg_Group_ButtonClick(const unsigned group_id, const unsigned ctrl_id)
{
    unsigned int missionIndex = (group_id - ID_MISSION_GROUP_START) * missionsPerPage + ctrl_id;
    const bfs::path& mapPath = bfs::path(campaignFolder_) / settings->missions.mapNames[missionIndex];

    StartServer(mapPath);
}

void dskCampaignMissionSelection::Msg_ButtonClick(unsigned ctrl_id)
{
    if(ctrl_id == ID_BACK)
        WINDOWMANAGER.Switch(std::make_unique<dskSelectMap>(csi_, 9));

    if(ctrl_id >= ID_FIRST_MISSION_PAGE && ctrl_id <= ID_LAST_MISSION_PAGE)
    {
        // Alle Controls erstmal zerstören (die ganze Gruppe)
        DeleteCtrl(ID_MISSION_GROUP_START + currentPage);

        switch(ctrl_id)
        {
            case ID_FIRST_MISSION_PAGE:
            {
                currentPage = 0;
                break;
            }
            case ID_NEXT_MISSION_PAGE:
            {
                if(currentPage < lastPage)
                    currentPage++;

                break;
            }
            case ID_PREVIOUS_MISSION_PAGE:
            {
                if(currentPage > 0)
                    currentPage--;
                break;
            }
            case ID_LAST_MISSION_PAGE:
            {
                currentPage = lastPage;
                break;
            }
        }
        UpdateMissionPage(currentPage);
        GetCtrl<ctrlText>(ID_MISSION_PAGE_LABEL)
          ->SetText(std::to_string(currentPage + 1) + "/" + std::to_string(lastPage + 1));
        UpdateEnabledStateOfNextPreviousButton();
    }
}
