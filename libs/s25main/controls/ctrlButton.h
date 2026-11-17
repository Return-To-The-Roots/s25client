// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "DrawPoint.h"
#include "Window.h"
#include "ctrlBaseTooltip.h"

#include <string>

struct MouseCoords;

/// Buttonklasse
class ctrlButton : public Window, public ctrlBaseTooltip
{
public:
    ctrlButton(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
               const std::string& tooltip);
    ~ctrlButton() override;

    void setEnabled(bool enable = true);
    bool getEnabled() const { return isEnabled; }
    TextureColor getTexture() const { return tc; }
    void setTexture(TextureColor tc) { this->tc = tc; }
    void setActive(bool activate = true) override;

    void setChecked(bool checked) { this->isChecked = checked; }
    bool getCheck() const { return isChecked; }
    void setIlluminated(bool illuminated) { this->isIlluminated = illuminated; }
    bool getIlluminated() const { return isIlluminated; }
    void setBorder(bool hasBorder) { this->hasBorder = hasBorder; }

    bool msgMouseMove(const MouseCoords& mc) override;
    bool msgLeftDown(const MouseCoords& mc) override;
    bool msgLeftUp(const MouseCoords& mc) override;

    ButtonState getState() const { return state; }

protected:
    /// Zeichnet Grundstruktur des Buttons
    void drawImpl() override;
    /// Abgeleitete Klassen müssen erweiterten Button-Inhalt zeichnen
    virtual void drawContent() const = 0;

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
