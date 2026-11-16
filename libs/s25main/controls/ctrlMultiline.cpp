// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ctrlMultiline.h"
#include "CollisionDetection.h"
#include "ctrlScrollBar.h"
#include "driver/MouseCoords.h"
#include "ogl/glFont.h"
#include <algorithm>

ctrlMultiline::ctrlMultiline(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                             const glFont* font, FontStyle format)
    : Window(parent, id, pos, size), tc_(tc), font(font), format_(format), showBackground_(true),
      scrollbarAllowed_(true), cachedContentWidth(0)
{
    RecalcVisibleLines();
    AddScrollBar(0, DrawPoint(size.x - SCROLLBAR_WIDTH, 0), Extent(SCROLLBAR_WIDTH, size.y), SCROLLBAR_WIDTH, tc,
                 maxNumVisibleLines);
}

/**
 *  fügt eine Zeile hinzu.
 */
void ctrlMultiline::AddString(const std::string& str, unsigned color, bool scroll)
{
    lines.push_back(Line(str, color));
    RecalcWrappedLines();

    auto* scrollbar = getCtrl<ctrlScrollBar>(0);
    if(scroll && scrollbar->getScrollPos() + 1 + maxNumVisibleLines == lines.size())
        scrollbar->setScrollPos(scrollbar->getScrollPos() + 1);
}

void ctrlMultiline::Clear()
{
    lines.clear();
    RecalcWrappedLines();
}

/**
 *  zeichnet das Fenster.
 */
void ctrlMultiline::Draw_()
{
    if(showBackground_)
        Draw3D(Rect(getDrawPos(), getSize()), tc_, false);

    Window::Draw_();

    unsigned numVisibleLines = std::min<unsigned>(maxNumVisibleLines, drawLines.size());

    unsigned scrollbarPos = getCtrl<ctrlScrollBar>(0)->getScrollPos();
    DrawPoint curPos = getDrawPos() + DrawPoint(PADDING, PADDING);
    for(unsigned i = 0; i < numVisibleLines; ++i)
    {
        font->Draw(curPos, drawLines[i + scrollbarPos].str, format_, drawLines[i + scrollbarPos].color);
        curPos.y += font->getHeight();
    }
}

void ctrlMultiline::RecalcVisibleLines()
{
    if(getSize().y < 2 * PADDING)
        maxNumVisibleLines = 0;
    else
        maxNumVisibleLines = (getSize().y - 2 * PADDING) / font->getHeight();
}

void ctrlMultiline::RecalcWrappedLines()
{
    drawLines.clear();
    cachedContentWidth = 0;
    // No space for a single line, or to narrow to even show the scrollbar -> Bail out
    if(maxNumVisibleLines == 0 || getSize().x < 2 * PADDING + SCROLLBAR_WIDTH)
    {
        getCtrl<ctrlScrollBar>(0)->setRange(0);
        return;
    }
    // Calculate the wrap info for each real line (2nd pass if we need a scrollbar after breaking into lines)
    std::vector<glFont::WrapInfo> wrapInfos;
    wrapInfos.reserve(lines.size());
    bool needScrollBar = lines.size() > maxNumVisibleLines && scrollbarAllowed_;
    do
    {
        wrapInfos.clear();
        unsigned curNumLines = 0;
        unsigned maxTextWidth = getSize().x - 2 * PADDING;
        if(needScrollBar)
            maxTextWidth -= SCROLLBAR_WIDTH;
        for(unsigned i = 0; i < lines.size(); i++)
        {
            wrapInfos.push_back(font->getWrapInfo(lines[i].str, maxTextWidth, maxTextWidth));
            if(!needScrollBar)
            {
                curNumLines += wrapInfos[i].lines.size();
                if(curNumLines > maxNumVisibleLines)
                    break;
            }
        }
        // We are done, if we already knew we need a scrollbar (latest at 2nd pass)
        // or if we don't need it even after potentially breaking long lines
        if(needScrollBar || !scrollbarAllowed_ || curNumLines <= maxNumVisibleLines)
            break;
        else
            needScrollBar = true;
    } while(true); // Endless loop, exited at latest after 2nd pass

    // New create the actually drawn lines
    for(unsigned i = 0; i < wrapInfos.size(); i++)
    {
        // Special case: No break, just push the line as-is
        if(wrapInfos[i].lines.size() == 1u)
            drawLines.push_back(lines[i]);
        else
        {
            // Break it
            const std::vector<std::string> newLines = wrapInfos[i].CreateSingleStrings(lines[i].str);
            for(const std::string& line : newLines)
                drawLines.push_back(Line(line, lines[i].color));
        }
    }
    // If we don't have a scrollbar, restrict to maximum displayable lines
    if(!scrollbarAllowed_ && drawLines.size() > maxNumVisibleLines)
        drawLines.resize(maxNumVisibleLines);
    getCtrl<ctrlScrollBar>(0)->setRange(drawLines.size());
}

