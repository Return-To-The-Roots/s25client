// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef dskCREDITS_H_INCLUDED
#define dskCREDITS_H_INCLUDED

#pragma once

#include "Desktop.h"

#include <vector>
struct KeyEvent;
class glArchivItem_Bitmap;

/// Klasse des Credits Desktops.
class dskCredits : public Desktop
{
    public:
        dskCredits();
        ~dskCredits() override;

    private:
        bool Msg_KeyDown(const KeyEvent& ke) override;
        void Msg_PaintAfter() override;
        void Msg_ButtonClick(const unsigned ctrl_id) override;

        bool Close();

        static glArchivItem_Bitmap* GetCreditsImgOrDefault(const std::string& name);

        struct CreditsEntry
        {
            struct Line
            {
                Line(const char* const text): line(text), column(0) { }
                Line(const std::string& text): line(text), column(0) { }
                Line(const std::string& text, unsigned int c): line(text), column(0) { }
                std::string line;
                unsigned int column;
            };
            std::string title;
            std::string lastLine;
            glArchivItem_Bitmap* pic;
            std::vector<Line> lines;
            CreditsEntry(const std::string& title, const std::string& lastLine = ""): title(title), lastLine(lastLine), pic(NULL){}
            CreditsEntry(const std::string& title, glArchivItem_Bitmap* pic, const std::string& lastLine = ""): title(title), lastLine(lastLine), pic(pic){}
        };

        std::vector<CreditsEntry> entries;
        std::vector<dskCredits::CreditsEntry>::iterator itCurEntry;

        struct Bob
        {
            unsigned id;
            unsigned direction;
            unsigned animationStep;
            unsigned color;
            short x;
            short y;
            unsigned char speed;
            bool hasWare;
            bool isFat;
        };

        std::vector<Bob> bobs;

        unsigned int startTime;
        unsigned int bobTime;
        unsigned int bobSpawnTime;
};

#endif // !dskCREDITS_H_INCLUDED
