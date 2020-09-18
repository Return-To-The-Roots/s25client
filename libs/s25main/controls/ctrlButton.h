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
    bool GetCheck() { return isChecked; }
    void SetIlluminated(bool illuminated) { this->isIlluminated = illuminated; }
    bool GetIlluminated() { return isIlluminated; }
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
