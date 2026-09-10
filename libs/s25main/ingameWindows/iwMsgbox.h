// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"
#include <string>
#include <vector>

class glArchivItem_Bitmap;
class ResourceId;
class Window;
struct KeyEvent;

struct MsgboxButtonConfig
{
    std::string text;
    MsgboxResult result;
    TextureColor color;
};

struct MsgboxConfig
{
    std::vector<MsgboxButtonConfig> buttons;
    unsigned defaultButton = 0;
    int cancelButton = -1;
    int focusedButton = -1;
};

class iwMsgbox : public IngameWindow
{
    /// ID für die Msgbox, um unterschiedliche
    unsigned msgboxid;

    std::vector<MsgboxButtonConfig> buttons_;
    unsigned defaultButton_;
    int cancelButton_;
    int focusedButton_;

    Window* msgHandler_;

public:
    iwMsgbox(const std::string& title, const std::string& text, Window* msgHandler, MsgboxButton button,
             MsgboxIcon icon, unsigned msgboxid = 0);
    iwMsgbox(const std::string& title, const std::string& text, Window* msgHandler, MsgboxButton button,
             const ResourceId& iconFile, unsigned iconIdx, unsigned msgboxid = 0);
    iwMsgbox(const std::string& title, const std::string& text, Window* msgHandler, MsgboxConfig config,
             unsigned msgboxid = 0);
    iwMsgbox(const std::string& title, const std::string& text, Window* msgHandler, MsgboxConfig config,
             MsgboxIcon icon, unsigned msgboxid = 0);
    iwMsgbox(const std::string& title, const std::string& text, Window* msgHandler, MsgboxConfig config,
             const ResourceId& iconFile, unsigned iconIdx, unsigned msgboxid = 0);

    ~iwMsgbox() override;

    /// Moves the icon to given position
    void MoveIcon(const DrawPoint& pos);

private:
    void Init(const std::string& text, glArchivItem_Bitmap* icon);

    void AddButton(unsigned short id, int x, const std::string& text, TextureColor tc);

    bool Msg_KeyDown(const KeyEvent& ke) override;
    void Msg_ButtonClick(unsigned ctrl_id) override;
};
