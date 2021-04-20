// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Window.h"
#include <boost/variant.hpp>
#include <vector>
class MouseCoords;
class glFont;

/// ChatCtrl-Klasse für ein ChatCtrl.
class ctrlChat : public Window
{
public:
    ctrlChat(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
             const glFont* font);
    ~ctrlChat() override;

    /// Größe ändern
    void Resize(const Extent& newSize) override;
    /// Fügt eine Chatnachricht hinzu.
    void AddMessage(const std::string& time_string, const std::string& player, unsigned player_color,
                    const std::string& msg, unsigned msg_color);
    /// Setzt Farbe der Zeitangaben.
    void SetTimeColor(unsigned color) { time_color = color; }

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
    struct RawChatLine
    {
        /// Zeitangabe (optional)
        std::string time_string;
        /// Spielername (optional)
        std::string player;
        /// Farbe des Spieler(namens) (optional)
        unsigned player_color;
        /// Chatnachricht
        std::string msg;
        /// Farbe der Chatnachricht
        unsigned msg_color;
    };

    using PrimaryChatLine = RawChatLine;
    struct SecondaryChatLine
    {
        std::string msg;
        /// Farbe der Chatnachricht
        unsigned msg_color;
    };
    using ChatLine = boost::variant<PrimaryChatLine, SecondaryChatLine>;

    TextureColor tc;    /// Hintergrundtextur.
    const glFont* font; /// Schriftart.

    std::vector<RawChatLine> raw_chat_lines; /// Chatzeilen, noch nicht umgebrochen
    std::vector<ChatLine> chat_lines;        /// Chatzeilen

    unsigned page_size;  /// Chatzeilen pro Seite
    unsigned time_color; /// Farbe der Zeitangaben

    unsigned short bracket1_size; /// Breite der Klammer "<" um den Spielernamen
    unsigned short bracket2_size; /// Breite der Klammer ">" um den Spielernamen
};
