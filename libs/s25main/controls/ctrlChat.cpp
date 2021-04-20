// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlChat.h"
#include "CollisionDetection.h"
#include "FileChecksum.h"
#include "ctrlScrollBar.h"
#include "driver/MouseCoords.h"
#include "ogl/glFont.h"
#include "variant.h"
#include "s25util/Log.h"

/// Breite der Scrollbar
static const unsigned short SCROLLBAR_WIDTH = 20;

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
 */
ctrlChat::ctrlChat(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                   const glFont* font)
    : Window(parent, id, pos, size), tc(tc), font(font), time_color(0xFFFFFFFF)
{
    // Zeilen pro Seite festlegen errechnen
    page_size = (size.y - 4) / (font->getHeight() + 2);

    // Scrollbalken hinzufügen
    AddScrollBar(0, DrawPoint(size.x - SCROLLBAR_WIDTH, 0), Extent(SCROLLBAR_WIDTH, size.y), SCROLLBAR_WIDTH, tc,
                 page_size);

    // Breite der Klammern <> um die Spielernamen berechnen
    bracket1_size = font->getWidth("<");
    bracket2_size = font->getWidth("> ");
}

ctrlChat::~ctrlChat() = default;

/**
 *  Größe ändern
 */
void ctrlChat::Resize(const Extent& newSize)
{
    const bool x_changed = (GetSize().x != newSize.x && !chat_lines.empty());
    Window::Resize(newSize);

    auto* scroll = GetCtrl<ctrlScrollBar>(0);
    scroll->SetPos(DrawPoint(newSize.x - SCROLLBAR_WIDTH, 0));
    scroll->Resize(Extent(SCROLLBAR_WIDTH, newSize.y));

    // Remember some things
    const bool was_on_bottom = (scroll->GetScrollPos() + page_size == chat_lines.size());
    unsigned short position = 0;
    // Remember the entry on top
    for(unsigned short i = 1; i <= scroll->GetScrollPos(); ++i)
        if(holds_alternative<PrimaryChatLine>(chat_lines[i]))
            ++position;

    // Rewrap
    if(x_changed)
    {
        chat_lines.clear();
        for(unsigned short i = 0; i < raw_chat_lines.size(); ++i)
            WrapLine(i);
    }

    // Zeilen pro Seite festlegen errechnen
    page_size = (newSize.y - 4) / (font->getHeight() + 2);

    scroll->SetPageSize(page_size);

    // If we are were on the last line, keep
    if(was_on_bottom)
    {
        scroll->SetScrollPos(((chat_lines.size() > page_size) ? chat_lines.size() - page_size : 0));
    } else if(x_changed)
    {
        unsigned short i;
        for(i = 0; position > 0; ++i)
            if(holds_alternative<PrimaryChatLine>(chat_lines[i]))
                --position;
        scroll->SetScrollPos(i);
    }

    // Don't display empty lines at the end if there are this is
    // not necessary because of a lack of lines in total
    if(chat_lines.size() < page_size)
        scroll->SetScrollPos(0);
    else if(scroll->GetScrollPos() + page_size > chat_lines.size())
        scroll->SetScrollPos(chat_lines.size() - page_size);
}

/**
 *  Zeichnet das Chat-Control.
 */
void ctrlChat::Draw_()
{
    // Box malen
    Draw3D(Rect(GetDrawPos(), GetSize()), tc, false);

    Window::Draw_();

    // Wieviele Linien anzeigen?
    unsigned show_lines = (page_size > unsigned(chat_lines.size()) ? unsigned(chat_lines.size()) : page_size);

    // Listeneinträge zeichnen
    // Add margin
    DrawPoint textPos = GetDrawPos() + DrawPoint(2, 2);
    unsigned pos = GetCtrl<ctrlScrollBar>(0)->GetScrollPos();
    for(unsigned i = 0; i < show_lines; ++i)
    {
        DrawPoint curTextPos = textPos;
        if(PrimaryChatLine* line = boost::get<PrimaryChatLine>(&chat_lines[i + pos]))
        {
            // Zeit, Spieler und danach Textnachricht
            if(!line->time_string.empty())
            {
                font->Draw(curTextPos, line->time_string, FontStyle{}, time_color);
                curTextPos.x += font->getWidth(line->time_string);
            }

            if(!line->player.empty())
            {
                // Klammer 1 (<)
                font->Draw(curTextPos, "<", FontStyle{}, line->player_color);
                curTextPos.x += bracket1_size;
                // Spielername
                font->Draw(curTextPos, line->player, FontStyle{}, line->player_color);
                curTextPos.x += font->getWidth(line->player);
                // Klammer 2 (>)
                font->Draw(curTextPos, "> ", FontStyle{}, line->player_color);
                curTextPos.x += bracket2_size;
            }
        }
        boost::apply_visitor(
          [this, curTextPos](const auto& line) { // Draw msg
              this->font->Draw(curTextPos, line.msg, FontStyle{}, line.msg_color);
          },
          chat_lines[i + pos]);
        textPos.y += font->getHeight() + 2;
    }
}

