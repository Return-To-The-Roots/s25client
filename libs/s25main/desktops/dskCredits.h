// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Desktop.h"
#include "Timer.h"
#include "libsiedler2/ImgDir.h"
#include <utility>
#include <vector>

struct KeyEvent;
class ctrlTimer;
class glArchivItem_Bitmap;

/// Klasse des Credits Desktops.
class dskCredits : public Desktop
{
public:
    dskCredits();
    ~dskCredits() override;

    bool Msg_LeftUp(const MouseCoords& mc) override;
    bool Msg_RightUp(const MouseCoords& mc) override;
    bool Msg_KeyDown(const KeyEvent& ke) override;
    void Msg_Timer(unsigned timerId) override;
    void Draw_() override;
    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_MsgBoxResult(unsigned msgbox_id, MsgboxResult) override;
    void SetActive(bool active) override;
    static bool Close();

private:
    void GotoNextPage();
    void GotoPrevPage();

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

    ctrlTimer* pageTimer;
    Timer bobSpawnTimer;
};
