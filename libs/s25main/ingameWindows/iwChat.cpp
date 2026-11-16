// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwChat.h"
#include "Loader.h"
#include "controls/ctrlEdit.h"
#include "controls/ctrlOptionGroup.h"
#include "helpers/MaxEnumValue.h"
#include "network/GameClient.h"
#include "gameData/const_gui_ids.h"

namespace {
ChatDestination lastChatDestination = ChatDestination::All;
}

iwChat::iwChat(Window* parent)
    : IngameWindow(CGI_CHAT, IngameWindow::posLastOrCenter, Extent(300, 150), _("Chat Window"),
                   LOADER.getImageN("resource", 41), false, CloseBehavior::Regular, parent)
{
    // Eingabefeld für Chattext
    AddEdit(0, DrawPoint(20, 30), Extent(260, 22), TextureColor::Grey, NormalFont);

    ctrlOptionGroup* group = AddOptionGroup(1, GroupSelectType::Check);
    // "Alle"
    group->AddTextButton(rttr::enum_cast(ChatDestination::All), DrawPoint(20, 80), Extent(260, 22), TextureColor::Grey,
                         _("All"), NormalFont);
    // "Verbündete"
    group->AddTextButton(rttr::enum_cast(ChatDestination::Allies), DrawPoint(20, 112), Extent(125, 22),
                         TextureColor::Green2, _("Allies"), NormalFont);
    // "Feinde"
    group->AddTextButton(rttr::enum_cast(ChatDestination::Enemies), DrawPoint(155, 112), Extent(125, 22),
                         TextureColor::Red1, _("Enemies"), NormalFont);

    // Entspr. vom letzten Mal auswählen auswählen
    group->setSelection(rttr::enum_cast(lastChatDestination));
}

void iwChat::msgPaintBefore()
{
    IngameWindow::msgPaintBefore();
    getCtrl<ctrlEdit>(0)->setFocus();
}

void iwChat::msgOptionGroupChange(const unsigned /*ctrl_id*/, unsigned selection)
{
    RTTR_Assert(selection <= helpers::MaxEnumValue_v<ChatDestination>);
    lastChatDestination = static_cast<ChatDestination>(selection);
    getCtrl<ctrlEdit>(0)->setFocus();
}

void iwChat::msgEditEnter(const unsigned /*ctrl_id*/)
{
    Close();

    auto* edit = getCtrl<ctrlEdit>(0);
    std::string text = edit->getText();
    edit->setText("");

    if(text.size() > 3u && text[0] == '!')
    {
        auto* listener = dynamic_cast<IChatCmdListener*>(getParent());
        if(listener)
            listener->OnChatCommand(text.substr(1));
    } else
        GAMECLIENT.commandChat(text, ChatDestination(lastChatDestination));
}
