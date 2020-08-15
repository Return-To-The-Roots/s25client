// Copyright (c) 2020 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef MockLocalGameState_h__
#define MockLocalGameState_h__

#include "ILocalGameState.h"
#include <turtle/mock.hpp>

MOCK_BASE_CLASS(MockLocalGameState, ILocalGameState)
{
    MOCK_CONST_METHOD(GetPlayerId, 0);
    MOCK_CONST_METHOD(IsHost, 0);
    MOCK_CONST_METHOD(FormatGFTime, 1);
    MOCK_NON_CONST_METHOD(SystemChat, 1);
};

#endif // MockLocalGameState_h__
