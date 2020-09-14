// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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
