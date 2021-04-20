// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Window.h"
#include "controls/ctrlBaseTooltip.h"
class MouseCoords;

class ctrlProgress : public Window, public ctrlBaseTooltip
{
public:
    ctrlProgress(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                 unsigned short button_minus, unsigned short button_plus, unsigned short maximum, const Extent& padding,
                 unsigned force_color, const std::string& tooltip, const std::string& button_minus_tooltip = "",
                 const std::string& button_plus_tooltip = "", unsigned short* write_val = nullptr);

    void Resize(const Extent& newSize) override;
    void SetPosition(unsigned short position);
    const unsigned short& GetPosition() const { return position; }

    void Msg_ButtonClick(unsigned ctrl_id) override;
    bool Msg_LeftDown(const MouseCoords& mc) override;
    bool Msg_LeftUp(const MouseCoords& mc) override;
    bool Msg_WheelUp(const MouseCoords& mc) override;
    bool Msg_WheelDown(const MouseCoords& mc) override;
    bool Msg_MouseMove(const MouseCoords& mc) override;

protected:
    void Draw_() override;

private:
    TextureColor tc;

    unsigned short position;
    unsigned short maximum;

    // Abstand vom Button zur Leiste (Leiste wird entsprechend verkleinert!)
    Extent padding_;

    /// Falls der Balken immer eine bestimmte Farben haben soll, ansonsten 0 setzen!
    unsigned force_color;

    unsigned CalcBarWidth() const;
};
