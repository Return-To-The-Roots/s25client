// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Window.h"
class MouseCoords;
class glFont;

class ctrlCheck : public Window
{
public:
    ctrlCheck(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, std::string text,
              const glFont* font, bool readonly);

    void SetCheck(bool check) { this->check = check; }
    bool GetCheck() const { return check; }
    void SetReadOnly(bool readonly) { this->readonly = readonly; }
    bool GetReadOnly() const { return readonly; }

    bool Msg_LeftDown(const MouseCoords& mc) override;

protected:
    void Draw_() override;

private:
    TextureColor tc;
    std::string text;
    const glFont* font;
    bool check;
    bool readonly;
};
