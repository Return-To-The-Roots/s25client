// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "dskCampaingMainMenu.h"
#include "dskCampaingMissionSelection.h"
#include "gameData/CampaignDescription.h"
#include "lua/CampaignDataLoader.h"
#include "Desktop.h"
#include "ListDir.h"
#include "Loader.h"
#include "RttrConfig.h"
#include "RttrLobbyClient.hpp"
#include "Window.h"
#include "WindowManager.h"
#include "commonDefines.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlGroup.h"
#include "controls/ctrlImageButton.h"
#include "controls/ctrlMultiline.h"
#include "controls/ctrlText.h"
#include "controls/ctrlTextButton.h"
#include "dskSelectMap.h"
#include "files.h"
#include "helpers/format.hpp"
#include "ingameWindows/iwConnecting.h"
#include "ingameWindows/iwMsgbox.h"
#include "network/GameClient.h"
#include "ogl/glFont.h"
#include "gameData/MapConsts.h"
#include "gameData/MaxPlayers.h"
#include "liblobby/LobbyClient.h"
#include "libsiedler2/ArchivItem_Map.h"
#include "libsiedler2/ArchivItem_Map_Header.h"
#include "libsiedler2/ErrorCodes.h"
#include "libsiedler2/prototypen.h"
#include "s25util/Log.h"
#include "s25util/utf8.h"
#include <boost/filesystem/operations.hpp>
#include <boost/pointer_cast.hpp>

namespace bfs = boost::filesystem;
constexpr unsigned ID_msgBoxError = 0;

namespace {
enum CtrlIds
{
    ID_CAMPAIGN_LABEL,
    ID_BACK,
    ID_PLAY_CAPITAL,
    ID_CHOOSE_CAPITAL,
    ID_SELECTED_CAPITAL_LABEL,
    ID_CAMPAIGN_LONG_DESCRIPTION
};

int startOffsetY = 70;
}

dskCampaingMainMenu::dskCampaingMainMenu(CreateServerInfo csi, std::string campaignFolder, int selectedCapital)
    : Desktop(LOADER.GetImageN("setup013", 0)), campaignFolder_(campaignFolder),
      csi_(std::move(csi)), currentSelectedCapital (selectedCapital)
{
    settings = std::make_unique<CampaignDescription>();
    CampaignDataLoader loader(*settings, campaignFolder);
    loader.Load();

    AddText(ID_CAMPAIGN_LABEL, DrawPoint(400, startOffsetY), _("Campaign"), COLOR_YELLOW, FontStyle::CENTER,
            LargeFont);

    ctrlMultiline* multiline = AddMultiline(ID_CAMPAIGN_LONG_DESCRIPTION, DrawPoint(200, startOffsetY + 50),
                                            Extent(400, 250), TextureColor::Green1, NormalFont);
    multiline->ShowBackground(true);
    std::string campaingDescription = _("Title:   ") + settings->shortDescription + "\n"
                                      + _("Author:  ") + settings->author + "\n" + _("Maps:    ")
                                      + std::to_string(settings->mapNames.size()) + "\n\n"
                                      + settings->longDescription;
    multiline->AddString(campaingDescription, COLOR_YELLOW);

    const bfs::path& mapFilePath =
      RTTRCONFIG.ExpandPath(settings->mapFolder) / settings->mapNames[currentSelectedCapital];

    libsiedler2::Archiv map;
    if(int ec = libsiedler2::loader::LoadMAP(mapFilePath, map, true))
    {
        LOG.write(_("Failed to load map %1%: %2%\n")) % mapFilePath % libsiedler2::getErrorString(ec);
    }

    const libsiedler2::ArchivItem_Map_Header& header =
      checkedCast<const libsiedler2::ArchivItem_Map*>(map[0])->getHeader();

    AddTextButton(ID_SELECTED_CAPITAL_LABEL, DrawPoint(300, 464), Extent(200, 22), TextureColor::Invisible,
                  s25util::ansiToUTF8(header.getName()),
                  NormalFont);
    AddTextButton(ID_CHOOSE_CAPITAL, DrawPoint(300, 494), Extent(200, 22), TextureColor::Green2, _("Choose capital"), NormalFont);
    AddTextButton(ID_BACK, DrawPoint(300, 560), Extent(200, 22), TextureColor::Red1, _("Back"), NormalFont);
    AddTextButton(ID_PLAY_CAPITAL, DrawPoint(510, 560), Extent(200, 22), TextureColor::Green2, _("Play capital"),
                  NormalFont);
}

void dskCampaingMainMenu::Msg_ButtonClick(unsigned ctrl_id)
{
    if(ctrl_id == ID_BACK)
        WINDOWMANAGER.Switch(std::make_unique<dskSelectMap>(csi_, 9));

    if(ctrl_id == ID_PLAY_CAPITAL)
    {
        const bfs::path& mapPath =
          RTTRCONFIG.ExpandPath(settings->mapFolder) / settings->mapNames[currentSelectedCapital];
        StartServer(mapPath);
    }

    if(ctrl_id == ID_CHOOSE_CAPITAL)
    {
        WINDOWMANAGER.Switch(
          std::make_unique<dskCampaignMissionSelection>(csi_, campaignFolder_, currentSelectedCapital));
    }
}

void dskCampaingMainMenu::StartServer(const boost::filesystem::path& mapPath)
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
