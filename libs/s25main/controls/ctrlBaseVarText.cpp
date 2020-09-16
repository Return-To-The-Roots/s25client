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

#include "ctrlBaseVarText.h"
#include "RTTR_Assert.h"
#include <sstream>

ctrlBaseVarText::ctrlBaseVarText(const std::string& fmtString, const unsigned color, const glFont* font, unsigned count,
                                 va_list fmtArgs)
    : ctrlBaseText(fmtString, color, font)
{
    for(unsigned i = 0; i < count; ++i)
        vars.push_back(va_arg(fmtArgs, void*));
}

std::string ctrlBaseVarText::GetFormatedText() const
{
    std::stringstream str;

    unsigned curVar = 0;
    bool isInFormat = false;

    for(char it : text)
    {
        if(isInFormat)
        {
            isInFormat = false;
            switch(it)
            {
                case 'd':
                    str << *reinterpret_cast<int*>(vars[curVar]); //-V206
                    curVar++;
                    break;
                case 'u':
                    str << *reinterpret_cast<unsigned*>(vars[curVar]); //-V206
                    curVar++;
                    break;
                case 's':
                    str << reinterpret_cast<const char*>(vars[curVar]);
                    curVar++;
                    break;
                case '%': str << '%'; break;
                default:
                    RTTR_Assert(false); // Invalid format string
                    str << '%' << it;
                    break;
            }
        } else if(it == '%')
            isInFormat = true;
        else
            str << it;
    }

    if(isInFormat)
    {
        RTTR_Assert(false); // Invalid format string
        str << '%';
    }

    return str.str();
}
