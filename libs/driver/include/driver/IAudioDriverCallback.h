// Copyright (c) 2005-2009 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/config.hpp>

class BOOST_SYMBOL_VISIBLE IAudioDriverCallback
{
public:
    virtual ~IAudioDriverCallback() = default;
    virtual void Msg_MusicFinished() = 0;
};
