// Copyright (C) 2005 - 2022 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwConnecting.h"
#include "Loader.h"
#include "WindowManager.h"
#include "controls/ctrlPercent.h"
#include "controls/ctrlText.h"
#include "desktops/dskGameLobby.h"
#include "desktops/dskGameInterface.h"
#include "iwMsgbox.h"
#include "network/GameClient.h"
#include "gameData/const_gui_ids.h"
#include "s25util/colors.h"
#include <cmath>

namespace {
enum
{
    ID_txtStatus,
    ID_prgLoad,
    ID_btAbort,
};
}

iwConnecting::iwConnecting(ServerType serverType, std::unique_ptr<ILobbyClient> lobbyClient)
    : IngameWindow(CGI_CONNECTING, IngameWindow::posCenter, Extent(300, 120), _("Connecting"),
                   LOADER.GetImageN("resource", 41), true, CloseBehavior::Custom),
      serverType_(serverType), lobbyClient_(std::move(lobbyClient))
{
    WINDOWMANAGER.SetCursor(Cursor::Moon);

    AddText(ID_txtStatus, DrawPoint(150, 30), _("Please wait..."), COLOR_YELLOW, FontStyle::CENTER, NormalFont);
    AddPercent(ID_prgLoad, DrawPoint(20, 50), Extent(260, 26), TextureColor::Grey, COLOR_YELLOW, SmallFont,
               &downloadProgress_)
      ->SetVisible(false);
    AddTextButton(ID_btAbort, DrawPoint(87, 80), Extent(125, 22), TextureColor::Red1, _("Abort"), NormalFont);

    GAMECLIENT.SetInterface(this);
}

void iwConnecting::Close()
{
    IngameWindow::Close();
    WINDOWMANAGER.SetCursor();
    GAMECLIENT.RemoveInterface(this);
}

void iwConnecting::Msg_ButtonClick(unsigned ctrlId)
{
    RTTR_Assert(ctrlId == ID_btAbort);
    GAMECLIENT.Stop();
    CI_Error(ClientError::ConnectionLost);
}

void iwConnecting::CI_Error(const ClientError ce)
{
    if(onError.empty())
    {
        WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(_("Error"), ClientErrorToStr(ce), nullptr, MsgboxButton::Ok,
                                                      MsgboxIcon::ExclamationRed, 0));
    } else
        onError(ce);
    Close();
}

void iwConnecting::CI_NextConnectState(const ConnectState cs)
{
    auto* prgLoad = GetCtrl<ctrlPercent>(ID_prgLoad);
    prgLoad->SetVisible(false);
    switch(cs)
    {
        case ConnectState::Initiated: setStatus(_("Waiting for Reply...")); break;
        case ConnectState::VerifyServer: setStatus(_("Verifying server info...")); break;
        case ConnectState::QueryPw: setStatus(_("Checking Password...")); break;
        case ConnectState::QueryMapInfo: setStatus(_("Checking Map...")); break;
        case ConnectState::ReceiveMap:
            setStatus(_("Receiving Map..."));
            prgLoad->SetVisible(true);
            break;
        case ConnectState::VerifyMap: setStatus(_("Checking Map...")); break;
        case ConnectState::QueryServerName: setStatus(_("Waiting for server info...")); break;
        case ConnectState::QueryPlayerList: setStatus(_("Waiting for Playerinfo...")); break;
        case ConnectState::QuerySettings: setStatus(_("Waiting for server info...")); break;
        case ConnectState::Finished:
            WINDOWMANAGER.Switch(std::make_unique<dskGameLobby>(serverType_, GAMECLIENT.GetGameLobby(),
                                                                GAMECLIENT.GetPlayerId(), std::move(lobbyClient_)));
            break;
    }
}

void iwConnecting::CI_MapPartReceived(uint32_t bytesReceived, uint32_t bytesTotal)
{
    downloadProgress_ = static_cast<unsigned short>(std::lround(bytesReceived * 100. / bytesTotal));
}

void iwConnecting::setStatus(const std::string& status)
{
    GetCtrl<ctrlText>(ID_txtStatus)->SetText(status);
}
