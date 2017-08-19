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

#ifndef iwAIDEBUG_H_INCLUDED
#define iwAIDEBUG_H_INCLUDED

#include "IngameWindow.h"
#include <vector>

class AIBase;
class ctrlText;
class GameWorldView;
namespace AIJH {
class AIPlayerJH;
}

class iwAIDebug : public IngameWindow
{
public:
    iwAIDebug(GameWorldView& gwv, const std::vector<AIBase*>& ais);

private:
    void Msg_ComboSelectItem(const unsigned ctrl_id, const int selection) override;
    // void Msg_ButtonClick(const unsigned ctrl_id);
    // void Msg_ProgressChange(const unsigned ctrl_id, const unsigned short position);
    void Msg_PaintBefore() override;

private:
    GameWorldView& gwv;
    std::vector<AIJH::AIPlayerJH*> ais_;
    unsigned player_;
    unsigned overlay_;
    ctrlText* text;
};

#endif