void ctrlMultiline::setScrollBarAllowed(bool allowed)
{
    if(scrollbarAllowed_ != allowed)
    {
        scrollbarAllowed_ = allowed;
        RecalcWrappedLines();
    }
}

bool ctrlMultiline::msgLeftDown(const MouseCoords& mc)
{
    return getCtrl<Window>(0)->msgLeftDown(mc);
}

bool ctrlMultiline::msgLeftUp(const MouseCoords& mc)
{
    return getCtrl<Window>(0)->msgLeftUp(mc);
}
bool ctrlMultiline::msgWheelUp(const MouseCoords& mc)
{
    const Extent padding(PADDING, PADDING);
    if(IsPointInRect(mc.pos, Rect(getDrawPos() + DrawPoint(padding), getSize() - padding * 2u)))
    {
        auto* scrollbar = getCtrl<ctrlScrollBar>(0);
        scrollbar->Scroll(-3);
        return true;
    } else
        return false;
}

bool ctrlMultiline::msgWheelDown(const MouseCoords& mc)
{
    const Extent padding(PADDING, PADDING);
    if(IsPointInRect(mc.pos, Rect(getDrawPos() + DrawPoint(padding), getSize() - padding * 2u)))
    {
        auto* scrollbar = getCtrl<ctrlScrollBar>(0);
        scrollbar->Scroll(+3);
        return true;
    } else
        return false;
}

bool ctrlMultiline::msgMouseMove(const MouseCoords& mc)
{
    return getCtrl<Window>(0)->msgMouseMove(mc);
}

void ctrlMultiline::Resize(const Extent& newSize)
{
    const Extent oldSize(getSize());
    const unsigned oldMaxNumVisLines = maxNumVisibleLines;
    Window::Resize(newSize);

    RecalcVisibleLines();
    auto* scrollBar = getCtrl<ctrlScrollBar>(0);
    scrollBar->setPageSize(maxNumVisibleLines);
    scrollBar->setHeight(getSize().y);
    scrollBar->setPos(DrawPoint(getSize().x - SCROLLBAR_WIDTH, 0));
    // Recalc only if:
    // - we increased the size or decreased beyond content width
    // - scrollbar requirement has changed (e.g. now need one but did not need it before)
    const bool oldNeedScrollbar = oldMaxNumVisLines < drawLines.size();
    const bool newNeedScrollbar = maxNumVisibleLines < drawLines.size();
    if(oldNeedScrollbar != newNeedScrollbar || getSize().x > oldSize.x || getSize().x < getContentWidth())
        RecalcWrappedLines();
}

/// Textzeile ersetzen. Klappt bestimmt nicht mit Scrollbar-Kram
void ctrlMultiline::setLine(const unsigned index, const std::string& str, unsigned color)
{
    if(index < lines.size())
    {
        lines[index] = Line(str, color);
        RecalcWrappedLines();
    }
}

void ctrlMultiline::setNumVisibleLines(unsigned numLines)
{
    setHeight(numLines * font->getHeight() + 2 * PADDING);
}

Extent ctrlMultiline::getContentSize() const
{
    return Extent(getContentWidth(),
                  std::min<unsigned>(getSize().y, drawLines.size() * font->getHeight() + 2u * PADDING));
}

unsigned ctrlMultiline::getContentWidth() const
{
    if(cachedContentWidth > 0)
        return cachedContentWidth;

    unsigned maxWidth = 0;
    unsigned addWidth = 2 * PADDING;
    if(drawLines.size() > maxNumVisibleLines)
        addWidth += SCROLLBAR_WIDTH;
    for(const Line& line : drawLines)
    {
        unsigned curWidth = font->getWidth(line.str) + addWidth;
        if(curWidth > maxWidth)
        {
            maxWidth = curWidth;
            if(maxWidth >= getSize().x)
                return getSize().x;
        }
    }
    cachedContentWidth = std::min<unsigned>(getSize().x, maxWidth);
    return cachedContentWidth;
}
