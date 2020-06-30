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
#ifndef dskCREDITS_H_INCLUDED
#define dskCREDITS_H_INCLUDED

#pragma once

#include "Desktop.h"
#include "libsiedler2/ImgDir.h"
#include <utility>
#include <vector>

struct KeyEvent;
class glArchivItem_Bitmap;

/// Klasse des Credits Desktops.
class dskCredits : public Desktop
{
public:
    dskCredits();
    ~dskCredits() override;

    bool Msg_KeyDown(const KeyEvent& ke) override;
    void Draw_() override;
    void Msg_ButtonClick(unsigned ctrl_id) override;
    void SetActive(bool active) override;
    static bool Close();

private:
    void DrawCredit();
    void DrawBobs();
    static glArchivItem_Bitmap* GetCreditsImgOrDefault(const std::string& name);

    struct CreditsEntry
    {
        struct Line
        {
            Line(const char* text, unsigned c = 0) : line(text), column(c) {}
            Line(std::string text, unsigned c = 0) : line(std::move(text)), column(c) {}
            std::string line;
            unsigned column;
        };
        std::string title;
        std::string lastLine;
        glArchivItem_Bitmap* pic;
        std::vector<Line> lines;
        explicit CreditsEntry(std::string title, std::string lastLine = "")
            : title(std::move(title)), lastLine(std::move(lastLine)), pic(nullptr)
        {}
        CreditsEntry(std::string title, glArchivItem_Bitmap* pic, std::string lastLine = "")
            : title(std::move(title)), lastLine(std::move(lastLine)), pic(pic)
        {}
    };

    std::vector<CreditsEntry> entries;
    std::vector<dskCredits::CreditsEntry>::iterator itCurEntry;

    struct Bob
    {
        unsigned id;
        libsiedler2::ImgDir direction;
        unsigned animationStep;
        unsigned color;
        DrawPoint pos;
        unsigned char speed;
        bool hasWare;
        bool isFat;
    };

    std::vector<Bob> bobs;

    unsigned startTime;
    unsigned bobTime;
    unsigned bobSpawnTime;
};

#endif // !dskCREDITS_H_INCLUDED
