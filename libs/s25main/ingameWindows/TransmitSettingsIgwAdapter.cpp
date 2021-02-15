// Copyright (c) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

#include "TransmitSettingsIgwAdapter.h"

#include "WindowManager.h"
#include "iwMsgbox.h"
#include "network/GameClient.h"
#include <mygettext/mygettext.h>

constexpr unsigned TransmitSettingsIgwAdapter::firstCtrlID;

TransmitSettingsIgwAdapter::TransmitSettingsIgwAdapter(unsigned id, const DrawPoint& pos, const Extent& size,
                                                       const std::string& title, glArchivItem_Bitmap* background,
                                                       bool modal, bool closeOnRightClick, Window* parent)
    : IngameWindow(id, pos, size, title, background, modal, closeOnRightClick, parent), settings_changed(false)
{
    // Timer for transmitting changes every 2 seconds
    using namespace std::chrono_literals;
    AddTimer(firstCtrlID + 1u, 2s);
}

void TransmitSettingsIgwAdapter::Close()
{
    TransmitSettings();
    if(!settings_changed)
    {
        IngameWindow::Close();
    } else
    {
        WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(GetTitle(),
                                                      _("The changes could not be applied and will be discarded. A "
                                                        "potential reason for this is that the game is paused."),
                                                      this, MsgboxButton::OkCancel, MsgboxIcon::ExclamationRed,
                                                      firstCtrlID));
    }
}

void TransmitSettingsIgwAdapter::Msg_MsgBoxResult(unsigned msgbox_id, MsgboxResult mbr)
{
    if(msgbox_id == firstCtrlID && mbr == MsgboxResult::Ok)
        IngameWindow::Close();
}

void TransmitSettingsIgwAdapter::Msg_Timer(const unsigned /*ctrl_id*/)
{
    if(GAMECLIENT.IsReplayModeOn())
        UpdateSettings();
    else
        TransmitSettings();
}
