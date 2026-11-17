// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlChat.h"
#include "CollisionDetection.h"
#include "FileChecksum.h"
#include "ctrlScrollBar.h"
#include "driver/MouseCoords.h"
#include "ogl/glFont.h"
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
    addScrollBar(0, DrawPoint(size.x - SCROLLBAR_WIDTH, 0), Extent(SCROLLBAR_WIDTH, size.y), SCROLLBAR_WIDTH, tc,
                 page_size);

    // Breite der Klammern <> um die Spielernamen berechnen
    bracket1_size = font->getWidth("<");
    bracket2_size = font->getWidth("> ");
}

ctrlChat::~ctrlChat() = default;

/**
 *  Größe ändern
 */
void ctrlChat::resize(const Extent& newSize)
{
    const bool xChanged = (getSize().x != newSize.x && !chat_lines.empty());
    Window::resize(newSize);

    auto* scroll = getCtrl<ctrlScrollBar>(0);
    scroll->setPos(DrawPoint(newSize.x - SCROLLBAR_WIDTH, 0));
    scroll->resize(Extent(SCROLLBAR_WIDTH, newSize.y));

    // Remember some things
    const bool wasOnBottom = (scroll->getScrollPos() + page_size == chat_lines.size());
    unsigned short position = 0;
    // Remember the entry on top
    for(unsigned short i = 1; i <= scroll->getScrollPos(); ++i)
        if(holds_alternative<PrimaryChatLine>(chat_lines[i]))
            ++position;

    // Rewrap
    if(xChanged)
    {
        chat_lines.clear();
        for(unsigned short i = 0; i < raw_chat_lines.size(); ++i)
            wrapLine(i);
    }

    // Zeilen pro Seite festlegen errechnen
    page_size = (newSize.y - 4) / (font->getHeight() + 2);

    scroll->setPageSize(page_size);

    // If we are were on the last line, keep
    if(wasOnBottom)
    {
        scroll->setScrollPos(((chat_lines.size() > page_size) ? chat_lines.size() - page_size : 0));
    } else if(xChanged)
    {
        unsigned short i;
        for(i = 0; position > 0; ++i)
            if(holds_alternative<PrimaryChatLine>(chat_lines[i]))
                --position;
        scroll->setScrollPos(i);
    }

    // Don't display empty lines at the end if there are this is
    // not necessary because of a lack of lines in total
    if(chat_lines.size() < page_size)
        scroll->setScrollPos(0);
    else if(scroll->getScrollPos() + page_size > chat_lines.size())
        scroll->setScrollPos(chat_lines.size() - page_size);
}

/**
 *  Zeichnet das Chat-Control.
 */
void ctrlChat::drawImpl()
{
    // Box malen
    draw3D(Rect(getDrawPos(), getSize()), tc, false);

    Window::drawImpl();

    // Wieviele Linien anzeigen?
    unsigned showLines = (page_size > unsigned(chat_lines.size()) ? unsigned(chat_lines.size()) : page_size);

    // Listeneinträge zeichnen
    // Add margin
    DrawPoint textPos = getDrawPos() + DrawPoint(2, 2);
    unsigned pos = getCtrl<ctrlScrollBar>(0)->getScrollPos();
    for(unsigned i = 0; i < showLines; ++i)
    {
        DrawPoint curTextPos = textPos;
        if(auto* line = get_if<PrimaryChatLine>(&chat_lines[i + pos]))
        {
            // Zeit, Spieler und danach Textnachricht
            if(!line->time_string.empty())
            {
                font->draw(curTextPos, line->time_string, FontStyle{}, time_color);
                curTextPos.x += font->getWidth(line->time_string);
            }

            if(!line->player.empty())
            {
                // Klammer 1 (<)
                font->draw(curTextPos, "<", FontStyle{}, line->player_color);
                curTextPos.x += bracket1_size;
                // Spielername
                font->draw(curTextPos, line->player, FontStyle{}, line->player_color);
                curTextPos.x += font->getWidth(line->player);
                // Klammer 2 (>)
                font->draw(curTextPos, "> ", FontStyle{}, line->player_color);
                curTextPos.x += bracket2_size;
            }
        }
        visit(
          [this, curTextPos](const auto& line) { // Draw msg
              this->font->draw(curTextPos, line.msg, FontStyle{}, line.msg_color);
          },
          chat_lines[i + pos]);
        textPos.y += font->getHeight() + 2;
    }
}

