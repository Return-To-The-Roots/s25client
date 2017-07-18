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
#ifndef CTRLBUTTON_H_INCLUDED
#define CTRLBUTTON_H_INCLUDED

#pragma once

#include "Window.h"
#include "Point.h"
#include "ctrlBaseColor.h"
#include "ctrlBaseText.h"
#include "ctrlBaseTooltip.h"
#include "ctrlBaseImage.h"

#include <string>

class MouseCoords;
class glArchivItem_Bitmap;
class glArchivItem_Font;

/// Buttonklasse
class ctrlButton : public Window, public ctrlBaseTooltip
{
    public:
        ctrlButton(Window* parent, unsigned int id, const DrawPoint& pos, const Extent& size,
                   const TextureColor tc, const std::string& tooltip);
        ~ctrlButton() override;

        void SetEnabled(bool enable = true);
        bool GetEnabled() const { return isEnabled; }
        TextureColor GetTexture() const { return tc; }
        void SetTexture(TextureColor tc) { this->tc = tc; }

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
        // Prüfen, ob bei gehighlighteten Button die Maus auch noch über dem Button ist
        void TestMouseOver();
        bool IsMouseOver(const Point<int>& mousePos) const;

    protected:

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

/// Button mit Text
class ctrlTextButton : public ctrlButton, public ctrlBaseText
{
    public:

        ctrlTextButton(Window* parent, unsigned int id, const DrawPoint& pos,
                       const Extent& size, const TextureColor tc,
                       const std::string& text,  glArchivItem_Font* font, const std::string& tooltip);

    protected:

        /// Abgeleitete Klassen müssen erweiterten Button-Inhalt zeichnen (Text in dem Fall)
        void DrawContent() const override;
};


/// Button mit einem Bild
class ctrlImageButton : public ctrlButton, public ctrlBaseImage
{
    public:

        ctrlImageButton(Window* parent, unsigned int id, const DrawPoint& pos,
                        const Extent& size, const TextureColor tc,
                        glArchivItem_Bitmap* const image, const std::string& tooltip);
    protected:

        void DrawContent() const override;
};

/// Button mit Farbe
class ctrlColorButton : public ctrlButton, public ctrlBaseColor
{
    public:

        ctrlColorButton(Window* parent, unsigned int id, const DrawPoint& pos,
                        const Extent& size, const TextureColor tc,
                        unsigned int fillColor, const std::string& tooltip);

    protected:

        void DrawContent() const override;
};



#endif // CTRLBUTTON_H_INCLUDED
