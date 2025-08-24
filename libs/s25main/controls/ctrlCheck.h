// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Window.h"
#include "ctrlBaseTooltip.h"
#include <string>
class MouseCoords;
class glFont;

class ctrlCheck : public Window, ctrlBaseTooltip
{
public:
    ctrlCheck(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, std::string text,
              const glFont* font, bool readonly);

    ctrlCheck* setChecked(bool checked)
    {
        this->check = checked;
        return this;
    }
    bool isChecked() const { return check; }
    ctrlCheck* setReadOnly(bool readonly)
    {
        this->readonly = readonly;
        return this;
    }
    bool isReadOnly() const { return readonly; }

    ctrlCheck* setTooltip(std::string tooltip);

    bool Msg_LeftDown(const MouseCoords& mc) override;
    bool Msg_MouseMove(const MouseCoords& mc) override;

protected:
    void Draw_() override;

private:
    TextureColor tc;
    std::string text;
    const glFont* font;
    bool check;
    bool readonly;
};
