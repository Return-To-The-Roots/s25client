// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h" // IWYU pragma: keep
#include "ctrlChat.h"
#include "ctrlScrollBar.h"
#include "ogl/glArchivItem_Font.h"
#include "CollisionDetection.h"
#include "Log.h"
#include "driver/src/MouseCoords.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

/// Breite der Scrollbar
static const unsigned short SCROLLBAR_WIDTH = 20;

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p ctrlChat.
 *
 *  @param[in] parent Elternfenster
 *  @param[in] id     ID des Steuerelements
 *  @param[in] x      X-Position
 *  @param[in] y      Y-Position
 *  @param[in] width  Breite des Controls
 *  @param[in] height Höhe des Controls
 *  @param[in] tc     Hintergrundtextur
 *  @param[in] font   Schriftart
 *
 *  @author OLiver
 */
ctrlChat::ctrlChat(Window* parent,
                   unsigned int id,
                   unsigned short x,
                   unsigned short y,
                   unsigned short width,
                   unsigned short height,
                   TextureColor tc,
                   glArchivItem_Font* font)
    : Window(x, y, id, parent, width, height),
      tc(tc), font(font), time_color(0xFFFFFFFF)
{
    // Zeilen pro Seite festlegen errechnen
    page_size = (height - 4) / (font->getHeight() + 2);

    // Scrollbalken hinzufügen
    AddScrollBar(0, width - SCROLLBAR_WIDTH, 0, SCROLLBAR_WIDTH, height, SCROLLBAR_WIDTH, tc, page_size);

    //// Erst einmal 128 Chatzeilen reservieren
    //ExtendMemory(128);

    // Breite der Klammern <> um die Spielernamen berechnen
    bracket1_size = font->getWidth("<");
    bracket2_size = font->getWidth("> ");
}

ctrlChat::~ctrlChat()
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Größe ändern
 *
 *  @author Divan
 */
void ctrlChat::Resize_(unsigned short width, unsigned short height)
{

    ctrlScrollBar* scroll = GetCtrl<ctrlScrollBar>(0);
    scroll->Move(width - SCROLLBAR_WIDTH, 0);
    scroll->Resize(SCROLLBAR_WIDTH, height);

    // Remember some things
    const bool was_on_bottom = (scroll->GetPos() + page_size == chat_lines.size());
    const bool width_changed = (this->width_ != width && !chat_lines.empty());
    unsigned short position = 0;
    // Remember the entry on top
    for(unsigned short i = 1; i <= scroll->GetPos(); ++i)
        if(!chat_lines[i].secondary)
            ++position;

    // Rewrap
    if(width_changed)
    {
        this->width_ = width;
        chat_lines.clear();
        for(unsigned short i = 0; i < raw_chat_lines.size(); ++i)
            WrapLine(i);
    }

    // Zeilen pro Seite festlegen errechnen
    page_size = (height - 4) / (font->getHeight() + 2);

    scroll->SetPageSize(page_size);

    // If we are were on the last line, keep
    if(was_on_bottom)
    {
        scroll->SetPos(((chat_lines.size() > page_size) ? chat_lines.size() - page_size : 0));
    }
    else if(width_changed)
    {
        unsigned short i;
        for(i = 0; position > 0; ++i)
            if(!chat_lines[i].secondary)
                --position;
        scroll->SetPos(i);
    }

    // Don't display empty lines at the end if there are this is
    // not necessary because of a lack of lines in total
    if(chat_lines.size() < page_size)
    {
        scroll->SetPos(0);
    }
    else if(scroll->GetPos() + page_size > chat_lines.size())
        scroll->SetPos(chat_lines.size() - page_size);

}
///////////////////////////////////////////////////////////////////////////////
/**
 *  Zeichnet das Chat-Control.
 *
 *  @author OLiver
 */
bool ctrlChat::Draw_()
{
    // Box malen
    Draw3D(GetX(), GetY(), width_, height_, tc, 2);

    // Scrolleiste zeichnen
    DrawControls();

    // Wieviele Linien anzeigen?
    unsigned show_lines = (page_size > unsigned(chat_lines.size()) ? unsigned(chat_lines.size()) : page_size);

    // Listeneinträge zeichnen
    unsigned int pos = GetCtrl<ctrlScrollBar>(0)->GetPos();
    for(unsigned int i = 0; i < show_lines; ++i)
    {
        // eine zweite oder n-nte Zeile?
        if(chat_lines[i + pos].secondary)
        {
            // dann etwas Platz lassen davor und den entsprechenden Text hinschreiben
            font->Draw(GetX() + 2, GetY() + 2 + i * (font->getHeight() + 2), chat_lines[i + pos].msg, 0, chat_lines[i + pos].msg_color);
        }
        else
        {
            // Breite von Zeitangabe und Spielername ausrechnen
            unsigned short time_width = (chat_lines[i + pos].time_string.length()) ? font->getWidth(chat_lines[i + pos].time_string) : 0;
            unsigned short player_width = (chat_lines[i + pos].player.length()) ? font->getWidth(chat_lines[i + pos].player) : 0;
            unsigned short x_position = GetX() + 2, y_position = GetY() + 2 + i * (font->getHeight() + 2);

            // Zeit, Spieler und danach Textnachricht
            if(time_width)
            {
                font->Draw(x_position, y_position, chat_lines[i + pos].time_string, 0, time_color);
                x_position +=  time_width;
            }

            if(player_width)
            {
                // Klammer 1 (<)
                font->Draw(x_position, y_position, "<", 0, chat_lines[i + pos].player_color);
                x_position += bracket1_size;
                // Spielername
                font->Draw(x_position, y_position, chat_lines[i + pos].player, 0, chat_lines[i + pos].player_color);
                x_position += player_width;
                // Klammer 2 (>)
                font->Draw(x_position, y_position, "> ", 0, chat_lines[i + pos].player_color);
                x_position += bracket2_size;
            }

            font->Draw(x_position, y_position, chat_lines[i + pos].msg, 0, chat_lines[i + pos].msg_color);

        }
    }

    return true;
}

