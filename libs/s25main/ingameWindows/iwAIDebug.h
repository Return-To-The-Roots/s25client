// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"
#include <vector>

class AIPlayer;
class ctrlMultiline;
class GameWorldView;
namespace AIJH {
class AIPlayerJH;
}

class iwAIDebug : public IngameWindow
{
public:
    iwAIDebug(GameWorldView& gwv, const std::vector<const AIPlayer*>& ais);
    ~iwAIDebug() override;

private:
    void Msg_ComboSelectItem(unsigned ctrl_id, unsigned selection) override;
    // void Msg_ButtonClick(unsigned ctrl_id);
    // void Msg_ProgressChange(unsigned ctrl_id, unsigned short position);
    void Msg_PaintBefore() override;

    class DebugPrinter;

    GameWorldView& gwv;
    std::vector<const AIJH::AIPlayerJH*> ais_;
    ctrlMultiline* text;
    DebugPrinter* printer;
};