void ctrlChat::wrapLine(unsigned short i)
{
    const RawChatLine& line = raw_chat_lines[i];

    // Breite von Zeitstring und Spielername berechnen (falls vorhanden)
    unsigned short prefixWidth =
      (!line.time_string.empty() ? font->getWidth(line.time_string) : 0)
      + (!line.player.empty() ? (bracket1_size + bracket2_size + font->getWidth(line.player)) : 0);

    // Reicht die Breite des Textfeldes noch nichtmal dafür aus?
    if(prefixWidth > getSize().x - 2 - SCROLLBAR_WIDTH)
    {
        // dann können wir das gleich vergessen
        return;
    }

    // Zeilen ggf. wrappen, falls der Platz nich reicht und die Zeilenanfanänge in wi speichern
    glFont::WrapInfo wi =
      font->getWrapInfo(line.msg, getSize().x - prefixWidth - 2 - SCROLLBAR_WIDTH, getSize().x - 2 - SCROLLBAR_WIDTH);

    // Message-Strings erzeugen aus den WrapInfo
    std::vector<std::string> strings = wi.createSingleStrings(line.msg);

    // Zeilen hinzufügen
    for(unsigned i = 0; i < strings.size(); ++i)
    {
        if(i == 0)
        {
            PrimaryChatLine chatLine = line;
            chatLine.msg = strings[i];
            chat_lines.emplace_back(std::move(chatLine));
        } else
        {
            chat_lines.emplace_back(SecondaryChatLine{strings[i], line.msg_color});
        }
    }
}

void ctrlChat::addMessage(const std::string& time_string, const std::string& player, const unsigned player_color,
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
    wrapLine(raw_chat_lines.size() - 1);

    // Scrollbar Bescheid sagen
    auto* scrollbar = getCtrl<ctrlScrollBar>(0);
    scrollbar->setRange(unsigned(chat_lines.size()));

    // Waren wir am Ende? Dann mit runterscrollen
    if(scrollbar->getScrollPos() + page_size == oldlength)
        scrollbar->setScrollPos(chat_lines.size() - page_size);
}

bool ctrlChat::msgMouseMove(const MouseCoords& mc)
{
    return relayMouseMessage(&Window::msgMouseMove, mc);
}

bool ctrlChat::msgLeftDown(const MouseCoords& mc)
{
    return relayMouseMessage(&Window::msgLeftDown, mc);
}

bool ctrlChat::msgLeftUp(const MouseCoords& mc)
{
    return relayMouseMessage(&Window::msgLeftUp, mc);
}

bool ctrlChat::msgWheelUp(const MouseCoords& mc)
{
    if(isPointInRect(mc.pos, Rect(getDrawPos() + DrawPoint(2, 2), getSize() - Extent(2, 4))))
    {
        auto* scrollbar = getCtrl<ctrlScrollBar>(0);
        scrollbar->scroll(-3);
        return true;
    } else
        return false;
}

bool ctrlChat::msgWheelDown(const MouseCoords& mc)
{
    if(isPointInRect(mc.pos, Rect(getDrawPos() + DrawPoint(2, 2), getSize() - Extent(2, 4))))
    {
        auto* scrollbar = getCtrl<ctrlScrollBar>(0);
        scrollbar->scroll(+3);
        return true;
    } else
        return false;
}

unsigned ctrlChat::calcUniqueColor(const std::string& name)
{
    unsigned checksum = calcChecksumOfBuffer(name.c_str(), name.length()) * name.length();
    unsigned color = checksum | (checksum << 12) | 0xff000000;
    return color;
}
