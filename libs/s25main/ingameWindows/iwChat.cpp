// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "rttrDefines.h" // IWYU pragma: keep
#include "iwChat.h"
#include "Loader.h"
#include "controls/ctrlEdit.h"
#include "controls/ctrlOptionGroup.h"
#include "network/GameClient.h"
#include "gameData/const_gui_ids.h"

unsigned char iwChat::chat_dest = CD_ALL;

iwChat::iwChat(Window* parent)
    : IngameWindow(CGI_CHAT, IngameWindow::posLastOrCenter, Extent(300, 150), _("Chat Window"), LOADER.GetImageN("resource", 41), false,
                   true, parent)
{
    // Eingabefeld für Chattext
    AddEdit(0, DrawPoint(20, 30), Extent(260, 22), TC_GREY, NormalFont);

    ctrlOptionGroup* group = AddOptionGroup(1, ctrlOptionGroup::CHECK);
    // "Alle"
    group->AddTextButton(CD_ALL, DrawPoint(20, 80), Extent(260, 22), TC_GREY, _("All"), NormalFont);
    // "Verbündete"
    group->AddTextButton(CD_ALLIES, DrawPoint(20, 112), Extent(125, 22), TC_GREEN2, _("Allies"), NormalFont);
    // "Feinde"
    group->AddTextButton(CD_ENEMIES, DrawPoint(155, 112), Extent(125, 22), TC_RED1, _("Enemies"), NormalFont);

    // Entspr. vom letzten Mal auswählen auswählen
    group->SetSelection(chat_dest);
}

void iwChat::Msg_PaintBefore()
{
    IngameWindow::Msg_PaintBefore();
    GetCtrl<ctrlEdit>(0)->SetFocus();
}

void iwChat::Msg_OptionGroupChange(const unsigned /*ctrl_id*/, unsigned selection)
{
    chat_dest = static_cast<unsigned char>(selection);
    GetCtrl<ctrlEdit>(0)->SetFocus();
}

void iwChat::Msg_EditEnter(const unsigned /*ctrl_id*/)
{
    Close();

    auto* edit = GetCtrl<ctrlEdit>(0);
    std::string text = edit->GetText();
    edit->SetText("");

    if(text.size() > 3u && text[0] == '!')
    {
        auto* listener = dynamic_cast<IChatCmdListener*>(GetParent());
        if(listener)
            listener->OnChatCommand(text.substr(1));
    } else
    {
        if(chat_dest != CD_ALL && chat_dest != CD_ALLIES && chat_dest != CD_ENEMIES)
            chat_dest = CD_ALL;
        GAMECLIENT.Command_Chat(text, ChatDestination(chat_dest));
    }
}
