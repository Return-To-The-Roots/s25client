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
#ifndef iwDIRECTIPCREATE_H_INCLUDED
#define iwDIRECTIPCREATE_H_INCLUDED

#pragma once

#include "IngameWindow.h"
#include "gameTypes/ServerType.h"

class iwDirectIPCreate : public IngameWindow
{
public:
    iwDirectIPCreate(ServerType server_type);

    void LC_Status_Error(const std::string& error);

protected:
    void Msg_EditChange(const unsigned ctrl_id) override;
    void Msg_EditEnter(const unsigned ctrl_id) override;
    void Msg_ButtonClick(const unsigned ctrl_id) override;
    void Msg_OptionGroupChange(const unsigned ctrl_id, const int selection) override;

private:
    void SetText(const std::string& text, unsigned color, bool button);

private:
    ServerType server_type;
};

#endif // !iwDIRECTIPCREATE_H_INCLUDED
