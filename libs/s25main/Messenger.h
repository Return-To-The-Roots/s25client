// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/ChatDestination.h"
#include "s25util/colors.h"
#include <list>
#include <string>

class Messenger
{
    struct Msg
    {
        std::string author;
        unsigned color_author;
        ChatDestination cd;
        std::string msg;
        unsigned color_msg;
        unsigned starttime;
        unsigned short width;
    };

    std::list<Msg> messages;

public:
    ~Messenger();

    void Draw();
    void AddMessage(const std::string& author, unsigned color_author, ChatDestination cd, const std::string& msg,
                    unsigned color_msg = COLOR_YELLOW);
};