void ctrlChat::WrapLine(unsigned short i)
{
    ChatLine line = raw_chat_lines[i];

    // Breite von Zeitstring und Spielername berechnen (falls vorhanden)
    unsigned short prefix_width = ( line.time_string.length() ? font->getWidth(line.time_string) : 0) + (line.player.length() ? (bracket1_size + bracket2_size + font->getWidth(line.player)) : 0 );

    // Reicht die Breite des Textfeldes noch nichtmal dafür aus?
    if(prefix_width > width_ - 2 - SCROLLBAR_WIDTH)
    {
        // dann können wir das gleich vergessen
        return;
    }

    // Zeilen ggf. wrappen, falls der Platz nich reicht und die Zeilenanfanänge in wi speichern
    glArchivItem_Font::WrapInfo wi = font->GetWrapInfo(line.msg, width_ - prefix_width - 2 - SCROLLBAR_WIDTH, width_ - 2 - SCROLLBAR_WIDTH);

    // Message-Strings erzeugen aus den WrapInfo
    std::vector<std::string> strings = wi.CreateSingleStrings(line.msg);

    // Zeilen hinzufügen
    for(unsigned int i = 0; i < strings.size(); ++i)
    {
        ChatLine wrap_line;
        // Nur bei den ersten Zeilen müssen ja Zeit und Spielername mit angegeben werden
        wrap_line.secondary = i != 0;
        if(!wrap_line.secondary)
        {
            wrap_line.time_string = line.time_string;
            wrap_line.player = line.player;
            wrap_line.player_color = line.player_color;
        }

        wrap_line.msg = strings[i];
        wrap_line.msg_color = line.msg_color;

        chat_lines.push_back(wrap_line);
    }

}

void ctrlChat::AddMessage(const std::string& time_string, const std::string& player, const unsigned int player_color, const std::string& msg, const unsigned int msg_color)
{
    ChatLine line;

    line.time_string = time_string;
    line.player = player;
    line.player_color = player_color;
    line.msg = msg;
    line.msg_color = msg_color;
    raw_chat_lines.push_back(line);

    const unsigned short oldlength = chat_lines.size();

    // Loggen
    LOG.lprintf("%s <", time_string.c_str());
    LOG.lcprintf(player_color, "%s", player.c_str());
    LOG.lprintf(">: ");
    LOG.lcprintf(msg_color, "%s", msg.c_str());
    LOG.lprintf("\n");

    // Umbrechen
    WrapLine(raw_chat_lines.size() - 1);

    // Scrollbar Bescheid sagen
    ctrlScrollBar* scrollbar = GetCtrl<ctrlScrollBar>(0);
    scrollbar->SetRange(unsigned(chat_lines.size()));

    // Waren wir am Ende? Dann mit runterscrollen
    if(scrollbar->GetPos() + page_size  == oldlength)
        scrollbar->SetPos(chat_lines.size() - page_size);
}

bool ctrlChat::Msg_MouseMove(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_MouseMove, mc);
}

bool ctrlChat::Msg_LeftDown(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_LeftDown, mc);
}

bool ctrlChat::Msg_LeftUp(const MouseCoords& mc)
{
    return RelayMouseMessage(&Window::Msg_LeftUp, mc);
}

bool ctrlChat::Msg_WheelUp(const MouseCoords& mc)
{
    // Forward to ScrollBar, if mouse over control
    ctrlScrollBar* scrollbar = GetCtrl<ctrlScrollBar>(0);

    if(Coll(mc.x, mc.y, GetX() + 2, GetY() + 2, width_ - 2, height_ - 4))
    {
        // Simulate three Button Clicks
        scrollbar->Msg_ButtonClick(0);
        scrollbar->Msg_ButtonClick(0);
        scrollbar->Msg_ButtonClick(0);
        return true;
    }
    else
        return false;
}


bool ctrlChat::Msg_WheelDown(const MouseCoords& mc)
{
    // Forward to ScrollBar, if mouse over control
    ctrlScrollBar* scrollbar = GetCtrl<ctrlScrollBar>(0);

    if(Coll(mc.x, mc.y, GetX() + 2, GetY() + 2, width_ - 2, height_ - 4))
    {
        // Simulate three Button Clicks
        scrollbar->Msg_ButtonClick(1);
        scrollbar->Msg_ButtonClick(1);
        scrollbar->Msg_ButtonClick(1);
        return true;
    }
    else
        return false;
}

