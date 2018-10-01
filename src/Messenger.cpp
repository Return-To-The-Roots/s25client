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

#include "rttrDefines.h" // IWYU pragma: keep
#include "Messenger.h"
#include "Loader.h"
#include "drivers/VideoDriverWrapper.h"
#include "mygettext/mygettext.h"
#include "ogl/FontStyle.h"
#include "ogl/glArchivItem_Font.h"
#include "libutil/Log.h"

/// Chat-Destination-String, der entsprechend angezeigt wird
const std::string CD_STRINGS[4] = {"", "(All) ", "(Team) ", "(Enemies) "};

/// Farbe für die einzelnen CDs
const unsigned CD_COLORS[4] = {0, COLOR_WHITE, COLOR_GREEN, COLOR_RED};

Messenger::~Messenger() {}

/// Zeit, die
void Messenger::Draw()
{
    const unsigned curTime = VIDEODRIVER.GetTickCount();
    DrawPoint textPos(20, 100);
    for(std::list<Messenger::Msg>::iterator it = messages.begin(); it != messages.end(); textPos.y += LargeFont->getHeight())
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

        std::string cd_str = (it->cd == CD_SYSTEM) ? "" : _(CD_STRINGS[it->cd]);

        DrawPoint curTextPos(textPos);
        LargeFont->Draw(curTextPos, it->author, FontStyle::LEFT, SetAlpha(it->color_author, transparency));
        curTextPos.x += LargeFont->getWidth(it->author);
        LargeFont->Draw(curTextPos, cd_str, FontStyle::LEFT, SetAlpha(CD_COLORS[it->cd], transparency));
        curTextPos.x += LargeFont->getWidth(cd_str);
        LargeFont->Draw(curTextPos, it->msg, FontStyle::LEFT, SetAlpha(it->color_msg, transparency));
        ++it;
    }
}

void Messenger::AddMessage(const std::string& author, const unsigned color_author, const ChatDestination cd, const std::string& msg,
                           const unsigned color_msg)
{
    LOG.writeColored(author, color_author);
    LOG.writeColored(CD_STRINGS[cd], CD_COLORS[cd]);
    LOG.write(msg + "\n");

    // in Zeilen aufteilen, damit alles auf den Bildschirm passt
    glArchivItem_Font::WrapInfo wi = LargeFont->GetWrapInfo(msg,
                                                            VIDEODRIVER.GetScreenSize().x - 60 - LargeFont->getWidth(author)
                                                              - ((cd == CD_SYSTEM) ? 0 : LargeFont->getWidth(_(CD_STRINGS[cd]))),
                                                            VIDEODRIVER.GetScreenSize().x - 60);

    // Message-Strings erzeugen aus den WrapInfo
    std::vector<std::string> strings = wi.CreateSingleStrings(msg);

    for(unsigned i = 0; i < strings.size(); ++i)
    {
        Messenger::Msg tmp;

        // Nur in erster Zeile den Autor und die ChatDest.!
        if(i == 0)
        {
            tmp.author = author;
            tmp.cd = cd;
        } else
            tmp.cd = CD_SYSTEM;

        tmp.msg = strings[i];

        tmp.width = LargeFont->getWidth(msg);
        if(i == 0)
            tmp.width += LargeFont->getWidth(author);

        tmp.color_author = color_author;
        tmp.color_msg = color_msg;
        tmp.starttime = VIDEODRIVER.GetTickCount();

        messages.push_back(tmp);
    }
}
