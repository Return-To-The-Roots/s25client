// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"
class ResourceId;
class Window;

class iwMsgbox : public IngameWindow
{
    /// Buttons, die auf der Box erscheinen sollen
    MsgboxButton button;
    /// ID für die Msgbox, um unterschiedliche
    unsigned msgboxid;

    /// Einzelne Stringzeilen, die durch die Umbrechung ggf. zu Stande kommen
    std::vector<std::string> strings;

    Window* msgHandler_;

public:
    iwMsgbox(const std::string& title, const std::string& text, Window* msgHandler, MsgboxButton button,
             MsgboxIcon icon, unsigned msgboxid = 0);
    iwMsgbox(const std::string& title, const std::string& text, Window* msgHandler, MsgboxButton button,
             const ResourceId& iconFile, unsigned iconIdx, unsigned msgboxid = 0);

    ~iwMsgbox() override;

    /// Moves the icon to given position
    void MoveIcon(const DrawPoint& pos);

private:
    void Init(const std::string& text, const ResourceId& iconFile, unsigned iconIdx);

    void AddButton(unsigned short id, int x, const std::string& text, TextureColor tc);

    void Msg_ButtonClick(unsigned ctrl_id) override;
};
