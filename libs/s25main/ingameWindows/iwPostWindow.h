// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"
#include "postSystem/PostCategory.h"
#include <vector>

class GameWorldView;
class PostBox;
class PostMsg;
struct KeyEvent;

class iwPostWindow : public IngameWindow
{
public:
    iwPostWindow(GameWorldView& gwv, PostBox& postBox);
    ~iwPostWindow();
    void Msg_PaintBefore() override;
    void Msg_ButtonClick(unsigned ctrl_id) override;
    bool Msg_KeyDown(const KeyEvent& ke) override;

private:
    GameWorldView& gwv;
    PostBox& postBox;
    bool showAll;
    PostCategory curCategory;
    std::vector<unsigned> curMsgIdxs;
    unsigned curMsgId;
    unsigned lastMsgCt;
    const PostMsg* curMsg;
    bool lastHasMissionGoal;

    /// Passt Steuerelemente an, setzt Einstellung für diverse Controls passend für die aktuelle PostMessage
    void DisplayPostMessage();
    /// Setzt den Text mehrzeilig in das Postfenster
    void SetMessageText(const std::string& message);
    void FilterMessages();
    bool ValidateMessages();
    const PostMsg* GetMsg(unsigned id) const;
    void SwitchCategory(PostCategory cat);
};
