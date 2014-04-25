// $Id: Messenger.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "main.h"
#include "Messenger.h"

#include "Loader.h"
#include "VideoDriverWrapper.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/// Chat-Destination-String, der entsprechend angezeigt wird
const std::string CD_STRINGS[4] =
{
    "", "(All) ", "(Team) ", "(Enemies) "
};

/// Farbe für die einzelnen CDs
const unsigned CD_COLORS[4] =
{
    0, COLOR_WHITE, COLOR_GREEN, COLOR_RED
};

Messenger::~Messenger()
{
}

/// Zeit, die
void Messenger::Draw()
{
    unsigned y = 100;
    for(std::list<Messenger::Msg>::iterator it = messages.begin(); it != messages.end(); ++it, y += LargeFont->getHeight())
    {
        unsigned diff = VideoDriverWrapper::inst().GetTickCount() - it->starttime;
        if(diff > 20000)
        {
            messages.erase(it++);
            if(it != messages.end())
                continue;
            else
                break;

        }

        // Transparenz der Schrift ausrechnen, da sie am Ende ausgeblendet wird
        unsigned transparency = 0xFF;
        if(diff > 18000)
            transparency = (transparency - transparency * (diff - 18000) / 2000);


        // Auf Alphaposition verschieben (höchstes Byte)
        transparency = transparency << 24;

        std::string cd_str = (it->cd == CD_SYSTEM) ? "" : _(CD_STRINGS[it->cd]);


        LargeFont->Draw(20, y, it->author, 0, (it->color_author & 0x00FFFFFF) | transparency);
        LargeFont->Draw(20 + LargeFont->getWidth(it->author, static_cast<unsigned>(it->author.length())), y, cd_str, 0, (CD_COLORS[it->cd] & 0x00FFFFFF) | transparency);
        LargeFont->Draw(20 + LargeFont->getWidth(it->author, static_cast<unsigned>(it->author.length())) +
                        +LargeFont->getWidth(cd_str, static_cast<unsigned>(cd_str.length())), y,
                        it->msg, 0, (it->color_msg & 0x00FFFFFF) | transparency);
    }
}

void Messenger::AddMessage(const std::string& author, const unsigned color_author, const ChatDestination cd, const std::string& msg, const unsigned color_msg)
{
    LOG.lcprintf(color_author, "%s", author.c_str());
    LOG.lcprintf(CD_COLORS[cd], "%s", CD_STRINGS[cd].c_str());
    LOG.lprintf("%s\n", msg.c_str());

    glArchivItem_Font::WrapInfo wi;

    // in Zeilen aufteilen, damit alles auf den Bildschirm passt
    LargeFont->GetWrapInfo(msg.c_str(), VideoDriverWrapper::inst().GetScreenWidth() - 60
                           - LargeFont->getWidth(author.c_str()) - ((cd == CD_SYSTEM) ? 0 : LargeFont->getWidth(_(CD_STRINGS[cd]))),
                           VideoDriverWrapper::inst().GetScreenWidth() - 60, wi);

    // Message-Strings erzeugen aus den WrapInfo
    std::string* strings = new std::string[wi.positions.size()];

    wi.CreateSingleStrings(msg.c_str(), strings);

    for(unsigned i = 0; i < wi.positions.size(); ++i)
    {
        Messenger::Msg tmp;

        // Nur in erster Zeile den Autor und die ChatDest.!
        if(i == 0)
        {
            tmp.author = author;
            tmp.cd = cd;
        }
        else
            tmp.cd = CD_SYSTEM;

        tmp.msg = strings[i];

        tmp.width = LargeFont->getWidth(msg.c_str());
        if(i == 0)
            tmp.width += LargeFont->getWidth(author.c_str());

        tmp.color_author = color_author;
        tmp.color_msg = color_msg;
        tmp.starttime = VideoDriverWrapper::inst().GetTickCount();

        messages.push_back(tmp);
    }

    delete [] strings;
}