void ctrlChat::WrapLine(unsigned short i)
{
    const RawChatLine& line = raw_chat_lines[i];

    // Breite von Zeitstring und Spielername berechnen (falls vorhanden)
    unsigned short prefix_width =
      (line.time_string.length() ? font->getWidth(line.time_string) : 0)
      + (line.player.length() ? (bracket1_size + bracket2_size + font->getWidth(line.player)) : 0);

    // Reicht die Breite des Textfeldes noch nichtmal dafür aus?
    if(prefix_width > GetSize().x - 2 - SCROLLBAR_WIDTH)
    {
        // dann können wir das gleich vergessen
        return;
    }

    // Zeilen ggf. wrappen, falls der Platz nich reicht und die Zeilenanfanänge in wi speichern
    glFont::WrapInfo wi =
      font->GetWrapInfo(line.msg, GetSize().x - prefix_width - 2 - SCROLLBAR_WIDTH, GetSize().x - 2 - SCROLLBAR_WIDTH);

    // Message-Strings erzeugen aus den WrapInfo
    std::vector<std::string> strings = wi.CreateSingleStrings(line.msg);

    // Zeilen hinzufügen
    for(unsigned i = 0; i < strings.size(); ++i)
    {
        if(i == 0)
        {
            PrimaryChatLine wrap_line = line;
            wrap_line.msg = strings[i];
            chat_lines.emplace_back(std::move(wrap_line));
        } else
        {
            chat_lines.emplace_back(SecondaryChatLine{strings[i], line.msg_color});
        }
    }
}

void ctrlChat::AddMessage(const std::string& time_string, const std::string& player, const unsigned player_color,
                          const std::string& msg, const unsigned msg_color)
{
    RawChatLine line;
    line.time_string = time_string;
    line.player = player;
    line.player_color = player_color;
    line.msg = msg;
    line.msg_color = msg_color;
    raw_chat_lines.emplace_back(std::move(line));

    const size_t oldlength = chat_lines.size();

    // Loggen
    LOG.write("%s <") % time_string;
    LOG.writeColored("%1%", player_color) % player;
    LOG.write(">: ");
    LOG.writeColored("%1%", msg_color) % msg;
    LOG.write("\n");

    // Umbrechen
    WrapLine(raw_chat_lines.size() - 1);

    // Scrollbar Bescheid sagen
    auto* scrollbar = GetCtrl<ctrlScrollBar>(0);
    scrollbar->SetRange(unsigned(chat_lines.size()));

    // Waren wir am Ende? Dann mit runterscrollen
    if(scrollbar->GetScrollPos() + page_size == oldlength)
        scrollbar->SetScrollPos(chat_lines.size() - page_size);
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
    if(IsPointInRect(mc.GetPos(), Rect(GetDrawPos() + DrawPoint(2, 2), GetSize() - Extent(2, 4))))
    {
        auto* scrollbar = GetCtrl<ctrlScrollBar>(0);
        scrollbar->Scroll(-3);
        return true;
    } else
        return false;
}

bool ctrlChat::Msg_WheelDown(const MouseCoords& mc)
{
    if(IsPointInRect(mc.GetPos(), Rect(GetDrawPos() + DrawPoint(2, 2), GetSize() - Extent(2, 4))))
    {
        auto* scrollbar = GetCtrl<ctrlScrollBar>(0);
        scrollbar->Scroll(+3);
        return true;
    } else
        return false;
}

unsigned ctrlChat::CalcUniqueColor(const std::string& name)
{
    unsigned checksum = CalcChecksumOfBuffer(name.c_str(), name.length()) * name.length();
    unsigned color = checksum | (checksum << 12) | 0xff000000;
    return color;
}
