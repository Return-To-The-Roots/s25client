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
#ifndef CTRLCHAT_H_INCLUDED
#define CTRLCHAT_H_INCLUDED

#pragma once

#include "Window.h"
#include <vector>
class MouseCoords;
class glArchivItem_Font;

/// ChatCtrl-Klasse für ein ChatCtrl.
class ctrlChat : public Window
{
public:
    ctrlChat(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc, glArchivItem_Font* font);
    ~ctrlChat() override;

    /// Größe ändern
    void Resize(const Extent& newSize) override;
    /// Fügt eine Chatnachricht hinzu.
    void AddMessage(const std::string& time_string, const std::string& player, const unsigned player_color, const std::string& msg,
                    unsigned msg_color);
    /// Setzt Farbe der Zeitangaben.
    void SetTimeColor(const unsigned color) { time_color = color; }

    bool Msg_MouseMove(const MouseCoords& mc) override;
    bool Msg_LeftDown(const MouseCoords& mc) override;
    bool Msg_LeftUp(const MouseCoords& mc) override;
    bool Msg_WheelUp(const MouseCoords& mc) override;
    bool Msg_WheelDown(const MouseCoords& mc) override;

    /// Create a (almost) unique color for the given name
    static unsigned CalcUniqueColor(const std::string& name);

protected:
    /// Zeichnet das Chat-Control.
    void Draw_() override;
    /// Converts an unwrapped line into a wrapped one and appends it
    void WrapLine(unsigned short i);

private:
    // Struktur für eine Chatzeile.
    struct ChatLine
    {
        /// Handelt es sich bei dieser Zeile um eine sekundäre (also den >1-Teil einer umbrochenen Zeile)
        bool secondary;
        /// Zeitangabe (optional, 0 wenn nicht benutzt)
        std::string time_string;
        /// Spielername
        std::string player;
        /// Farbe des Spieler(namens) (optional, 0 wenn nicht benutzt)
        unsigned player_color;
        /// Chatnachricht
        std::string msg;
        /// Farbe der Chatnachricht
        unsigned msg_color;
    };

private:
    TextureColor tc;         /// Hintergrundtextur.
    glArchivItem_Font* font; /// Schriftart.

    std::vector<ChatLine> raw_chat_lines; /// Chatzeilen, noch nicht umgebrochen
    std::vector<ChatLine> chat_lines;     /// Chatzeilen

    unsigned page_size;  /// Chatzeilen pro Seite
    unsigned time_color; /// Farbe der Zeitangaben

    unsigned short bracket1_size; /// Breite der Klammer "<" um den Spielernamen
    unsigned short bracket2_size; /// Breite der Klammer ">" um den Spielernamen
};

#endif // !CTRLCHAT_H_INCLUDED
