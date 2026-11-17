// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Messenger.h"
#include "Loader.h"
#include "drivers/VideoDriverWrapper.h"
#include "helpers/EnumArray.h"
#include "helpers/format.hpp"
#include "mygettext/mygettext.h"
#include "ogl/FontStyle.h"
#include "ogl/glFont.h"
#include "s25util/Log.h"
#include <array>

/// Chat-Destination-String, der entsprechend angezeigt wird
const helpers::EnumArray<std::string, ChatDestination> CD_STRINGS = {{"", "(All) ", "(Team) ", "(Enemies) "}};

/// Farbe für die einzelnen CDs
const helpers::EnumArray<unsigned, ChatDestination> CD_COLORS = {{0, COLOR_WHITE, COLOR_GREEN, COLOR_RED}};

Messenger::~Messenger() = default;

/// Zeit, die
void Messenger::draw()
{
    const unsigned curTime = VIDEODRIVER.getTickCount();
    DrawPoint textPos(20, 100);
    for(auto it = messages.begin(); it != messages.end(); textPos.y += LargeFont->getHeight())
    {
        unsigned diff = curTime - it->starttime;
        if(diff > 20000)
        {
            it = messages.erase(it);
            continue;
        }

        // Transparenz der Schrift ausrechnen, da sie am Ende ausgeblendet wird
        unsigned transparency = 0xFF;
        if(diff > 18000)
            transparency = (transparency - transparency * (diff - 18000) / 2000);

        std::string cdStr = (it->cd == ChatDestination::System) ? "" : _(CD_STRINGS[it->cd]);

        DrawPoint curTextPos(textPos);
        LargeFont->draw(curTextPos, it->author, FontStyle::LEFT, setAlpha(it->color_author, transparency));
        curTextPos.x += LargeFont->getWidth(it->author);
        LargeFont->draw(curTextPos, cdStr, FontStyle::LEFT, setAlpha(CD_COLORS[it->cd], transparency));
        curTextPos.x += LargeFont->getWidth(cdStr);
        LargeFont->draw(curTextPos, it->msg, FontStyle::LEFT, setAlpha(it->color_msg, transparency));
        ++it;
    }
}

void Messenger::addMessage(const std::string& author, const unsigned color_author, const ChatDestination cd,
                           const std::string& msg, const unsigned color_msg)
{
    if(!author.empty())
        LOG.writeColored("%1% ", color_author) % author;
    LOG.writeColored("%1%", CD_COLORS[cd]) % CD_STRINGS[cd];
    LOG.write("%1%\n") % msg;

    // in Zeilen aufteilen, damit alles auf den Bildschirm passt
    glFont::WrapInfo wi =
      LargeFont->getWrapInfo(msg,
                             VIDEODRIVER.getRenderSize().x - 60 - LargeFont->getWidth(author)
                               - ((cd == ChatDestination::System) ? 0 : LargeFont->getWidth(_(CD_STRINGS[cd]))),
                             VIDEODRIVER.getRenderSize().x - 60);

    // Message-Strings erzeugen aus den WrapInfo
    std::vector<std::string> strings = wi.createSingleStrings(msg);

    for(unsigned i = 0; i < strings.size(); ++i)
    {
        Messenger::Msg tmp;

        // Nur in erster Zeile den Autor und die ChatDest.!
        if(i == 0)
        {
            if(!author.empty())
                tmp.author = helpers::format(_("<%s> "), author);
            tmp.cd = cd;
        } else
            tmp.cd = ChatDestination::System;

        tmp.msg = strings[i];

        tmp.width = LargeFont->getWidth(msg);
        if(i == 0)
            tmp.width += LargeFont->getWidth(author);

        tmp.color_author = color_author;
        tmp.color_msg = color_msg;
        tmp.starttime = VIDEODRIVER.getTickCount();

        messages.push_back(tmp);
    }
}
