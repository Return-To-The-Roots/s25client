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

#include "TransmittingSettingsWindow.h"
#include "WindowManager.h"
#include "iwMsgbox.h"
#include "network/GameClient.h"
#include <mygettext/mygettext.h>

TransmittingSettingsWindow::TransmittingSettingsWindow(unsigned id, const DrawPoint& pos, const Extent& size,
                                                       std::string title, glArchivItem_Bitmap* background, bool modal,
                                                       bool closeOnRightClick, Window* parent)
    : IngameWindow(id, pos, size, title, background, modal, closeOnRightClick, parent), settings_changed(false)
{
    // Timer for transmitting changes every 2 seconds
    using namespace std::chrono_literals;
    AddTimer(1001, 2s);
}

void TransmittingSettingsWindow::Close()
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
                                                      this, MsgboxButton::OkCancel, MsgboxIcon::ExclamationRed, 1000));
    }
}

void TransmittingSettingsWindow::Msg_MsgBoxResult(const unsigned msgbox_id, const MsgboxResult mbr)
{
    switch(msgbox_id)
    {
        case 1000: // Close?
        {
            if(mbr == MsgboxResult::Ok)
            {
                IngameWindow::Close();
            }
        }
        break;
    }
}

void TransmittingSettingsWindow::Msg_Timer(const unsigned /*ctrl_id*/)
{
    if(GAMECLIENT.IsReplayModeOn())
        UpdateSettings();
    else
        TransmitSettings();
}
