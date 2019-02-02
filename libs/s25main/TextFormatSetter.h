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

#pragma once

#ifndef TextFormatSetter_h__
#define TextFormatSetter_h__

#include "controls/ctrlBaseText.h"
#include <boost/format.hpp>

struct TextFormatSetter
{
    ctrlBaseText* ctrl;
    boost::format fmt;
    TextFormatSetter(ctrlBaseText* ctrl) : ctrl(ctrl), fmt(ctrl->GetText()) {}
    ~TextFormatSetter() { ctrl->SetText(fmt.str()); }
    template<typename T>
    TextFormatSetter& operator%(const T& val)
    {
        fmt % val;
        return *this;
    }
};

#endif // TextFormatSetter_h__
