// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "DrawPoint.h"
#include "Window.h"
#include "ctrlBaseTooltip.h"

#include <string>

class MouseCoords;

/// Buttonklasse
class ctrlButton : public Window, public ctrlBaseTooltip
{
public:
    ctrlButton(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
               const std::string& tooltip);
    ~ctrlButton() override;

    void SetEnabled(bool enable = true);
    bool GetEnabled() const { return isEnabled; }
    TextureColor GetTexture() const { return tc; }
    void SetTexture(TextureColor tc) { this->tc = tc; }
    void SetActive(bool activate = true) override;

    void SetChecked(bool checked) { this->isChecked = checked; }
    bool GetCheck() const { return isChecked; }
    void SetIlluminated(bool illuminated) { this->isIlluminated = illuminated; }
    bool GetIlluminated() const { return isIlluminated; }
    void SetBorder(bool hasBorder) { this->hasBorder = hasBorder; }

    bool Msg_MouseMove(const MouseCoords& mc) override;
    bool Msg_LeftDown(const MouseCoords& mc) override;
    bool Msg_LeftUp(const MouseCoords& mc) override;

protected:
    /// Zeichnet Grundstruktur des Buttons
    void Draw_() override;
    /// Abgeleitete Klassen müssen erweiterten Button-Inhalt zeichnen
    virtual void DrawContent() const = 0;
    bool IsMouseOver(const Position& mousePos) const;

    /// Texturfarbe des Buttons
    TextureColor tc;
    /// Status des Buttons (gedrückt, erhellt usw. durch Maus ausgelöst)
    ButtonState state;
    /// Hat der Button einen 3D-Rand?
    bool hasBorder;
    /// Button dauerhaft gedrückt?
    bool isChecked;
    /// Button "erleuchtet"?
    bool isIlluminated;
    /// Button angeschalten?
    bool isEnabled;
};
