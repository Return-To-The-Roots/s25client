// $Id: iwTextfile.h 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef iwMISSIONSTATEMENT_H_INCLUDED
#define iwMISSIONSTATEMENT_H_INCLUDED

#pragma once

#include "IngameWindow.h"

class iwMissionStatement : public IngameWindow
{
    public:
        iwMissionStatement(const std::string& title, const std::string& content);
        void Msg_ButtonClick(const unsigned int ctrl_id);
};

#endif // !iwMISSIONSTATEMENT_H_INCLUDED
