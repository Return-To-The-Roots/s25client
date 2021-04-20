// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

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
