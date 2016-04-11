// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h" // IWYU pragma: keep
#include "iwChat.h"
#include "Loader.h"
#include "GameClient.h"
#include "Random.h"
#include "controls/ctrlEdit.h"
#include "controls/ctrlOptionGroup.h"
#include "gameData/const_gui_ids.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

unsigned char iwChat::chat_dest = 0;

iwChat::iwChat()
    : IngameWindow(CGI_CHAT, 0xFFFF, 0xFFFF, 300, 150, _("Chat Window"), LOADER.GetImageN("resource", 41))
{
    // Eingabefeld für Chattext
    AddEdit(0, 20, 30, 260, 22, TC_GREY, NormalFont);

    ctrlOptionGroup* group = AddOptionGroup(1, ctrlOptionGroup::CHECK);
    // "Alle"
    group->AddTextButton(0,  20,  80, 260, 22, TC_GREY, _("All"), NormalFont);
    // "Verbündete"
    group->AddTextButton(1,  20, 112, 125, 22, TC_GREEN2, _("Allies"), NormalFont);
    // "Feinde"
    group->AddTextButton(2, 155, 112, 125, 22, TC_RED1, _("Enemies"), NormalFont);

    // Entspr. vom letzten Mal auswählen auswählen
    group->SetSelection(chat_dest);
}

void iwChat::Msg_PaintBefore()
{
    GetCtrl<ctrlEdit>(0)->SetFocus();
}

void iwChat::Msg_OptionGroupChange(const unsigned int  /*ctrl_id*/, const int selection)
{
    chat_dest = static_cast<unsigned char>(selection);
    GetCtrl<ctrlEdit>(0)->SetFocus();
}

void iwChat::Msg_EditEnter(const unsigned int  /*ctrl_id*/)
{
    Close();

    ctrlEdit* edit = GetCtrl<ctrlEdit>(0);

    if(chat_dest != 0 && chat_dest != 1 && chat_dest != 2)
        chat_dest = 0;

    if (edit->GetText() == "apocalypsis")
    {
        GAMECLIENT.CheatArmageddon();
        return;
    }
    else if (edit->GetText() == "surrender")
    {
        GAMECLIENT.Surrender();
        return;
    }
    else if (edit->GetText() == "async!")
    {
        (void) RANDOM.Rand(__FILE__, __LINE__, 0, 255);
        return;
    }
    else if (edit->GetText() == "segfault!")
    {
        char* x = NULL;

        *x = 1; //-V522 // NOLINT

        return;
    }

    GAMECLIENT.Command_Chat(edit->GetText(), ChatDestination(chat_dest + 1));

    edit->SetText("");
}

