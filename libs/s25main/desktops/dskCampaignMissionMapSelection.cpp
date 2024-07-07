// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "dskCampaignMissionMapSelection.h"
#include "Loader.h"
#include "WindowManager.h"
#include "controls/ctrlMapSelection.h"
#include "dskCampaignSelection.h"
#include "ingameWindows/iwConnecting.h"
#include "ingameWindows/iwMsgbox.h"
#include "lua/CampaignDataLoader.h"
#include "network/GameClient.h"
#include "gameData/CampaignDescription.h"
#include "s25util/Log.h"

namespace bfs = boost::filesystem;
constexpr unsigned ID_msgBoxError = 0;

namespace {
enum
{
    ID_Back,
    ID_Start,
    ID_MapSelection
};

constexpr Extent buttonSize(200, 22);
constexpr int spacingBetweenButtons = 2;

} // namespace

dskCampaignMissionMapSelection::dskCampaignMissionMapSelection(CreateServerInfo csi,
                                                               boost::filesystem::path campaignFolder)
    : Desktop(LOADER.GetImageN("setup015", 0)), campaignFolder_(std::move(campaignFolder)), csi_(std::move(csi))
{
    AddTextButton(ID_Start, DrawPoint(300, 530), buttonSize, TextureColor::Red1, _("Start"), NormalFont);
    AddTextButton(ID_Back, DrawPoint(300, 530 + buttonSize.y + spacingBetweenButtons), buttonSize, TextureColor::Red1,
                  _("Back"), NormalFont);

    settings_ = std::make_unique<CampaignDescription>();
    CampaignDataLoader loader(*settings_, campaignFolder_);
    if(!loader.Load() || settings_->getNumMaps() == 0)
        LOG.write(_("Failed to load campaign %1%.\n")) % campaignFolder_;

    if(settings_->selectionMapData.has_value())
    {
        auto* mapSelection =
          AddMapSelection(ID_MapSelection, DrawPoint(0, 0), Extent(800, 508), settings_->selectionMapData.value());
        mapSelection->setMissionsStatus(std::vector<MissionStatus>(settings_->getNumMaps(), {true, true}));
    }
}

void dskCampaignMissionMapSelection::StartServer(const boost::filesystem::path& mapPath,
                                                 const boost::optional<boost::filesystem::path>& luaPath)
{
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

void dskCampaignMissionMapSelection::Msg_ButtonClick(unsigned ctrl_id)
{
    if(ctrl_id == ID_Back)
        WINDOWMANAGER.Switch(std::make_unique<dskCampaignSelection>(csi_));
    else if(ctrl_id == ID_Start)
    {
        if(GetCtrl<ctrlMapSelection>(ID_MapSelection))
        {
            auto selectedMission = GetCtrl<ctrlMapSelection>(ID_MapSelection)->getCurrentSelection();
            if(selectedMission != -1)
            {
                const bfs::path mapPath = settings_->getMapFilePath(selectedMission);
                const bfs::path luaPath = settings_->getLuaFilePath(selectedMission);
                StartServer(mapPath, luaPath);
            }
        }
    }
}
