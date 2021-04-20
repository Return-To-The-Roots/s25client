// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
